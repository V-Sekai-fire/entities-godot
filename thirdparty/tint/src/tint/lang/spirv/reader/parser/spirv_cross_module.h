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

// Godot: a small in-memory SPIR-V module for the SPIR-V reader, so that the reader does not
// depend on SPIRV-Tools. The binary is checked with SPIRV-Cross' spirv_cross::Parser, the two
// SPIRV-Tools passes the reader runs (split combined image samplers, resolve binding conflicts)
// are done here, and the module is then held as plain instruction lists.
//
// The classes mirror the subset of the `spvtools::opt` API that parser.cc uses (IRContext,
// Module, Function, BasicBlock, Instruction, analysis::Type, analysis::Constant and the managers)
// with the same names and meanings, so parser.cc only swaps `spvtools::opt::` for `spvc::`.
//
// This header deliberately does not include any SPIRV-Cross header: SPIRV-Cross uses the C++03
// `spirv.hpp` header, the parser uses the C++11 `spirv.hpp11` header, and both define
// `namespace spv` behind the same include guard. Only spirv_cross_module.cc sees SPIRV-Cross,
// and it defines TINT_SPIRV_CROSS_MODULE_IMPL to hide the spirv.hpp11 based accessors.

#ifndef SRC_TINT_LANG_SPIRV_READER_PARSER_SPIRV_CROSS_MODULE_H_
#define SRC_TINT_LANG_SPIRV_READER_PARSER_SPIRV_CROSS_MODULE_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef TINT_SPIRV_CROSS_MODULE_IMPL
#include "spirv/unified1/spirv.hpp11"
#define TINT_SPVC_ENUM(T, v) static_cast<spv::T>(v)
#endif

namespace tint::spirv::reader::spvc {

class BasicBlock;
class Function;
class Module;
class IRContext;
class ConstantManager;

/// One logical operand of an instruction.
struct Operand {
    /// The operand words
    std::vector<uint32_t> words;
    /// @returns the operand as a literal string
    std::string AsString() const;
    /// @returns the operand as an ID
    uint32_t AsId() const { return words.empty() ? 0u : words[0]; }
};

/// A SPIR-V instruction, stored as its raw words.
class Instruction {
  public:
    /// @param words the full instruction, including the leading word count / opcode word
    explicit Instruction(std::vector<uint32_t> words);

#ifndef TINT_SPIRV_CROSS_MODULE_IMPL
    /// @returns the opcode
    spv::Op opcode() const { return static_cast<spv::Op>(opcode_); }
#endif
    /// @returns the opcode as a raw word
    uint32_t OpcodeWord() const { return opcode_; }
    /// @returns the result type ID, or 0
    uint32_t type_id() const { return has_type_ ? words_[1] : 0; }
    /// @returns the result ID, or 0
    uint32_t result_id() const { return has_result_ ? words_[has_type_ ? 2 : 1] : 0; }
    /// @returns the number of logical operands, including the result type and result ID
    uint32_t NumOperands() const { return static_cast<uint32_t>(operands_.size()); }
    /// @returns the number of logical operands, excluding the result type and result ID
    uint32_t NumInOperands() const { return NumOperands() - FirstInOperand(); }
    /// @returns the number of operand words, including the result type and result ID
    uint32_t NumOperandWords() const { return static_cast<uint32_t>(words_.size() - 1); }
    /// @returns the number of operand words, excluding the result type and result ID
    uint32_t NumInOperandWords() const { return NumOperandWords() - FirstInOperand(); }
    /// @param i the operand index, counting the result type and result ID
    /// @returns the first word of the operand
    uint32_t GetSingleWordOperand(uint32_t i) const { return words_[operands_[i].first]; }
    /// @param i the operand index, not counting the result type and result ID
    /// @returns the first word of the operand
    uint32_t GetSingleWordInOperand(uint32_t i) const {
        return GetSingleWordOperand(i + FirstInOperand());
    }
    /// @param i the operand index, counting the result type and result ID
    /// @returns the operand
    Operand GetOperand(uint32_t i) const;
    /// @param i the operand index, not counting the result type and result ID
    /// @returns the operand
    Operand GetInOperand(uint32_t i) const { return GetOperand(i + FirstInOperand()); }
    /// @returns the instruction words
    const std::vector<uint32_t>& words() const { return words_; }
    /// Replaces a single-word in-operand.
    /// @param i the operand index, not counting the result type and result ID
    /// @param value the new value
    void SetInOperandWord(uint32_t i, uint32_t value) {
        words_[operands_[i + FirstInOperand()].first] = value;
    }

  private:
    uint32_t FirstInOperand() const { return (has_type_ ? 1u : 0u) + (has_result_ ? 1u : 0u); }

    std::vector<uint32_t> words_;
    /// (first word, word count) of each logical operand
    std::vector<std::pair<uint32_t, uint32_t>> operands_;
    uint32_t opcode_ = 0;
    bool has_type_ = false;
    bool has_result_ = false;
};

/// A SPIR-V basic block. Like spvtools::opt::BasicBlock, the OpLabel is not part of the
/// instruction list; OpPhi, function-scope OpVariable, the merge instruction (if any) and the
/// terminator are.
class BasicBlock {
  public:
    /// @returns the block label ID
    uint32_t id() const { return id_; }
    /// @returns the function the block belongs to
    Function* GetParent() const { return parent_; }

    /// @returns the first instruction
    std::list<Instruction>::iterator begin() { return insts_.begin(); }
    /// @returns the end of the instruction list
    std::list<Instruction>::iterator end() { return insts_.end(); }
    /// @returns the first instruction
    std::list<Instruction>::const_iterator begin() const { return insts_.begin(); }
    /// @returns the end of the instruction list
    std::list<Instruction>::const_iterator end() const { return insts_.end(); }

    /// @returns the OpSelectionMerge or OpLoopMerge instruction, or nullptr
    Instruction* GetMergeInst() const;
    /// @returns the OpLoopMerge instruction, or nullptr
    Instruction* GetLoopMergeInst() const;
    /// @returns the block terminator
    Instruction* terminator() const;
    /// @returns the merge block ID of the merge instruction, or 0
    uint32_t MergeBlockIdIfAny() const;
    /// @returns the continue target ID of the OpLoopMerge instruction, or 0
    uint32_t ContinueBlockIdIfAny() const;
    /// @returns true if the block has an OpLoopMerge
    bool IsLoopHeader() const { return GetLoopMergeInst() != nullptr; }
    /// @returns true if the block ends with OpReturn or OpReturnValue
    bool IsReturn() const;
    /// Calls @p f for each successor label of the terminator.
    void ForEachSuccessorLabel(const std::function<void(uint32_t)>& f) const;

  private:
    friend class IRContext;
    uint32_t id_ = 0;
    Function* parent_ = nullptr;
    mutable std::list<Instruction> insts_;
};

/// A SPIR-V function definition.
class Function {
  public:
    /// @returns the function result ID
    uint32_t result_id() const { return def_.result_id(); }
    /// @returns the function return type ID
    uint32_t type_id() const { return def_.type_id(); }
    /// Calls @p f with each OpFunctionParameter instruction.
    void ForEachParam(const std::function<void(Instruction*)>& f) {
        for (auto& p : params_) {
            f(&p);
        }
    }
    /// @returns the entry block
    BasicBlock* entry() const { return blocks_.empty() ? nullptr : blocks_.front().get(); }
    /// @param id a block label ID
    /// @returns the block with label @p id, or nullptr
    BasicBlock* FindBlock(uint32_t id) const;
    /// @returns the blocks in declaration order
    const std::vector<std::unique_ptr<BasicBlock>>& blocks() const { return blocks_; }

    /// @param def the OpFunction instruction
    explicit Function(Instruction def) : def_(std::move(def)) {}

  private:
    friend class IRContext;
    Instruction def_;
    std::list<Instruction> params_;
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
    std::unordered_map<uint32_t, BasicBlock*> block_by_id_;
};

/// The module-level instruction sections, in the layout of spvtools::opt::Module.
class Module {
  public:
    /// Function iteration
    class iterator {
      public:
        explicit iterator(std::list<Function>::iterator it) : it_(it) {}
        Function& operator*() const { return *it_; }
        Function* operator->() const { return &*it_; }
        iterator& operator++() {
            ++it_;
            return *this;
        }
        bool operator!=(const iterator& other) const { return it_ != other.it_; }
        bool operator==(const iterator& other) const { return it_ == other.it_; }

      private:
        std::list<Function>::iterator it_;
    };
    /// @returns the first function
    iterator begin() { return iterator(functions_.begin()); }
    /// @returns the end of the function list
    iterator end() { return iterator(functions_.end()); }

    /// @returns OpCapability instructions
    std::list<Instruction>& capabilities() { return capabilities_; }
    /// @returns OpExtension instructions
    std::list<Instruction>& extensions() { return extensions_; }
    /// @returns OpExtInstImport instructions
    std::list<Instruction>& ext_inst_imports() { return ext_inst_imports_; }
    /// @returns OpEntryPoint instructions
    std::list<Instruction>& entry_points() { return entry_points_; }
    /// @returns OpExecutionMode instructions
    std::list<Instruction>& execution_modes() { return execution_modes_; }
    /// @returns OpName and OpMemberName instructions
    std::list<Instruction>& debugs2() { return debugs2_; }
    /// @returns decoration instructions
    std::list<Instruction>& annotations() { return annotations_; }
    /// @returns type, constant, global variable and OpUndef instructions
    std::list<Instruction>& types_values() { return types_values_; }

  private:
    friend class IRContext;
    std::list<Instruction> capabilities_;
    std::list<Instruction> extensions_;
    std::list<Instruction> ext_inst_imports_;
    std::list<Instruction> memory_model_;
    std::list<Instruction> entry_points_;
    std::list<Instruction> execution_modes_;
    std::list<Instruction> debugs1_;
    std::list<Instruction> debugs2_;
    std::list<Instruction> debugs3_;
    std::list<Instruction> annotations_;
    std::list<Instruction> types_values_;
    std::list<Instruction> ext_inst_debuginfo_;
    std::list<Function> functions_;
};

namespace analysis {

/// A SPIR-V type. One class for every kind of type: the As*() casts return this object when the
/// kind matches, and the accessors of the matching spvtools::opt::analysis class are available.
class Type {
  public:
    /// The kind of type, as in spvtools::opt::analysis::Type::Kind
    enum Kind {
        kVoid,
        kBool,
        kInteger,
        kFloat,
        kVector,
        kMatrix,
        kImage,
        kSampler,
        kSampledImage,
        kArray,
        kRuntimeArray,
        kStruct,
        kPointer,
        kFunction,
        kUnsupported,
    };

    /// @returns the kind of type
    Kind kind() const { return kind_; }

    const Type* AsInteger() const { return kind_ == kInteger ? this : nullptr; }
    const Type* AsFloat() const { return kind_ == kFloat ? this : nullptr; }
    const Type* AsVector() const { return kind_ == kVector ? this : nullptr; }
    const Type* AsMatrix() const { return kind_ == kMatrix ? this : nullptr; }
    const Type* AsImage() const { return kind_ == kImage ? this : nullptr; }
    const Type* AsSampler() const { return kind_ == kSampler ? this : nullptr; }
    const Type* AsSampledImage() const { return kind_ == kSampledImage ? this : nullptr; }
    const Type* AsArray() const { return kind_ == kArray ? this : nullptr; }
    const Type* AsRuntimeArray() const { return kind_ == kRuntimeArray ? this : nullptr; }
    const Type* AsStruct() const { return kind_ == kStruct ? this : nullptr; }
    const Type* AsPointer() const { return kind_ == kPointer ? this : nullptr; }
    const Type* AsFunction() const { return kind_ == kFunction ? this : nullptr; }

    /// The array length, as in spvtools::opt::analysis::Array::LengthInfo
    struct LengthInfo {
        enum Case : uint32_t {
            kConstant = 0,
            kConstantWithSpecId = 1,
            kDefiningId = 2,
        };
        /// The ID of the length instruction
        uint32_t id = 0;
        /// The length case, then the value words (or spec ID, or defining ID)
        std::vector<uint32_t> words;
    };

    /// Integer and float width
    uint32_t width() const { return width_; }
    /// Integer signedness
    bool IsSigned() const { return signed_; }
    /// Element type of vectors, matrices and arrays
    const Type* element_type() const { return element_type_; }
    /// Element count of vectors, column count of matrices
    uint32_t element_count() const { return element_count_; }
    /// Array length
    const LengthInfo& length_info() const { return length_info_; }
    /// Struct member types
    const std::vector<const Type*>& element_types() const { return element_types_; }
    /// Struct member decorations: member index to (decoration, literals...) lists
    const std::map<uint32_t, std::vector<std::vector<uint32_t>>>& element_decorations() const {
        return element_decorations_;
    }
    /// Type decorations: (decoration, literals...) lists
    const std::vector<std::vector<uint32_t>>& decorations() const { return decorations_; }
    /// Image sampled type
    const Type* sampled_type() const { return element_type_; }
    /// Sampled image's image type
    const Type* image_type() const { return element_type_; }
    /// Pointer pointee type
    const Type* pointee_type() const { return element_type_; }
    /// Image depth
    uint32_t depth() const { return depth_; }
    /// Image arrayed
    bool is_arrayed() const { return arrayed_; }
    /// Image multisampled
    bool is_multisampled() const { return multisampled_; }
    /// Image sampled
    uint32_t sampled() const { return sampled_; }
#ifndef TINT_SPIRV_CROSS_MODULE_IMPL
    /// Image dimension
    spv::Dim dim() const { return TINT_SPVC_ENUM(Dim, dim_); }
    /// Image format
    spv::ImageFormat format() const { return TINT_SPVC_ENUM(ImageFormat, format_); }
    /// Image access qualifier (ReadOnly if absent, like spvtools)
    spv::AccessQualifier access_qualifier() const {
        return TINT_SPVC_ENUM(AccessQualifier, access_);
    }
    /// Pointer storage class
    spv::StorageClass storage_class() const { return TINT_SPVC_ENUM(StorageClass, storage_); }
#endif
    /// @returns a description of the type
    std::string str() const { return str_; }

  private:
    friend class tint::spirv::reader::spvc::IRContext;
    Kind kind_ = kUnsupported;
    uint32_t width_ = 0;
    bool signed_ = false;
    const Type* element_type_ = nullptr;
    uint32_t element_count_ = 0;
    LengthInfo length_info_;
    std::vector<const Type*> element_types_;
    std::map<uint32_t, std::vector<std::vector<uint32_t>>> element_decorations_;
    std::vector<std::vector<uint32_t>> decorations_;
    uint32_t dim_ = 0;
    uint32_t depth_ = 0;
    bool arrayed_ = false;
    bool multisampled_ = false;
    uint32_t sampled_ = 0;
    uint32_t format_ = 0;
    uint32_t access_ = 0;
    uint32_t storage_ = 0;
    std::string str_;
};

/// A SPIR-V constant. One class for every kind of constant, like analysis::Type.
class Constant {
  public:
    /// The kind of constant
    enum Kind { kNull, kBool, kInt, kFloat, kComposite };

    /// @returns the constant type
    const Type* type() const { return type_; }

    const Constant* AsNullConstant() const { return kind_ == kNull ? this : nullptr; }
    const Constant* AsBoolConstant() const { return kind_ == kBool ? this : nullptr; }
    const Constant* AsIntConstant() const { return kind_ == kInt ? this : nullptr; }
    const Constant* AsFloatConstant() const { return kind_ == kFloat ? this : nullptr; }
    const Constant* AsScalarConstant() const {
        return (kind_ == kBool || kind_ == kInt || kind_ == kFloat) ? this : nullptr;
    }
    const Constant* AsVectorConstant() const { return Composite(Type::kVector); }
    const Constant* AsMatrixConstant() const { return Composite(Type::kMatrix); }
    const Constant* AsArrayConstant() const { return Composite(Type::kArray); }
    const Constant* AsStructConstant() const { return Composite(Type::kStruct); }

    /// Bool value
    bool value() const { return !words_.empty() && words_[0] != 0; }
    /// Scalar literal words
    const std::vector<uint32_t>& words() const { return words_; }
    /// @returns the low word of the scalar, 0 for null constants
    uint32_t GetU32BitValue() const { return words_.empty() ? 0u : words_[0]; }
    /// @returns the low word of the scalar as signed, 0 for null constants
    int32_t GetS32BitValue() const { return static_cast<int32_t>(GetU32BitValue()); }
    /// @returns the low word of the scalar, 0 for null constants
    uint32_t GetU32() const { return GetU32BitValue(); }
    /// @returns the low word of the scalar as signed, 0 for null constants
    int32_t GetS32() const { return GetS32BitValue(); }
    /// @returns the scalar as a 32-bit float, 0 for null constants
    float GetFloat() const {
        float f = 0;
        uint32_t bits = GetU32BitValue();
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }
    /// @returns the scalar zero extended to 64 bits
    uint64_t GetZeroExtendedValue() const {
        uint64_t v = GetU32BitValue();
        if (words_.size() > 1) {
            v |= static_cast<uint64_t>(words_[1]) << 32;
        }
        return v;
    }
    /// @returns the composite components
    const std::vector<const Constant*>& GetComponents() const { return components_; }

  private:
    friend class tint::spirv::reader::spvc::IRContext;
    friend class tint::spirv::reader::spvc::ConstantManager;
    const Constant* Composite(Type::Kind k) const {
        return (kind_ == kComposite && type_ && type_->kind() == k) ? this : nullptr;
    }
    Kind kind_ = kNull;
    const Type* type_ = nullptr;
    std::vector<uint32_t> words_;
    std::vector<const Constant*> components_;
};

/// The array type, for spvtools::opt::analysis::Array::LengthInfo.
struct Array {
    using LengthInfo = Type::LengthInfo;
};

}  // namespace analysis

/// Maps result IDs to their defining instruction.
class DefUseManager {
  public:
    /// @param id a result ID
    /// @returns the defining instruction, or nullptr
    Instruction* GetDef(uint32_t id) const {
        auto it = defs_.find(id);
        return it != defs_.end() ? it->second : nullptr;
    }

  private:
    friend class IRContext;
    std::unordered_map<uint32_t, Instruction*> defs_;
};

/// The module types.
class TypeManager {
  public:
    /// @param id a type ID
    /// @returns the type, or nullptr
    const analysis::Type* GetType(uint32_t id) const {
        auto it = types_.find(id);
        return it != types_.end() ? it->second : nullptr;
    }
    /// @param type a type
    /// @returns the type ID, or 0
    uint32_t GetId(const analysis::Type* type) const {
        auto it = ids_.find(type);
        return it != ids_.end() ? it->second : 0u;
    }
    /// @returns the first (ID, type) pair, in ID order
    std::map<uint32_t, const analysis::Type*>::const_iterator begin() const {
        return types_.begin();
    }
    /// @returns the end of the (ID, type) pairs
    std::map<uint32_t, const analysis::Type*>::const_iterator end() const { return types_.end(); }

  private:
    friend class IRContext;
    std::vector<std::unique_ptr<analysis::Type>> owned_;
    std::map<uint32_t, const analysis::Type*> types_;
    std::unordered_map<const analysis::Type*, uint32_t> ids_;
};

/// The module constants.
class ConstantManager {
  public:
    /// @param id a result ID
    /// @returns the constant declared by @p id, or nullptr
    const analysis::Constant* FindDeclaredConstant(uint32_t id) const {
        auto it = constants_.find(id);
        return it != constants_.end() ? it->second : nullptr;
    }
    /// @param type a scalar type
    /// @param words the literal words
    /// @returns a constant of @p type with @p words
    const analysis::Constant* GetConstant(const analysis::Type* type,
                                          const std::vector<uint32_t>& words);

  private:
    friend class IRContext;
    std::vector<std::unique_ptr<analysis::Constant>> owned_;
    std::unordered_map<uint32_t, const analysis::Constant*> constants_;
};

/// The module decorations.
class DecorationManager {
  public:
    /// @param id a target ID
    /// @param include_linkage unused, as in spvtools only affects linkage decorations
    /// @returns the decoration instructions that target @p id
    std::vector<Instruction*> GetDecorationsFor(uint32_t id, bool include_linkage) const {
        (void)include_linkage;
        auto it = decorations_.find(id);
        return it != decorations_.end() ? it->second : std::vector<Instruction*>{};
    }

  private:
    friend class IRContext;
    std::unordered_map<uint32_t, std::vector<Instruction*>> decorations_;
};

/// Structured control flow order, as spvtools::opt::CFG::ComputeStructuredOrder.
class CFG {
  public:
    /// Computes the structured order of the blocks reachable from @p root, not walking past
    /// @p end.
    void ComputeStructuredOrder(Function* func,
                                BasicBlock* root,
                                BasicBlock* end,
                                std::list<BasicBlock*>* order) const;
};

/// The innermost loop of each block, as spvtools::opt::StructuredCFGAnalysis.
class StructuredCFGAnalysis {
  public:
    /// @param bb_id a block label ID
    /// @returns the header ID of the innermost loop that contains the block, or 0
    uint32_t ContainingLoop(uint32_t bb_id) const {
        auto it = containing_loop_.find(bb_id);
        return it != containing_loop_.end() ? it->second : 0u;
    }

  private:
    friend class IRContext;
    std::unordered_map<uint32_t, uint32_t> containing_loop_;
};

/// The SPIR-V module and its analyses.
class IRContext {
  public:
    ~IRContext();

    /// Checks @p words with spirv_cross::Parser, splits constant-indexed arrays of images and
    /// samplers into one variable per element (WGSL has no binding arrays), splits combined
    /// image samplers into separate image and sampler variables like
    /// spvtools::opt::SplitCombinedImageSamplerPass, and builds the module.
    /// @param words the SPIR-V binary
    /// @param count the number of words
    /// @param error set to the error message on failure
    /// @returns the module, or nullptr on failure
    static std::unique_ptr<IRContext> Build(const uint32_t* words,
                                            size_t count,
                                            std::string* error);

    /// Gives a sampler that shares a binding with an image in the same entry point the next
    /// free binding, like spvtools::opt::ResolveBindingConflictsPass.
    void ResolveBindingConflicts();

    /// Godot: sets the binding of every resource variable from @p plan (see
    /// Options::binding_plan) instead of resolving conflicts per stage.
    /// @returns false with @p error set if the plan has no binding for a variable
    bool ApplyBindingPlan(
        const std::function<std::optional<uint32_t>(uint32_t, uint32_t, uint32_t, bool)>& plan,
        std::string* error);

    /// @returns the module
    Module* module() { return &module_; }
    /// @returns OpExtension instructions
    std::list<Instruction>& extensions() { return module_.extensions_; }
    /// @returns OpExtInstImport instructions
    std::list<Instruction>& ext_inst_imports() { return module_.ext_inst_imports_; }
    /// @returns OpName and OpMemberName instructions
    std::list<Instruction>& debugs2() { return module_.debugs2_; }
    /// @returns type, constant, global variable and OpUndef instructions
    std::list<Instruction>& types_values() { return module_.types_values_; }
    /// @returns the first OpCapability
    std::list<Instruction>::iterator capability_begin() { return module_.capabilities_.begin(); }
    /// @returns the end of the OpCapability list
    std::list<Instruction>::iterator capability_end() { return module_.capabilities_.end(); }

    DefUseManager* get_def_use_mgr() { return &def_use_; }
    TypeManager* get_type_mgr() { return &types_; }
    ConstantManager* get_constant_mgr() { return &constants_; }
    DecorationManager* get_decoration_mgr() { return &decorations_; }
    CFG* cfg() { return &cfg_; }
    StructuredCFGAnalysis* GetStructuredCFGAnalysis() { return &structured_cfg_; }

    /// @param inst an instruction in a function body
    /// @returns the block of @p inst, or nullptr
    BasicBlock* get_instr_block(const Instruction* inst) const {
        auto it = inst_block_.find(inst);
        return it != inst_block_.end() ? it->second : nullptr;
    }
    /// @param id a block label ID or the result ID of an instruction in a function body
    /// @returns the block, or nullptr
    BasicBlock* get_instr_block(uint32_t id) const;

  private:
    IRContext() = default;
    void Populate(const std::vector<uint32_t>& words);
    void BuildTypes();
    void BuildConstants();
    void BuildStructuredCFG();

    Module module_;
    DefUseManager def_use_;
    TypeManager types_;
    ConstantManager constants_;
    DecorationManager decorations_;
    CFG cfg_;
    StructuredCFGAnalysis structured_cfg_;
    std::unordered_map<const Instruction*, BasicBlock*> inst_block_;
    std::unordered_map<uint32_t, BasicBlock*> label_block_;
    /// The element variables of each split array of handles
    std::vector<std::vector<uint32_t>> handle_array_groups_;
};

}  // namespace tint::spirv::reader::spvc

#endif  // SRC_TINT_LANG_SPIRV_READER_PARSER_SPIRV_CROSS_MODULE_H_
