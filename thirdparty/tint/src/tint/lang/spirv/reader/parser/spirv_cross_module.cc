// Copyright 2026 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Builds the reader's SPIR-V model from spirv_cross::ParsedIR.
//
// SPIRV-Cross keeps the non-control instructions of each block as offsets into the module words,
// and stores the merge and terminator instructions as fields of SPIRBlock, OpPhi as
// SPIRBlock::phi_variables and function-scope OpVariable as SPIRFunction::local_variables. This
// file turns those back into instruction lists in the order SPIR-V requires.

#define TINT_SPIRV_CROSS_MODULE_IMPL
#include "src/tint/lang/spirv/reader/parser/spirv_cross_module.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <unordered_set>
#include <utility>

#include "spirv_parser.hpp"

namespace tint::spirv::reader::spvc {

namespace sc = SPIRV_CROSS_NAMESPACE;

namespace {

Instruction Make(spv::Op op, std::vector<uint32_t> operands) {
    operands.insert(operands.begin(),
                    (static_cast<uint32_t>(operands.size() + 1) << 16) | static_cast<uint32_t>(op));
    return Instruction(std::move(operands));
}

uint32_t DecorationValue(uint32_t kind, const sc::Meta::Decoration& d) {
    switch (static_cast<spv::Decoration>(kind)) {
        case spv::DecorationBuiltIn:
            return static_cast<uint32_t>(d.builtin_type);
        case spv::DecorationLocation:
            return d.location;
        case spv::DecorationComponent:
            return d.component;
        case spv::DecorationDescriptorSet:
            return d.set;
        case spv::DecorationBinding:
            return d.binding;
        case spv::DecorationOffset:
            return d.offset;
        case spv::DecorationSpecId:
            return d.spec_id;
        case spv::DecorationIndex:
            return d.index;
        case spv::DecorationArrayStride:
            return d.array_stride;
        case spv::DecorationMatrixStride:
            return d.matrix_stride;
        case spv::DecorationInputAttachmentIndex:
            return d.input_attachment;
        case spv::DecorationXfbBuffer:
            return d.xfb_buffer;
        case spv::DecorationXfbStride:
            return d.xfb_stride;
        case spv::DecorationStream:
            return d.stream;
        default:
            return 0;
    }
}

std::vector<Decoration> DecorationsOf(const sc::Meta::Decoration& d) {
    std::vector<Decoration> out;
    d.decoration_flags.for_each_bit(
        [&](uint32_t kind) { out.push_back(Decoration{kind, DecorationValue(kind, d)}); });
    return out;
}

}  // namespace

Instruction::Instruction(std::vector<uint32_t> words) : words_(std::move(words)) {
    opcode_ = words_.empty() ? 0u : (words_[0] & 0xffffu);
    spv::HasResultAndType(static_cast<spv::Op>(opcode_), &has_result_, &has_type_);
}

const Instruction* BasicBlock::GetMergeInst() const {
    if (insts_.size() < 2) {
        return nullptr;
    }
    const auto& inst = insts_[insts_.size() - 2];
    auto op = static_cast<spv::Op>(inst.OpcodeWord());
    if (op == spv::OpSelectionMerge || op == spv::OpLoopMerge) {
        return &inst;
    }
    return nullptr;
}

const Instruction* BasicBlock::GetLoopMergeInst() const {
    auto* inst = GetMergeInst();
    if (inst && static_cast<spv::Op>(inst->OpcodeWord()) == spv::OpLoopMerge) {
        return inst;
    }
    return nullptr;
}

uint32_t BasicBlock::MergeBlockIdIfAny() const {
    auto* inst = GetMergeInst();
    return inst ? inst->GetSingleWordInOperand(0) : 0u;
}

uint32_t BasicBlock::ContinueBlockIdIfAny() const {
    auto* inst = GetLoopMergeInst();
    return inst ? inst->GetSingleWordInOperand(1) : 0u;
}

bool BasicBlock::IsReturn() const {
    if (insts_.empty()) {
        return false;
    }
    auto op = static_cast<spv::Op>(insts_.back().OpcodeWord());
    return op == spv::OpReturn || op == spv::OpReturnValue;
}

void BasicBlock::ForEachSuccessorLabel(const std::function<void(uint32_t)>& f) const {
    if (insts_.empty()) {
        return;
    }
    const auto& term = insts_.back();
    switch (static_cast<spv::Op>(term.OpcodeWord())) {
        case spv::OpBranch:
            f(term.GetSingleWordInOperand(0));
            break;
        case spv::OpBranchConditional:
            f(term.GetSingleWordInOperand(1));
            f(term.GetSingleWordInOperand(2));
            break;
        case spv::OpSwitch:
            f(term.GetSingleWordInOperand(1));
            for (uint32_t i = 3; i < term.NumInOperands(); i += 2) {
                f(term.GetSingleWordInOperand(i));
            }
            break;
        default:
            break;
    }
}

void Function::ForEachParam(const std::function<void(const Instruction*)>& f) const {
    for (const auto& param : params_) {
        f(&param);
    }
}

const BasicBlock* Function::FindBlock(uint32_t id) const {
    auto iter = block_by_id_.find(id);
    return iter != block_by_id_.end() ? iter->second : nullptr;
}

void CFG::ComputeStructuredOrder(const Function* func,
                                 const BasicBlock* root,
                                 const BasicBlock* end,
                                 std::list<const BasicBlock*>* order) const {
    (void)module_;
    // Structured successors: the merge block, then the continue target, then the branch targets.
    auto successors = [&](const BasicBlock* b) {
        std::vector<const BasicBlock*> out;
        auto add = [&](uint32_t id) {
            if (auto* s = func->FindBlock(id)) {
                out.push_back(s);
            }
        };
        if (uint32_t merge = b->MergeBlockIdIfAny()) {
            add(merge);
            if (uint32_t cont = b->ContinueBlockIdIfAny()) {
                add(cont);
            }
        }
        b->ForEachSuccessorLabel(add);
        return out;
    };

    // Depth-first traversal, recording the reverse post-order, not walking past `end`.
    struct Entry {
        const BasicBlock* block;
        std::vector<const BasicBlock*> succs;
        size_t next;
    };
    std::unordered_set<const BasicBlock*> visited{root};
    std::vector<Entry> stack;
    stack.push_back({root, successors(root), 0});
    while (!stack.empty()) {
        auto& top = stack.back();
        if (top.block == end || top.next == top.succs.size()) {
            order->push_front(top.block);
            stack.pop_back();
            continue;
        }
        const BasicBlock* child = top.succs[top.next++];
        if (visited.insert(child).second) {
            stack.push_back({child, successors(child), 0});
        }
    }
}

struct Module::Impl {
    sc::ParsedIR ir;

    template <typename T>
    const T* Get(uint32_t id) const {
        if (id >= ir.ids.size() || ir.ids[id].get_type() != static_cast<sc::Types>(T::type)) {
            return nullptr;
        }
        return &ir.ids[id].get<T>();
    }

    /// Reads the OpExtInstImport names straight from the module words, as SPIRV-Cross only keeps
    /// an enum for the instruction sets it knows.
    void ReadExtInstImports(Module& m) const {
        const auto& w = ir.spirv;
        for (size_t offset = 5; offset < w.size();) {
            uint32_t count = w[offset] >> 16;
            auto op = static_cast<spv::Op>(w[offset] & 0xffffu);
            if (count == 0) {
                break;
            }
            if (op == spv::OpExtInstImport && count >= 3) {
                const char* str = reinterpret_cast<const char*>(&w[offset + 2]);
                size_t max_len = (count - 2) * sizeof(uint32_t);
                m.ext_inst_imports_.push_back(
                    ExtInstImport{w[offset + 1], std::string(str, strnlen(str, max_len))});
            }
            if (op == spv::OpFunction) {
                break;  // No more module-level declarations.
            }
            offset += count;
        }
    }

    void Populate(Module& m) const {
        m.extensions_.assign(ir.declared_extensions.begin(), ir.declared_extensions.end());
        ReadExtInstImports(m);

        for (const auto& [id, meta] : ir.meta) {
            if (!meta.decoration.alias.empty()) {
                m.names_[id] = meta.decoration.alias;
            }
            for (size_t i = 0; i < meta.members.size(); i++) {
                if (!meta.members[i].alias.empty()) {
                    auto& names = m.member_names_[id];
                    names.resize(std::max(names.size(), i + 1));
                    names[i] = meta.members[i].alias;
                }
            }
        }

        // Function-scope OpUndef also appear in the module-wide list; keep them in their blocks.
        std::unordered_set<uint32_t> function_undefs;

        for (uint32_t fid : ir.ids_for_type[sc::TypeFunction]) {
            const auto* f = Get<sc::SPIRFunction>(fid);
            if (!f) {
                continue;
            }
            auto func = std::make_unique<Function>();
            func->id_ = fid;
            func->return_type_ = f->return_type;
            for (const auto& arg : f->arguments) {
                func->params_.push_back(Make(spv::OpFunctionParameter, {arg.type, arg.id}));
            }
            for (uint32_t bid : f->blocks) {
                const auto* blk = Get<sc::SPIRBlock>(bid);
                if (!blk) {
                    throw sc::CompilerError("missing block " + std::to_string(bid));
                }
                auto bb = std::make_unique<BasicBlock>();
                bb->id_ = bid;
                auto& insts = bb->insts_;

                // Function-scope variables live at the start of the entry block.
                if (bid == f->entry_block) {
                    for (uint32_t vid : f->local_variables) {
                        const auto* var = Get<sc::SPIRVariable>(vid);
                        if (!var || var->phi_variable) {
                            continue;
                        }
                        std::vector<uint32_t> ops{var->basetype, vid,
                                                  static_cast<uint32_t>(var->storage)};
                        if (var->initializer) {
                            ops.push_back(var->initializer);
                        }
                        insts.push_back(Make(spv::OpVariable, std::move(ops)));
                    }
                }

                // OpPhi instructions, in declaration order.
                std::vector<uint32_t> phi_order;
                std::unordered_map<uint32_t, std::vector<uint32_t>> phi_ops;
                for (const auto& phi : blk->phi_variables) {
                    uint32_t pid = phi.function_variable;
                    auto [iter, added] = phi_ops.insert({pid, {}});
                    if (added) {
                        phi_order.push_back(pid);
                        const auto* var = Get<sc::SPIRVariable>(pid);
                        iter->second = {var ? uint32_t(var->basetype) : 0u, pid};
                    }
                    iter->second.push_back(phi.local_variable);
                    iter->second.push_back(phi.parent);
                }
                for (uint32_t pid : phi_order) {
                    insts.push_back(Make(spv::OpPhi, std::move(phi_ops[pid])));
                }

                for (const auto& op : blk->ops) {
                    auto opcode = static_cast<spv::Op>(op.op);
                    if (opcode == spv::OpLine || opcode == spv::OpNoLine) {
                        continue;  // Debug line info is not part of the instruction stream.
                    }
                    std::vector<uint32_t> words(ir.spirv.begin() + op.offset - 1,
                                                ir.spirv.begin() + op.offset + op.length);
                    Instruction inst(std::move(words));
                    if (opcode == spv::OpUndef) {
                        function_undefs.insert(inst.result_id());
                    }
                    insts.push_back(std::move(inst));
                }

                // Merge instruction.
                if (blk->merge == sc::SPIRBlock::MergeSelection) {
                    insts.push_back(Make(spv::OpSelectionMerge, {blk->next_block, 0u}));
                } else if (blk->merge == sc::SPIRBlock::MergeLoop) {
                    insts.push_back(
                        Make(spv::OpLoopMerge, {blk->merge_block, blk->continue_block, 0u}));
                }

                // Terminator.
                switch (blk->terminator) {
                    case sc::SPIRBlock::Direct:
                        insts.push_back(Make(spv::OpBranch, {blk->next_block}));
                        break;
                    case sc::SPIRBlock::Select:
                        insts.push_back(Make(spv::OpBranchConditional,
                                             {blk->condition, blk->true_block, blk->false_block}));
                        break;
                    case sc::SPIRBlock::MultiSelect: {
                        // SPIRV-Cross fills both case lists when the operand count allows
                        // either reading; the selector width decides which one is real.
                        uint32_t width = 32;
                        auto width_iter = ir.load_type_width.find(blk->condition);
                        if (width_iter != ir.load_type_width.end()) {
                            width = width_iter->second;
                        } else if (const auto* c = Get<sc::SPIRConstant>(blk->condition)) {
                            if (const auto* ct = Get<sc::SPIRType>(c->constant_type)) {
                                width = ct->width;
                            }
                        }
                        if (width > 32) {
                            throw sc::CompilerError("64-bit switch selectors are not supported");
                        }
                        std::vector<uint32_t> ops{blk->condition, blk->default_block};
                        for (const auto& c : blk->cases_32bit) {
                            ops.push_back(static_cast<uint32_t>(c.value));
                            ops.push_back(c.block);
                        }
                        insts.push_back(Make(spv::OpSwitch, std::move(ops)));
                        break;
                    }
                    case sc::SPIRBlock::Return:
                        if (blk->return_value) {
                            insts.push_back(Make(spv::OpReturnValue, {blk->return_value}));
                        } else {
                            insts.push_back(Make(spv::OpReturn, {}));
                        }
                        break;
                    case sc::SPIRBlock::Unreachable:
                        insts.push_back(Make(spv::OpUnreachable, {}));
                        break;
                    case sc::SPIRBlock::Kill:
                        insts.push_back(Make(spv::OpKill, {}));
                        break;
                    default:
                        throw sc::CompilerError("unsupported block terminator in block " +
                                          std::to_string(bid));
                }

                func->block_by_id_[bid] = bb.get();
                func->blocks_.push_back(std::move(bb));
            }
            m.functions_.push_back(std::move(func));
        }

        // Module-scope specialization constants and undefs, in declaration order.
        for (uint32_t id : ir.ids_for_constant_undef_or_type) {
            if (const auto* c = Get<sc::SPIRConstant>(id)) {
                if (!c->specialization) {
                    continue;
                }
                const auto* ty = Get<sc::SPIRType>(c->constant_type);
                bool scalar = ty && ty->vecsize == 1 && ty->columns == 1 && ty->array.empty() &&
                              ty->basetype != sc::SPIRType::Struct;
                if (scalar && ty->basetype == sc::SPIRType::Boolean) {
                    m.types_values_.push_back(Make(
                        c->scalar() ? spv::OpSpecConstantTrue : spv::OpSpecConstantFalse,
                        {c->constant_type, id}));
                } else if (scalar) {
                    m.types_values_.push_back(
                        Make(spv::OpSpecConstant, {c->constant_type, id, c->scalar()}));
                } else {
                    std::vector<uint32_t> ops{c->constant_type, id};
                    for (auto sub : c->subconstants) {
                        ops.push_back(sub);
                    }
                    m.types_values_.push_back(Make(spv::OpSpecConstantComposite, std::move(ops)));
                }
            } else if (const auto* op = Get<sc::SPIRConstantOp>(id)) {
                std::vector<uint32_t> ops{op->basetype, id, static_cast<uint32_t>(op->opcode)};
                ops.insert(ops.end(), op->arguments.begin(), op->arguments.end());
                m.types_values_.push_back(Make(spv::OpSpecConstantOp, std::move(ops)));
            } else if (const auto* undef = Get<sc::SPIRUndef>(id)) {
                if (!function_undefs.count(id)) {
                    m.types_values_.push_back(Make(spv::OpUndef, {undef->basetype, id}));
                }
            }
        }

        // Module-scope variables, in declaration order.
        for (uint32_t id : ir.ids_for_type[sc::TypeVariable]) {
            const auto* var = Get<sc::SPIRVariable>(id);
            if (!var || var->storage == spv::StorageClassFunction) {
                continue;
            }
            std::vector<uint32_t> ops{var->basetype, id, static_cast<uint32_t>(var->storage)};
            if (var->initializer) {
                ops.push_back(var->initializer);
            }
            m.types_values_.push_back(Make(spv::OpVariable, std::move(ops)));
        }

        // Entry points and execution modes, ordered by function ID.
        std::vector<uint32_t> ep_ids;
        for (const auto& [fid, ep] : ir.entry_points) {
            ep_ids.push_back(fid);
        }
        std::sort(ep_ids.begin(), ep_ids.end());
        for (uint32_t fid : ep_ids) {
            const auto& ep = ir.entry_points.at(fid);
            m.entry_points_.push_back(
                EntryPoint{static_cast<uint32_t>(ep.model), fid, ep.orig_name});
            ep.flags.for_each_bit([&](uint32_t mode) {
                std::vector<uint32_t> ops{fid, mode};
                if (mode == spv::ExecutionModeLocalSize) {
                    ops.insert(ops.end(),
                               {ep.workgroup_size.x, ep.workgroup_size.y, ep.workgroup_size.z});
                } else if (mode == spv::ExecutionModeLocalSizeId) {
                    ops.insert(ops.end(), {ep.workgroup_size.id_x, ep.workgroup_size.id_y,
                                           ep.workgroup_size.id_z});
                }
                m.execution_modes_.push_back(Make(spv::OpExecutionMode, std::move(ops)));
            });
        }
    }

    Constant BuildConstant(const sc::SPIRConstant& c, uint32_t type_id) const {
        Constant out;
        out.type_id = type_id;
        const auto* ty = Get<sc::SPIRType>(type_id);
        if (!ty) {
            return out;
        }
        if (!ty->array.empty() || ty->basetype == sc::SPIRType::Struct) {
            for (auto sub_id : c.subconstants) {
                if (const auto* sub = Get<sc::SPIRConstant>(sub_id)) {
                    out.components.push_back(BuildConstant(*sub, sub->constant_type));
                }
            }
        } else if (ty->columns > 1) {
            uint32_t col_ty = ty->parent_type;
            const auto* col = Get<sc::SPIRType>(col_ty);
            uint32_t elem_ty = col ? uint32_t(col->parent_type) : 0u;
            for (uint32_t i = 0; i < ty->columns; i++) {
                Constant column;
                column.type_id = col_ty;
                for (uint32_t j = 0; j < ty->vecsize; j++) {
                    Constant el;
                    el.type_id = elem_ty;
                    el.bits = c.scalar(i, j);
                    column.components.push_back(el);
                }
                out.components.push_back(std::move(column));
            }
        } else if (ty->vecsize > 1) {
            for (uint32_t j = 0; j < ty->vecsize; j++) {
                Constant el;
                el.type_id = ty->parent_type;
                el.bits = c.scalar(0, j);
                out.components.push_back(el);
            }
        } else {
            out.bits = c.scalar();
        }
        return out;
    }
};

Module::Module() : impl_(std::make_unique<Impl>()) {}
Module::~Module() = default;

std::unique_ptr<Module> Module::Build(const uint32_t* words, size_t count, std::string* error) {
    std::unique_ptr<Module> m(new Module());
    try {
        sc::Parser parser(words, count);
        parser.parse();
        m->impl_->ir = std::move(parser.get_parsed_ir());
        m->impl_->Populate(*m);
    } catch (const std::exception& e) {
        *error = std::string("SPIRV-Cross failed to parse the module: ") + e.what();
        return nullptr;
    }
    return m;
}

std::vector<Decoration> Module::GetDecorationsFor(uint32_t id) const {
    const auto* meta = impl_->ir.find_meta(id);
    if (!meta) {
        return {};
    }
    return DecorationsOf(meta->decoration);
}

const Type* Module::GetType(uint32_t id) const {
    auto iter = types_.find(id);
    if (iter != types_.end()) {
        return iter->second.get();
    }
    const auto* t = impl_->Get<sc::SPIRType>(id);
    if (!t) {
        return nullptr;
    }
    auto out = std::make_unique<Type>();
    out->id = id;
    out->description =
        "type %" + std::to_string(id) + " (opcode " + std::to_string(uint32_t(t->op)) + ")";
    auto image = [&] {
        out->image.sampled_type = t->image.type;
        out->image.dim = static_cast<uint32_t>(t->image.dim);
        out->image.depth = t->image.depth ? 1u : 0u;
        out->image.arrayed = t->image.arrayed;
        out->image.multisampled = t->image.ms;
        out->image.sampled = t->image.sampled;
        out->image.format = static_cast<uint32_t>(t->image.format);
        // Like spvtools::opt::analysis::Image, an absent access qualifier reads as ReadOnly.
        out->image.access = t->image.access == spv::AccessQualifierMax
                                ? static_cast<uint32_t>(spv::AccessQualifierReadOnly)
                                : static_cast<uint32_t>(t->image.access);
    };
    if (t->basetype == sc::SPIRType::SampledImage && !t->pointer && t->array.empty()) {
        out->kind = Type::kSampledImage;
        image();
    } else {
        switch (t->op) {
            case spv::OpTypeVoid:
                out->kind = Type::kVoid;
                break;
            case spv::OpTypeBool:
                out->kind = Type::kBool;
                break;
            case spv::OpTypeInt:
                out->kind = Type::kInteger;
                out->width = t->width;
                out->is_signed = t->basetype == sc::SPIRType::SByte ||
                                 t->basetype == sc::SPIRType::Short ||
                                 t->basetype == sc::SPIRType::Int ||
                                 t->basetype == sc::SPIRType::Int64;
                break;
            case spv::OpTypeFloat:
                out->kind = Type::kFloat;
                out->width = t->width;
                break;
            case spv::OpTypeVector:
                out->kind = Type::kVector;
                out->element_type = t->parent_type;
                out->count = t->vecsize;
                break;
            case spv::OpTypeMatrix:
                out->kind = Type::kMatrix;
                out->element_type = t->parent_type;
                out->count = t->columns;
                break;
            case spv::OpTypeArray:
                out->kind = Type::kArray;
                out->element_type = t->parent_type;
                out->count = t->array.back();
                out->count_is_spec_constant = !t->array_size_literal.back();
                break;
            case spv::OpTypeRuntimeArray:
                out->kind = Type::kRuntimeArray;
                out->element_type = t->parent_type;
                break;
            case spv::OpTypeStruct: {
                out->kind = Type::kStruct;
                out->members.assign(t->member_types.begin(), t->member_types.end());
                out->member_decorations.resize(out->members.size());
                if (const auto* meta = impl_->ir.find_meta(id)) {
                    for (size_t i = 0; i < meta->members.size() && i < out->members.size(); i++) {
                        out->member_decorations[i] = DecorationsOf(meta->members[i]);
                    }
                }
                break;
            }
            case spv::OpTypePointer:
                out->kind = Type::kPointer;
                out->element_type = t->parent_type;
                out->storage_class = static_cast<uint32_t>(t->storage);
                break;
            case spv::OpTypeSampler:
                out->kind = Type::kSampler;
                break;
            case spv::OpTypeImage:
                out->kind = Type::kImage;
                image();
                break;
            default:
                out->kind = Type::kUnsupported;
                break;
        }
    }
    auto* result = out.get();
    types_[id] = std::move(out);
    return result;
}

std::optional<Constant> Module::FindDeclaredConstant(uint32_t id) const {
    const auto* c = impl_->Get<sc::SPIRConstant>(id);
    if (!c || c->specialization) {
        return std::nullopt;
    }
    return impl_->BuildConstant(*c, c->constant_type);
}

}  // namespace tint::spirv::reader::spvc
