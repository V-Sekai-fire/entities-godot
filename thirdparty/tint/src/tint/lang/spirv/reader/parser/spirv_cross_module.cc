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

// Godot: builds the reader's SPIR-V module (see spirv_cross_module.h). spirv_cross::Parser checks
// the binary; the instruction lists are then read straight from the words, so every instruction
// (OpPhi, merges, terminators, function-scope OpVariable) keeps its exact operands.

#define TINT_SPIRV_CROSS_MODULE_IMPL
#include "src/tint/lang/spirv/reader/parser/spirv_cross_module.h"

#include <algorithm>
#include <exception>
#include <unordered_set>

#include "spirv_parser.hpp"

namespace tint::spirv::reader::spvc {

namespace sc = SPIRV_CROSS_NAMESPACE;

namespace {

/// The word offset and word count of one instruction in a binary.
struct RawInst {
    size_t offset;
    uint32_t count;
    uint32_t opcode;
};

bool SplitWords(const std::vector<uint32_t>& w, std::vector<RawInst>* out, std::string* error) {
    if (w.size() < 5) {
        *error = "SPIR-V binary is too short";
        return false;
    }
    for (size_t i = 5; i < w.size();) {
        uint32_t count = w[i] >> 16;
        if (count == 0 || i + count > w.size()) {
            *error = "malformed SPIR-V instruction at word " + std::to_string(i);
            return false;
        }
        out->push_back(RawInst{i, count, w[i] & 0xffffu});
        i += count;
    }
    return true;
}

bool WordEndsString(uint32_t w) {
    return (w & 0xffu) == 0 || ((w >> 8) & 0xffu) == 0 || ((w >> 16) & 0xffu) == 0 ||
           (w >> 24) == 0;
}

std::vector<uint32_t> EncodeString(const std::string& s) {
    std::vector<uint32_t> out((s.size() + 4) / 4, 0u);
    std::memcpy(out.data(), s.data(), s.size());
    return out;
}

std::string DecodeString(const uint32_t* words, size_t count) {
    const char* str = reinterpret_cast<const char*>(words);
    return std::string(str, strnlen(str, count * sizeof(uint32_t)));
}

void Append(std::vector<uint32_t>& out, spv::Op op, const std::vector<uint32_t>& operands) {
    out.push_back((static_cast<uint32_t>(operands.size() + 1) << 16) | static_cast<uint32_t>(op));
    out.insert(out.end(), operands.begin(), operands.end());
}

/// The UniformConstant pointer types of a module being rewritten. A new pointer to a pointee that
/// the module already points to (later on) reuses that pointer type and moves its declaration up,
/// as two pointer types to one pointee make function arguments and parameters disagree.
struct UcPointers {
    std::unordered_map<uint32_t, uint32_t> declared;  // pointee -> first declared pointer type
    std::unordered_set<uint32_t> emitted;             // pointer types already in the output

    void Scan(const std::vector<uint32_t>& w, const std::vector<RawInst>& insts) {
        for (const auto& in : insts) {
            const uint32_t* o = &w[in.offset];
            if (in.opcode == spv::OpTypePointer && o[2] == spv::StorageClassUniformConstant &&
                !declared.count(o[3])) {
                declared[o[3]] = o[1];
            }
        }
    }
    /// @returns the pointer type to @p pointee, declared in @p out
    uint32_t Get(std::vector<uint32_t>& out, uint32_t& bound, uint32_t pointee) {
        auto it = declared.find(pointee);
        uint32_t id = it != declared.end() ? it->second : bound++;
        declared[pointee] = id;
        if (emitted.insert(id).second) {
            Append(out, spv::OpTypePointer, {id, spv::StorageClassUniformConstant, pointee});
        }
        return id;
    }
    /// @returns true if the existing declaration of pointer type @p id is still to be copied
    bool Copy(uint32_t id) { return emitted.insert(id).second; }
};

/// Splits combined image sampler variables (and pointer parameters) into an image variable and
/// a sampler variable, like spvtools::opt::SplitCombinedImageSamplerPass: each OpLoad of the
/// combined variable becomes a load of each part and an OpSampledImage with the original result
/// ID, decorations are copied to both parts, OpName gets `_image` / `_sampler` suffixes, and the
/// entry point interfaces list both parts.
bool SplitCombinedImageSamplers(std::vector<uint32_t>& w, std::string* error) {
    std::vector<RawInst> insts;
    if (!SplitWords(w, &insts, error)) {
        return false;
    }

    uint32_t sampler_type = 0;
    std::unordered_map<uint32_t, uint32_t> si_image;  // OpTypeSampledImage -> image type
    std::unordered_set<uint32_t> si_arrays;           // arrays of sampled images
    std::unordered_map<uint32_t, uint32_t> cptr_si;   // UniformConstant pointer -> sampled image
    std::unordered_set<uint32_t> combined;            // combined variables and parameters
    std::unordered_set<uint32_t> multisampled;        // multisampled image types
    for (const auto& in : insts) {
        const uint32_t* o = &w[in.offset];
        switch (static_cast<spv::Op>(in.opcode)) {
            case spv::OpTypeImage:
                if (in.count > 6 && o[6] != 0) {
                    multisampled.insert(o[1]);
                }
                break;
            case spv::OpTypeSampler:
                sampler_type = o[1];
                break;
            case spv::OpTypeSampledImage:
                si_image[o[1]] = o[2];
                break;
            case spv::OpTypeArray:
            case spv::OpTypeRuntimeArray:
                if (si_image.count(o[2]) || si_arrays.count(o[2])) {
                    si_arrays.insert(o[1]);
                }
                break;
            case spv::OpTypePointer:
                if (o[2] == spv::StorageClassUniformConstant) {
                    if (si_image.count(o[3])) {
                        cptr_si[o[1]] = o[3];
                    } else if (si_arrays.count(o[3])) {
                        *error = "arrays of combined image samplers are not supported";
                        return false;
                    }
                }
                break;
            case spv::OpVariable:
            case spv::OpFunctionParameter:
                if (cptr_si.count(o[1])) {
                    combined.insert(o[2]);
                } else if (in.opcode == spv::OpFunctionParameter && si_image.count(o[1])) {
                    *error = "sampled image function parameters passed by value are not supported";
                    return false;
                }
                break;
            default:
                break;
        }
    }
    if (combined.empty()) {
        return true;
    }

    uint32_t bound = w[3];
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> parts;  // combined -> image, sampler
    for (uint32_t id : combined) {
        uint32_t image = bound++;
        uint32_t sampler = bound++;
        parts[id] = {image, sampler};
    }

    std::vector<uint32_t> out(w.begin(), w.begin() + 5);
    UcPointers uc_ptr;
    uc_ptr.Scan(w, insts);
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> cptr_split;
    // Loads of multisampled combined image samplers: result ID -> image load result ID. A
    // multisampled image can only be fetched, so no OpSampledImage is made for it (tint does not
    // accept one); its OpImage becomes an OpCopyObject of the image.
    std::unordered_map<uint32_t, uint32_t> ms_loads;
    bool sampler_emitted = false;
    if (!sampler_type) {
        sampler_type = bound++;
    }

    for (const auto& in : insts) {
        const uint32_t* o = &w[in.offset];
        std::vector<uint32_t> ops(o + 1, o + in.count);
        auto copy = [&] { out.insert(out.end(), o, o + in.count); };
        auto is_combined = [&](uint32_t id) { return combined.count(id) != 0; };

        switch (static_cast<spv::Op>(in.opcode)) {
            case spv::OpTypeSampler:
                if (!sampler_emitted) {
                    copy();
                    sampler_emitted = true;
                }
                break;
            case spv::OpTypePointer: {
                uint32_t id = ops[0];
                if (!cptr_si.count(id)) {
                    if (ops[1] != spv::StorageClassUniformConstant || uc_ptr.Copy(id)) {
                        copy();
                    }
                    break;
                }
                if (!sampler_emitted) {
                    Append(out, spv::OpTypeSampler, {sampler_type});
                    sampler_emitted = true;
                }
                auto pointer_to = [&](uint32_t pointee) { return uc_ptr.Get(out, bound, pointee); };
                uint32_t image_ptr = pointer_to(si_image[cptr_si[id]]);
                uint32_t sampler_ptr = pointer_to(sampler_type);
                cptr_split[id] = {image_ptr, sampler_ptr};
                break;
            }
            case spv::OpTypeFunction: {
                std::vector<uint32_t> n{ops[0], ops[1]};
                for (size_t i = 2; i < ops.size(); i++) {
                    auto it = cptr_split.find(ops[i]);
                    if (it != cptr_split.end()) {
                        n.push_back(it->second.first);
                        n.push_back(it->second.second);
                    } else {
                        n.push_back(ops[i]);
                    }
                }
                Append(out, spv::OpTypeFunction, n);
                break;
            }
            case spv::OpVariable:
            case spv::OpFunctionParameter: {
                if (!is_combined(ops[1])) {
                    copy();
                    break;
                }
                auto [image_ptr, sampler_ptr] = cptr_split[ops[0]];
                auto [image, sampler] = parts[ops[1]];
                if (in.opcode == spv::OpVariable) {
                    Append(out, spv::OpVariable,
                           {sampler_ptr, sampler, spv::StorageClassUniformConstant});
                    Append(out, spv::OpVariable, {image_ptr, image, spv::StorageClassUniformConstant});
                } else {
                    Append(out, spv::OpFunctionParameter, {image_ptr, image});
                    Append(out, spv::OpFunctionParameter, {sampler_ptr, sampler});
                }
                break;
            }
            case spv::OpLoad: {
                if (!is_combined(ops[2])) {
                    copy();
                    break;
                }
                auto si = si_image.find(ops[0]);
                if (si == si_image.end()) {
                    *error = "combined image sampler loaded as a non sampled image type";
                    return false;
                }
                auto [image, sampler] = parts[ops[2]];
                uint32_t image_value = bound++;
                Append(out, spv::OpLoad, {si->second, image_value, image});
                if (multisampled.count(si->second)) {
                    ms_loads[ops[1]] = image_value;
                    break;
                }
                uint32_t sampler_value = bound++;
                Append(out, spv::OpLoad, {sampler_type, sampler_value, sampler});
                Append(out, spv::OpSampledImage, {ops[0], ops[1], image_value, sampler_value});
                break;
            }
            case spv::OpImage: {
                auto it = ms_loads.find(ops[2]);
                if (it == ms_loads.end()) {
                    copy();
                    break;
                }
                Append(out, spv::OpCopyObject, {ops[0], ops[1], it->second});
                break;
            }
            case spv::OpFunctionCall: {
                std::vector<uint32_t> n{ops[0], ops[1], ops[2]};
                for (size_t i = 3; i < ops.size(); i++) {
                    if (is_combined(ops[i])) {
                        n.push_back(parts[ops[i]].first);
                        n.push_back(parts[ops[i]].second);
                    } else {
                        n.push_back(ops[i]);
                    }
                }
                Append(out, spv::OpFunctionCall, n);
                break;
            }
            case spv::OpDecorate: {
                if (!is_combined(ops[0])) {
                    copy();
                    break;
                }
                auto [image, sampler] = parts[ops[0]];
                ops[0] = image;
                Append(out, spv::OpDecorate, ops);
                ops[0] = sampler;
                Append(out, spv::OpDecorate, ops);
                break;
            }
            case spv::OpName: {
                if (!is_combined(ops[0])) {
                    copy();
                    break;
                }
                auto [image, sampler] = parts[ops[0]];
                std::string name = DecodeString(o + 2, in.count - 2);
                auto image_name = EncodeString(name + "_image");
                auto sampler_name = EncodeString(name + "_sampler");
                image_name.insert(image_name.begin(), image);
                sampler_name.insert(sampler_name.begin(), sampler);
                Append(out, spv::OpName, image_name);
                Append(out, spv::OpName, sampler_name);
                break;
            }
            case spv::OpEntryPoint: {
                // Model, function, name string, then the interface IDs.
                size_t first = 2;
                while (first < ops.size() && !WordEndsString(ops[first])) {
                    first++;
                }
                first++;
                std::vector<uint32_t> appended;
                for (size_t i = first; i < ops.size(); i++) {
                    if (is_combined(ops[i])) {
                        appended.push_back(parts[ops[i]].second);
                        ops[i] = parts[ops[i]].first;
                    }
                }
                ops.insert(ops.end(), appended.begin(), appended.end());
                Append(out, spv::OpEntryPoint, ops);
                break;
            }
            case spv::OpCopyObject:
            case spv::OpAccessChain:
            case spv::OpInBoundsAccessChain:
            case spv::OpPtrAccessChain:
            case spv::OpStore:
            case spv::OpDecorateId:
            case spv::OpGroupDecorate:
                for (size_t i = 0; i < ops.size(); i++) {
                    if (is_combined(ops[i])) {
                        *error = "unhandled use of a combined image sampler (opcode " +
                                 std::to_string(in.opcode) + ")";
                        return false;
                    }
                }
                copy();
                break;
            default:
                copy();
                break;
        }
    }
    out[3] = bound;
    w = std::move(out);
    return true;
}

/// Splits module-scope arrays of images or samplers (WGSL has no binding arrays) into one
/// variable per element. Every element keeps the array's descriptor set and binding;
/// IRContext::ResolveBindingConflicts then gives element i the binding `binding + i` and moves
/// the later bindings up, the layout the Godot WebGPU driver uses for split binding arrays.
/// Only constant indices are accepted.
/// @param groups receives the element variable IDs of each split array
bool SplitHandleArrays(std::vector<uint32_t>& w,
                       std::vector<std::vector<uint32_t>>* groups,
                       std::string* error) {
    std::vector<RawInst> insts;
    if (!SplitWords(w, &insts, error)) {
        return false;
    }

    std::unordered_set<uint32_t> handles;                   // image and sampler types
    std::unordered_map<uint32_t, uint32_t> int_constants;   // 32-bit integer constant values
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> arrays;  // array -> elem, count
    std::unordered_map<uint32_t, uint32_t> arr_ptr;         // UC pointer to array -> array
    std::unordered_map<uint32_t, uint32_t> vars;            // array variable -> array
    std::unordered_map<uint32_t, uint32_t> int_types;       // 32-bit int type IDs
    for (const auto& in : insts) {
        const uint32_t* o = &w[in.offset];
        switch (static_cast<spv::Op>(in.opcode)) {
            case spv::OpTypeImage:
            case spv::OpTypeSampler:
                handles.insert(o[1]);
                break;
            case spv::OpTypeInt:
                if (o[2] == 32) {
                    int_types[o[1]] = 1;
                }
                break;
            case spv::OpConstant:
                if (in.count == 4 && int_types.count(o[1])) {
                    int_constants[o[2]] = o[3];
                }
                break;
            case spv::OpTypeArray:
                if (handles.count(o[2])) {
                    auto len = int_constants.find(o[3]);
                    if (len == int_constants.end()) {
                        *error = "arrays of handle types with a specialized length are not supported";
                        return false;
                    }
                    arrays[o[1]] = {o[2], len->second};
                }
                break;
            case spv::OpTypePointer:
                if (arrays.count(o[3])) {
                    if (o[2] != spv::StorageClassUniformConstant) {
                        *error = "arrays of handle types are not supported";
                        return false;
                    }
                    arr_ptr[o[1]] = o[3];
                }
                break;
            case spv::OpVariable:
                if (arr_ptr.count(o[1])) {
                    vars[o[2]] = arr_ptr[o[1]];
                }
                break;
            case spv::OpFunctionParameter:
                if (arr_ptr.count(o[1]) || arrays.count(o[1])) {
                    *error = "arrays of handle types as function parameters are not supported";
                    return false;
                }
                break;
            default:
                break;
        }
    }
    if (vars.empty()) {
        return true;
    }

    uint32_t bound = w[3];
    std::unordered_map<uint32_t, std::vector<uint32_t>> elements;  // array variable -> elements
    // Declaration order of the array variables, for deterministic IDs.
    for (const auto& in : insts) {
        const uint32_t* o = &w[in.offset];
        if (in.opcode == spv::OpVariable && vars.count(o[2])) {
            auto& el = elements[o[2]];
            for (uint32_t i = 0; i < arrays[vars[o[2]]].second; i++) {
                el.push_back(bound++);
            }
            groups->push_back(el);
        }
    }

    std::vector<uint32_t> out(w.begin(), w.begin() + 5);
    UcPointers uc_ptr;
    uc_ptr.Scan(w, insts);
    std::unordered_map<uint32_t, uint32_t> elem_ptr;   // array pointer -> element pointer
    std::unordered_map<uint32_t, uint32_t> alias;      // access chain result -> element variable

    // A run-time index: the access chain is dropped, and so are the loads of it and the
    // OpSampledImage, OpImage and OpCopyObject made from those loads. The instruction that
    // finally uses such a value (a sample, fetch, query or write) becomes an OpSwitch on the
    // index with one case per element, each redoing the dropped chain on its element and the
    // instruction itself, merged with an OpPhi. The default case is the last element, so an
    // index past the end reads it.
    struct Dyn {
        uint32_t var;                              // the array variable
        uint32_t index;                            // the index ID
        std::vector<std::vector<uint32_t>> chain;  // dropped instructions after the access chain
    };
    std::unordered_map<uint32_t, Dyn> dyn;         // pointer or value ID -> its chain
    std::unordered_map<uint32_t, uint32_t> label_rename;  // block label -> label of its tail
    uint32_t block = 0;           // label of the block being copied
    uint32_t block_tail = 0;      // label of the current tail of that block
    auto any_dyn_operand = [&](const std::vector<uint32_t>& ops, size_t first) {
        for (size_t i = first; i < ops.size(); i++) {
            if (dyn.count(ops[i])) {
                return true;
            }
        }
        return false;
    };

    auto emit_switch = [&](uint32_t opcode, const std::vector<uint32_t>& ops, bool has_result,
                           size_t pos) {
        const Dyn& d = dyn[ops[pos]];
        const auto& el = elements[d.var];
        const uint32_t merge = bound++;
        std::vector<uint32_t> case_labels;
        for (size_t k = 0; k < el.size(); k++) {
            case_labels.push_back(bound++);
        }
        Append(out, spv::OpSelectionMerge, {merge, 0u});
        std::vector<uint32_t> sw{d.index, case_labels.back()};
        for (size_t k = 0; k + 1 < el.size(); k++) {
            sw.push_back(static_cast<uint32_t>(k));
            sw.push_back(case_labels[k]);
        }
        Append(out, spv::OpSwitch, sw);
        std::vector<uint32_t> phi;
        if (has_result) {
            phi = {ops[0], ops[1]};
        }
        for (size_t k = 0; k < el.size(); k++) {
            Append(out, spv::OpLabel, {case_labels[k]});
            // Redo the chain on element k with fresh result IDs.
            std::unordered_map<uint32_t, uint32_t> fresh;
            auto sub = [&](uint32_t id) {
                auto it = fresh.find(id);
                return it != fresh.end() ? it->second : id;
            };
                for (const auto& c : d.chain) {
                    if (c[0] == spv::OpLoad) {  // opcode, type, result, pointer, ...
                        uint32_t r = bound++;
                        fresh[c[2]] = r;
                        Append(out, spv::OpLoad, {c[1], r, el[k]});
                    } else {  // opcode, type, result, operands...
                        std::vector<uint32_t> n(c.begin() + 1, c.end());
                        uint32_t r = bound++;
                        fresh[n[1]] = r;
                        n[1] = r;
                        for (size_t j = 2; j < n.size(); j++) {
                            n[j] = sub(n[j]);
                        }
                        Append(out, static_cast<spv::Op>(c[0]), n);
                    }
                }
                std::vector<uint32_t> n = ops;
                n[pos] = d.chain.empty() ? el[k] : sub(n[pos]);
                if (has_result) {
                    uint32_t r = bound++;
                    n[1] = r;
                    phi.push_back(r);
                    phi.push_back(case_labels[k]);
                }
                Append(out, static_cast<spv::Op>(opcode), n);
                Append(out, spv::OpBranch, {merge});
            }
            Append(out, spv::OpLabel, {merge});
            if (has_result) {
                Append(out, spv::OpPhi, phi);
            }
            block_tail = merge;
            label_rename[block] = merge;
    };

    for (const auto& in : insts) {
        const uint32_t* o = &w[in.offset];
        std::vector<uint32_t> ops(o + 1, o + in.count);
        auto copy = [&] { out.insert(out.end(), o, o + in.count); };
        switch (static_cast<spv::Op>(in.opcode)) {
            case spv::OpTypeArray:
                if (!arrays.count(ops[0])) {
                    copy();
                }
                break;
            case spv::OpTypePointer: {
                if (!arr_ptr.count(ops[0])) {
                    if (ops[1] != spv::StorageClassUniformConstant || uc_ptr.Copy(ops[0])) {
                        copy();
                    }
                    break;
                }
                elem_ptr[ops[0]] = uc_ptr.Get(out, bound, arrays[arr_ptr[ops[0]]].first);
                break;
            }
            case spv::OpVariable:
                if (!vars.count(ops[1])) {
                    copy();
                    break;
                }
                for (uint32_t el : elements[ops[1]]) {
                    Append(out, spv::OpVariable,
                           {elem_ptr[ops[0]], el, spv::StorageClassUniformConstant});
                }
                break;
            case spv::OpDecorate:
                if (!vars.count(ops[0])) {
                    copy();
                    break;
                }
                for (uint32_t el : elements[ops[0]]) {
                    ops[0] = el;
                    Append(out, spv::OpDecorate, ops);
                }
                break;
            case spv::OpName: {
                if (!vars.count(ops[0])) {
                    copy();
                    break;
                }
                std::string name = DecodeString(o + 2, in.count - 2);
                const auto& el = elements[ops[0]];
                for (size_t i = 0; i < el.size(); i++) {
                    auto n = EncodeString(name + "_" + std::to_string(i));
                    n.insert(n.begin(), el[i]);
                    Append(out, spv::OpName, n);
                }
                break;
            }
            case spv::OpEntryPoint: {
                size_t first = 2;
                while (first < ops.size() && !WordEndsString(ops[first])) {
                    first++;
                }
                first++;
                std::vector<uint32_t> n(ops.begin(), ops.begin() + std::min(first, ops.size()));
                for (size_t i = first; i < ops.size(); i++) {
                    if (vars.count(ops[i])) {
                        const auto& el = elements[ops[i]];
                        n.insert(n.end(), el.begin(), el.end());
                    } else {
                        n.push_back(ops[i]);
                    }
                }
                Append(out, spv::OpEntryPoint, n);
                break;
            }
            case spv::OpAccessChain:
            case spv::OpInBoundsAccessChain: {
                if (!vars.count(ops[2])) {
                    copy();
                    break;
                }
                if (ops.size() != 4) {
                    *error = "arrays of handle types accessed with more than one index";
                    return false;
                }
                auto idx = int_constants.find(ops[3]);
                if (idx == int_constants.end()) {
                    dyn[ops[1]] = Dyn{ops[2], ops[3], {}};
                    break;
                }
                const auto& el = elements[ops[2]];
                if (idx->second >= el.size()) {
                    *error = "array of handle types indexed out of bounds";
                    return false;
                }
                alias[ops[1]] = el[idx->second];
                break;
            }
            case spv::OpLabel:
                block = block_tail = ops[0];
                copy();
                break;
            case spv::OpLoopMerge:
                if (block_tail != block) {
                    *error = "an array of handle types indexed with a non-constant in a loop header "
                             "is not supported";
                    return false;
                }
                copy();
                break;
            case spv::OpLoad:
            case spv::OpImageTexelPointer: {
                if (in.opcode == spv::OpLoad && dyn.count(ops[2])) {
                    Dyn d = dyn[ops[2]];
                    d.chain.push_back(ops);
                    d.chain.back().insert(d.chain.back().begin(), in.opcode);
                    dyn[ops[1]] = std::move(d);
                    break;
                }
                if (dyn.count(ops[2])) {
                    *error = "texel pointers into an array of handle types indexed with a "
                             "non-constant are not supported";
                    return false;
                }
                auto it = alias.find(ops[2]);
                if (it != alias.end()) {
                    ops[2] = it->second;
                    Append(out, static_cast<spv::Op>(in.opcode), ops);
                } else if (vars.count(ops[2])) {
                    *error = "arrays of handle types loaded as a whole are not supported";
                    return false;
                } else {
                    copy();
                }
                break;
            }
            case spv::OpCopyObject:
            case spv::OpFunctionCall:
                if ((in.opcode == spv::OpCopyObject) && dyn.count(ops[2])) {
                    Dyn d = dyn[ops[2]];
                    d.chain.push_back(ops);
                    d.chain.back().insert(d.chain.back().begin(), in.opcode);
                    dyn[ops[1]] = std::move(d);
                    break;
                }
                if (in.opcode == spv::OpFunctionCall && any_dyn_operand(ops, 3)) {
                    // A run-time element passed to a function: call it once per element.
                    size_t pos = 0;
                    for (size_t i = 3; i < ops.size(); i++) {
                        if (dyn.count(ops[i])) {
                            if (pos) {
                                *error = "a function call with two arrays of handle types indexed "
                                         "with a non-constant is not supported";
                                return false;
                            }
                            pos = i;
                        }
                    }
                    emit_switch(in.opcode, ops, true, pos);
                    break;
                }
                [[fallthrough]];
            case spv::OpStore:
            case spv::OpPtrAccessChain:
            case spv::OpCopyMemory:
                // Any other use of an array variable or of an element pointer is refused.
                for (size_t i = 0; i < ops.size(); i++) {
                    if (vars.count(ops[i]) || alias.count(ops[i]) || dyn.count(ops[i])) {
                        *error = "unhandled use of an array of handle types (opcode " +
                                 std::to_string(in.opcode) + ")";
                        return false;
                    }
                }
                copy();
                break;
            default: {
                bool has_result = false;
                bool has_type = false;
                spv::HasResultAndType(static_cast<spv::Op>(in.opcode), &has_result, &has_type);
                const size_t first_in = (has_type ? 1u : 0u) + (has_result ? 1u : 0u);
                if (!any_dyn_operand(ops, 0)) {
                    copy();
                    break;
                }
                if ((in.opcode == spv::OpSampledImage || in.opcode == spv::OpImage) &&
                    dyn.count(ops[2]) && !any_dyn_operand(ops, 3)) {
                    Dyn d = dyn[ops[2]];
                    d.chain.push_back(ops);
                    d.chain.back().insert(d.chain.back().begin(), in.opcode);
                    dyn[ops[1]] = std::move(d);
                    break;
                }
                if (ops.size() <= first_in || !dyn.count(ops[first_in]) ||
                    any_dyn_operand(ops, first_in + 1) || (has_type && dyn.count(ops[0]))) {
                    *error = "unhandled use of an array of handle types indexed with a "
                             "non-constant (opcode " + std::to_string(in.opcode) + ")";
                    return false;
                }
                emit_switch(in.opcode, ops, has_result, first_in);
                break;
            }
        }
    }
    // A branch from a split block now comes from its tail: rename OpPhi parents.
    {
        std::vector<RawInst> done;
        std::string unused;
        SplitWords(out, &done, &unused);
        for (const auto& in : done) {
            if (in.opcode != spv::OpPhi) {
                continue;
            }
            for (uint32_t i = 4; i < in.count; i += 2) {
                auto it = label_rename.find(out[in.offset + i]);
                if (it != label_rename.end()) {
                    out[in.offset + i] = it->second;
                }
            }
        }
    }
    out[3] = bound;
    w = std::move(out);
    return true;
}

/// @returns the in-operand indices of an instruction that can hold a pointer to a module-scope
/// resource variable.
template <typename F>
void ForEachPointerOperand(const Instruction& inst, F&& f) {
    switch (static_cast<spv::Op>(inst.OpcodeWord())) {
        case spv::OpLoad:
        case spv::OpStore:
        case spv::OpAccessChain:
        case spv::OpInBoundsAccessChain:
        case spv::OpPtrAccessChain:
        case spv::OpCopyObject:
        case spv::OpArrayLength:
        case spv::OpImageTexelPointer:
        case spv::OpAtomicLoad:
        case spv::OpAtomicStore:
        case spv::OpAtomicExchange:
        case spv::OpAtomicCompareExchange:
        case spv::OpAtomicIIncrement:
        case spv::OpAtomicIDecrement:
        case spv::OpAtomicIAdd:
        case spv::OpAtomicISub:
        case spv::OpAtomicSMin:
        case spv::OpAtomicUMin:
        case spv::OpAtomicSMax:
        case spv::OpAtomicUMax:
        case spv::OpAtomicAnd:
        case spv::OpAtomicOr:
        case spv::OpAtomicXor:
            if (inst.NumInOperands() > 0) {
                f(inst.GetSingleWordInOperand(0));
            }
            break;
        case spv::OpCopyMemory:
            f(inst.GetSingleWordInOperand(0));
            f(inst.GetSingleWordInOperand(1));
            break;
        case spv::OpFunctionCall:
            for (uint32_t i = 1; i < inst.NumInOperands(); i++) {
                f(inst.GetSingleWordInOperand(i));
            }
            break;
        default:
            break;
    }
}

}  // namespace

std::string Operand::AsString() const {
    return DecodeString(words.data(), words.size());
}

Instruction::Instruction(std::vector<uint32_t> words) : words_(std::move(words)) {
    opcode_ = words_.empty() ? 0u : (words_[0] & 0xffffu);
    spv::HasResultAndType(static_cast<spv::Op>(opcode_), &has_result_, &has_type_);

    const uint32_t n = static_cast<uint32_t>(words_.size());
    uint32_t idx = 1;
    auto add = [&](uint32_t len) {
        len = std::min(len, n - idx);
        operands_.push_back({idx, len});
        idx += len;
    };
    auto add_string = [&] {
        uint32_t end = idx;
        while (end < n) {
            if (WordEndsString(words_[end++])) {
                break;
            }
        }
        add(end - idx);
    };
    switch (static_cast<spv::Op>(opcode_)) {
        case spv::OpExtension:
        case spv::OpSourceExtension:
        case spv::OpModuleProcessed:
            add_string();
            break;
        case spv::OpExtInstImport:
        case spv::OpString:
        case spv::OpName:
            add(1);
            add_string();
            break;
        case spv::OpMemberName:
            add(1);
            add(1);
            add_string();
            break;
        case spv::OpEntryPoint:
            add(1);
            add(1);
            add_string();
            break;
        case spv::OpSource:
            add(1);
            add(1);
            if (idx < n) {
                add(1);
            }
            if (idx < n) {
                add_string();
            }
            break;
        case spv::OpDecorateString:
            add(1);
            add(1);
            while (idx < n) {
                add_string();
            }
            break;
        case spv::OpMemberDecorateString:
            add(1);
            add(1);
            add(1);
            while (idx < n) {
                add_string();
            }
            break;
        case spv::OpDecorate:
            if (n > 3 && words_[2] == spv::DecorationLinkageAttributes) {
                add(1);
                add(1);
                add_string();
            }
            break;
        default:
            break;
    }
    while (idx < n) {
        add(1);
    }
}

Operand Instruction::GetOperand(uint32_t i) const {
    const auto& [first, count] = operands_[i];
    return Operand{std::vector<uint32_t>(words_.begin() + first, words_.begin() + first + count)};
}

Instruction* BasicBlock::GetMergeInst() const {
    if (insts_.size() < 2) {
        return nullptr;
    }
    auto it = std::prev(insts_.end(), 2);
    auto op = static_cast<spv::Op>(it->OpcodeWord());
    if (op == spv::OpSelectionMerge || op == spv::OpLoopMerge) {
        return &*it;
    }
    return nullptr;
}

Instruction* BasicBlock::GetLoopMergeInst() const {
    auto* inst = GetMergeInst();
    if (inst && static_cast<spv::Op>(inst->OpcodeWord()) == spv::OpLoopMerge) {
        return inst;
    }
    return nullptr;
}

Instruction* BasicBlock::terminator() const {
    return insts_.empty() ? nullptr : &insts_.back();
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

BasicBlock* Function::FindBlock(uint32_t id) const {
    auto iter = block_by_id_.find(id);
    return iter != block_by_id_.end() ? iter->second : nullptr;
}

void CFG::ComputeStructuredOrder(Function* func,
                                 BasicBlock* root,
                                 BasicBlock* end,
                                 std::list<BasicBlock*>* order) const {
    // Structured successors: the merge block, then the continue target, then the branch targets.
    auto successors = [&](const BasicBlock* b) {
        std::vector<BasicBlock*> out;
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
        BasicBlock* block;
        std::vector<BasicBlock*> succs;
        size_t next;
    };
    std::unordered_set<BasicBlock*> visited{root};
    std::vector<Entry> stack;
    stack.push_back({root, successors(root), 0});
    while (!stack.empty()) {
        auto& top = stack.back();
        if (top.block == end || top.next == top.succs.size()) {
            order->push_front(top.block);
            stack.pop_back();
            continue;
        }
        BasicBlock* child = top.succs[top.next++];
        if (visited.insert(child).second) {
            stack.push_back({child, successors(child), 0});
        }
    }
}

const analysis::Constant* ConstantManager::GetConstant(const analysis::Type* type,
                                                       const std::vector<uint32_t>& words) {
    auto c = std::make_unique<analysis::Constant>();
    c->type_ = type;
    c->words_ = words;
    if (words.empty()) {
        c->kind_ = analysis::Constant::kNull;
    } else if (type && type->kind() == analysis::Type::kBool) {
        c->kind_ = analysis::Constant::kBool;
    } else if (type && type->kind() == analysis::Type::kFloat) {
        c->kind_ = analysis::Constant::kFloat;
    } else {
        c->kind_ = analysis::Constant::kInt;
    }
    owned_.push_back(std::move(c));
    return owned_.back().get();
}

IRContext::~IRContext() = default;

BasicBlock* IRContext::get_instr_block(uint32_t id) const {
    auto it = label_block_.find(id);
    if (it != label_block_.end()) {
        return it->second;
    }
    return get_instr_block(def_use_.GetDef(id));
}

std::unique_ptr<IRContext> IRContext::Build(const uint32_t* data, size_t count, std::string* error) {
    std::vector<uint32_t> words(data, data + count);
#ifdef SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS
    // Built without exceptions: SPIRV-Cross aborts on a malformed module instead of throwing.
    {
        sc::Parser parser(data, count);
        parser.parse();
    }
#else
    try {
        // Let SPIRV-Cross check the module structure.
        sc::Parser parser(data, count);
        parser.parse();
    } catch (const std::exception& e) {
        *error = std::string("SPIRV-Cross failed to parse the module: ") + e.what();
        return nullptr;
    }
#endif
    std::vector<std::vector<uint32_t>> groups;
    if (!SplitHandleArrays(words, &groups, error)) {
        return nullptr;
    }
    if (!SplitCombinedImageSamplers(words, error)) {
        return nullptr;
    }
    std::vector<RawInst> insts;
    if (!SplitWords(words, &insts, error)) {
        return nullptr;
    }
    std::unique_ptr<IRContext> ctx(new IRContext());
    ctx->handle_array_groups_ = std::move(groups);
    ctx->Populate(words);
    ctx->BuildTypes();
    ctx->BuildConstants();
    ctx->BuildStructuredCFG();
    return ctx;
}

void IRContext::Populate(const std::vector<uint32_t>& w) {
    std::vector<RawInst> insts;
    std::string unused;
    SplitWords(w, &insts, &unused);

    Function* func = nullptr;
    BasicBlock* block = nullptr;
    for (const auto& in : insts) {
        Instruction inst(std::vector<uint32_t>(w.begin() + in.offset,
                                               w.begin() + in.offset + in.count));
        auto op = static_cast<spv::Op>(in.opcode);
        if (op == spv::OpLine || op == spv::OpNoLine) {
            continue;  // Debug line info is not part of the instruction stream.
        }
        if (func) {
            switch (op) {
                case spv::OpFunctionParameter:
                    func->params_.push_back(std::move(inst));
                    break;
                case spv::OpLabel: {
                    auto bb = std::make_unique<BasicBlock>();
                    bb->id_ = inst.result_id();
                    bb->parent_ = func;
                    block = bb.get();
                    func->blocks_.push_back(std::move(bb));
                    break;
                }
                case spv::OpFunctionEnd:
                    func = nullptr;
                    block = nullptr;
                    break;
                default:
                    if (block) {
                        block->insts_.push_back(std::move(inst));
                    }
                    break;
            }
            continue;
        }
        switch (op) {
            case spv::OpCapability:
                module_.capabilities_.push_back(std::move(inst));
                break;
            case spv::OpExtension:
                module_.extensions_.push_back(std::move(inst));
                break;
            case spv::OpExtInstImport:
                module_.ext_inst_imports_.push_back(std::move(inst));
                break;
            case spv::OpMemoryModel:
                module_.memory_model_.push_back(std::move(inst));
                break;
            case spv::OpEntryPoint:
                module_.entry_points_.push_back(std::move(inst));
                break;
            case spv::OpExecutionMode:
            case spv::OpExecutionModeId:
                module_.execution_modes_.push_back(std::move(inst));
                break;
            case spv::OpString:
            case spv::OpSource:
            case spv::OpSourceExtension:
            case spv::OpSourceContinued:
                module_.debugs1_.push_back(std::move(inst));
                break;
            case spv::OpName:
            case spv::OpMemberName:
                module_.debugs2_.push_back(std::move(inst));
                break;
            case spv::OpModuleProcessed:
                module_.debugs3_.push_back(std::move(inst));
                break;
            case spv::OpDecorate:
            case spv::OpDecorateId:
            case spv::OpDecorateString:
            case spv::OpMemberDecorate:
            case spv::OpMemberDecorateString:
            case spv::OpDecorationGroup:
            case spv::OpGroupDecorate:
            case spv::OpGroupMemberDecorate:
                module_.annotations_.push_back(std::move(inst));
                break;
            case spv::OpExtInst:
                module_.ext_inst_debuginfo_.push_back(std::move(inst));
                break;
            case spv::OpFunction:
                module_.functions_.emplace_back(std::move(inst));
                func = &module_.functions_.back();
                break;
            default:
                module_.types_values_.push_back(std::move(inst));
                break;
        }
    }

    // Index the definitions. The lists are not changed after this point.
    auto def = [&](Instruction& inst) {
        if (uint32_t id = inst.result_id()) {
            def_use_.defs_[id] = &inst;
        }
    };
    for (auto& inst : module_.ext_inst_imports_) {
        def(inst);
    }
    for (auto& inst : module_.types_values_) {
        def(inst);
    }
    for (auto& inst : module_.debugs1_) {
        def(inst);
    }
    for (auto& f : module_.functions_) {
        def(f.def_);
        for (auto& p : f.params_) {
            def(p);
        }
        for (auto& bb : f.blocks_) {
            f.block_by_id_[bb->id_] = bb.get();
            label_block_[bb->id_] = bb.get();
            for (auto& inst : bb->insts_) {
                def(inst);
                inst_block_[&inst] = bb.get();
            }
        }
    }
    for (auto& inst : module_.annotations_) {
        switch (static_cast<spv::Op>(inst.OpcodeWord())) {
            case spv::OpDecorate:
            case spv::OpDecorateId:
            case spv::OpDecorateString:
            case spv::OpMemberDecorate:
            case spv::OpMemberDecorateString:
                decorations_.decorations_[inst.GetSingleWordInOperand(0)].push_back(&inst);
                break;
            default:
                break;
        }
    }
}

void IRContext::BuildTypes() {
    using T = analysis::Type;
    std::unordered_map<uint32_t, T*> by_id;
    std::vector<std::pair<Instruction*, T*>> created;
    for (auto& inst : module_.types_values_) {
        T::Kind kind;
        switch (static_cast<spv::Op>(inst.OpcodeWord())) {
            case spv::OpTypeVoid:
                kind = T::kVoid;
                break;
            case spv::OpTypeBool:
                kind = T::kBool;
                break;
            case spv::OpTypeInt:
                kind = T::kInteger;
                break;
            case spv::OpTypeFloat:
                kind = T::kFloat;
                break;
            case spv::OpTypeVector:
                kind = T::kVector;
                break;
            case spv::OpTypeMatrix:
                kind = T::kMatrix;
                break;
            case spv::OpTypeImage:
                kind = T::kImage;
                break;
            case spv::OpTypeSampler:
                kind = T::kSampler;
                break;
            case spv::OpTypeSampledImage:
                kind = T::kSampledImage;
                break;
            case spv::OpTypeArray:
                kind = T::kArray;
                break;
            case spv::OpTypeRuntimeArray:
                kind = T::kRuntimeArray;
                break;
            case spv::OpTypeStruct:
                kind = T::kStruct;
                break;
            case spv::OpTypePointer:
                kind = T::kPointer;
                break;
            case spv::OpTypeFunction:
                kind = T::kFunction;
                break;
            case spv::OpTypeOpaque:
            case spv::OpTypeEvent:
            case spv::OpTypeDeviceEvent:
            case spv::OpTypeReserveId:
            case spv::OpTypeQueue:
            case spv::OpTypePipe:
            case spv::OpTypeForwardPointer:
            case spv::OpTypeAccelerationStructureKHR:
            case spv::OpTypeRayQueryKHR:
                kind = T::kUnsupported;
                break;
            default:
                continue;  // Not a type.
        }
        if (inst.OpcodeWord() == spv::OpTypeForwardPointer) {
            continue;  // Declares no new type.
        }
        auto t = std::make_unique<T>();
        t->kind_ = kind;
        t->str_ = "type %" + std::to_string(inst.result_id()) + " (opcode " +
                  std::to_string(inst.OpcodeWord()) + ")";
        by_id[inst.result_id()] = t.get();
        created.push_back({&inst, t.get()});
        types_.types_[inst.result_id()] = t.get();
        types_.ids_[t.get()] = inst.result_id();
        types_.owned_.push_back(std::move(t));
    }

    auto get = [&](uint32_t id) -> const T* {
        auto it = by_id.find(id);
        return it != by_id.end() ? it->second : nullptr;
    };
    for (auto& [inst, t] : created) {
        const auto& in = *inst;
        auto op = [&](uint32_t i) { return in.GetSingleWordInOperand(i); };
        switch (t->kind_) {
            case T::kInteger:
                t->width_ = op(0);
                t->signed_ = op(1) != 0;
                break;
            case T::kFloat:
                t->width_ = op(0);
                break;
            case T::kVector:
            case T::kMatrix:
                t->element_type_ = get(op(0));
                t->element_count_ = op(1);
                break;
            case T::kImage:
                t->element_type_ = get(op(0));
                t->dim_ = op(1);
                t->depth_ = op(2);
                t->arrayed_ = op(3) != 0;
                t->multisampled_ = op(4) != 0;
                t->sampled_ = op(5);
                t->format_ = op(6);
                // Like spvtools::opt::analysis::Image, an absent access qualifier reads as
                // ReadOnly.
                t->access_ =
                    in.NumInOperands() > 7 ? op(7) : uint32_t(spv::AccessQualifierReadOnly);
                break;
            case T::kSampledImage:
                t->element_type_ = get(op(0));
                break;
            case T::kArray: {
                t->element_type_ = get(op(0));
                uint32_t len_id = op(1);
                t->length_info_.id = len_id;
                auto* len = def_use_.GetDef(len_id);
                bool has_spec_id = false;
                uint32_t spec_id = 0;
                for (auto* d : decorations_.GetDecorationsFor(len_id, false)) {
                    if (d->OpcodeWord() == spv::OpDecorate &&
                        d->GetSingleWordInOperand(1) == spv::DecorationSpecId) {
                        has_spec_id = true;
                        spec_id = d->GetSingleWordInOperand(2);
                    }
                }
                if (len && len->OpcodeWord() == spv::OpConstant) {
                    t->length_info_.words.push_back(T::LengthInfo::kConstant);
                    for (uint32_t i = 0; i < len->NumInOperands(); i++) {
                        t->length_info_.words.push_back(len->GetSingleWordInOperand(i));
                    }
                } else if (len && len->OpcodeWord() == spv::OpSpecConstant && has_spec_id) {
                    t->length_info_.words = {T::LengthInfo::kConstantWithSpecId, spec_id};
                } else {
                    t->length_info_.words = {T::LengthInfo::kDefiningId, len_id};
                }
                break;
            }
            case T::kRuntimeArray:
                t->element_type_ = get(op(0));
                break;
            case T::kStruct:
                for (uint32_t i = 0; i < in.NumInOperands(); i++) {
                    t->element_types_.push_back(get(op(i)));
                }
                break;
            case T::kPointer:
                t->storage_ = op(0);
                t->element_type_ = get(op(1));
                break;
            default:
                break;
        }
    }

    // Type decorations, as spvtools::opt::analysis::TypeManager::AttachDecoration.
    for (auto& inst : module_.annotations_) {
        auto opcode = static_cast<spv::Op>(inst.OpcodeWord());
        if (opcode != spv::OpDecorate && opcode != spv::OpMemberDecorate) {
            continue;
        }
        auto it = by_id.find(inst.GetSingleWordInOperand(0));
        if (it == by_id.end()) {
            continue;
        }
        const auto& words = inst.words();
        if (opcode == spv::OpDecorate) {
            it->second->decorations_.emplace_back(words.begin() + 2, words.end());
        } else if (words.size() > 3) {
            it->second->element_decorations_[words[2]].emplace_back(words.begin() + 3,
                                                                     words.end());
        }
    }
}

void IRContext::BuildConstants() {
    using C = analysis::Constant;
    for (auto& inst : module_.types_values_) {
        const auto* type = types_.GetType(inst.type_id());
        std::unique_ptr<C> c;
        switch (static_cast<spv::Op>(inst.OpcodeWord())) {
            case spv::OpConstantTrue:
            case spv::OpConstantFalse:
                c = std::make_unique<C>();
                c->kind_ = C::kBool;
                c->words_ = {inst.OpcodeWord() == spv::OpConstantTrue ? 1u : 0u};
                break;
            case spv::OpConstant:
                c = std::make_unique<C>();
                c->kind_ = (type && type->kind() == analysis::Type::kFloat) ? C::kFloat : C::kInt;
                for (uint32_t i = 0; i < inst.NumInOperands(); i++) {
                    c->words_.push_back(inst.GetSingleWordInOperand(i));
                }
                break;
            case spv::OpConstantNull:
                c = std::make_unique<C>();
                c->kind_ = C::kNull;
                break;
            case spv::OpConstantComposite:
            case spv::OpSpecConstantComposite: {
                // Like spvtools, a composite is a constant only if all its components are.
                std::vector<const C*> components;
                bool ok = true;
                for (uint32_t i = 0; i < inst.NumInOperands(); i++) {
                    const auto* component =
                        constants_.FindDeclaredConstant(inst.GetSingleWordInOperand(i));
                    if (!component) {
                        ok = false;
                        break;
                    }
                    components.push_back(component);
                }
                if (ok) {
                    c = std::make_unique<C>();
                    c->kind_ = C::kComposite;
                    c->components_ = std::move(components);
                }
                break;
            }
            default:
                break;
        }
        if (!c) {
            continue;
        }
        c->type_ = type;
        constants_.constants_[inst.result_id()] = c.get();
        constants_.owned_.push_back(std::move(c));
    }
}

void IRContext::BuildStructuredCFG() {
    // As spvtools::opt::StructuredCFGAnalysis, only the innermost containing loop is recorded.
    for (auto& func : module_.functions_) {
        if (func.blocks_.empty()) {
            continue;
        }
        std::list<BasicBlock*> order;
        cfg_.ComputeStructuredOrder(&func, func.blocks_.front().get(), nullptr, &order);

        struct State {
            uint32_t containing_loop = 0;
            uint32_t merge_node = 0;
        };
        std::vector<State> state(1);
        for (BasicBlock* block : order) {
            if (block->id() == state.back().merge_node) {
                state.pop_back();
            }
            structured_cfg_.containing_loop_[block->id()] = state.back().containing_loop;
            if (Instruction* merge = block->GetMergeInst()) {
                State next;
                next.merge_node = merge->GetSingleWordInOperand(0);
                next.containing_loop = merge->OpcodeWord() == spv::OpLoopMerge
                                           ? block->id()
                                           : state.back().containing_loop;
                state.push_back(next);
            }
        }
    }
}

void IRContext::ResolveBindingConflicts() {
    struct VarBinding {
        Instruction* var;
        uint32_t set;
        Instruction* binding_decoration;
        uint32_t binding() const { return binding_decoration->GetSingleWordInOperand(2); }
    };
    std::vector<VarBinding> vars;
    for (auto& inst : module_.types_values_) {
        if (inst.OpcodeWord() != spv::OpVariable) {
            continue;
        }
        Instruction* set = nullptr;
        Instruction* binding = nullptr;
        for (auto* d : decorations_.GetDecorationsFor(inst.result_id(), false)) {
            if (d->OpcodeWord() != spv::OpDecorate) {
                continue;
            }
            if (d->GetSingleWordInOperand(1) == spv::DecorationDescriptorSet) {
                set = d;
            } else if (d->GetSingleWordInOperand(1) == spv::DecorationBinding) {
                binding = d;
            }
        }
        if (set && binding) {
            vars.push_back({&inst, set->GetSingleWordInOperand(2), binding});
        }
    }
    if (vars.empty()) {
        return;
    }

    auto unwrap = [&](uint32_t type_id) {
        Instruction* ty = def_use_.GetDef(type_id);
        while (ty) {
            auto op = static_cast<spv::Op>(ty->OpcodeWord());
            if (op == spv::OpTypePointer) {
                ty = def_use_.GetDef(ty->GetSingleWordInOperand(1));
            } else if (op == spv::OpTypeArray || op == spv::OpTypeRuntimeArray) {
                ty = def_use_.GetDef(ty->GetSingleWordInOperand(0));
            } else {
                break;
            }
        }
        return ty ? ty->OpcodeWord() : 0u;
    };
    // Orders by binding, then images before samplers, then by variable ID.
    auto less = [&](const VarBinding* a, const VarBinding* b) {
        if (a->binding() != b->binding()) {
            return a->binding() < b->binding();
        }
        uint32_t at = unwrap(a->var->type_id());
        uint32_t bt = unwrap(b->var->type_id());
        if (at != bt) {
            if (at == spv::OpTypeSampler) {
                return false;
            }
            if (bt == spv::OpTypeSampler) {
                return true;
            }
        }
        return a->var->result_id() < b->var->result_id();
    };

    using BindingList = std::vector<VarBinding*>;
    using DescriptorSets = std::map<uint32_t, BindingList>;
    std::unordered_map<uint32_t, VarBinding*> var_by_id;
    for (auto& v : vars) {
        var_by_id[v.var->result_id()] = &v;
    }

    std::unordered_map<uint32_t, const std::vector<uint32_t>*> group_of;
    for (const auto& g : handle_array_groups_) {
        for (uint32_t id : g) {
            group_of[id] = &g;
        }
    }

    // Variables used directly by each function, and the call graph.
    std::unordered_map<uint32_t, DescriptorSets> used;
    std::unordered_map<uint32_t, std::vector<uint32_t>> callees;
    for (auto& func : module_.functions_) {
        std::unordered_set<VarBinding*> seen;
        auto& sets = used[func.result_id()];
        for (auto& bb : func.blocks_) {
            for (auto& inst : bb->insts_) {
                ForEachPointerOperand(inst, [&](uint32_t id) {
                    // A used element of a split handle array uses the whole array, so all the
                    // elements keep consecutive bindings.
                    auto group = group_of.find(id);
                    std::vector<uint32_t> ids{id};
                    if (group != group_of.end()) {
                        ids = *group->second;
                    }
                    for (uint32_t used_id : ids) {
                        auto it = var_by_id.find(used_id);
                        if (it != var_by_id.end() && seen.insert(it->second).second) {
                            sets[it->second->set].push_back(it->second);
                        }
                    }
                });
                if (inst.OpcodeWord() == spv::OpFunctionCall) {
                    callees[func.result_id()].push_back(inst.GetSingleWordInOperand(0));
                }
            }
        }
        for (auto& [set, list] : sets) {
            std::stable_sort(list.begin(), list.end(), less);
        }
    }

    // Callees before callers, from each entry point.
    std::vector<uint32_t> order;
    std::unordered_set<uint32_t> visited;
    std::function<void(uint32_t)> visit = [&](uint32_t f) {
        if (!visited.insert(f).second) {
            return;
        }
        for (uint32_t c : callees[f]) {
            visit(c);
        }
        order.push_back(f);
    };
    for (auto& ep : module_.entry_points_) {
        visit(ep.GetSingleWordInOperand(1));
    }
    for (uint32_t caller : order) {
        auto& sink = used[caller];
        for (uint32_t callee : callees[caller]) {
            for (auto& [set, src] : used[callee]) {
                auto& dst = sink[set];
                BindingList merged(src.size() + dst.size());
                auto end = std::merge(src.begin(), src.end(), dst.begin(), dst.end(),
                                      merged.begin(), less);
                merged.erase(std::unique(merged.begin(), end), merged.end());
                dst = std::move(merged);
            }
        }
    }

    std::vector<BindingList*> lists;
    for (auto& ep : module_.entry_points_) {
        for (auto& [set, list] : used[ep.GetSingleWordInOperand(1)]) {
            lists.push_back(&list);
        }
    }
    bool changed;
    do {
        changed = false;
        for (auto* list : lists) {
            for (size_t i = 1; i < list->size(); i++) {
                uint32_t prev = (*list)[i - 1]->binding();
                if (prev >= (*list)[i]->binding()) {
                    (*list)[i]->binding_decoration->SetInOperandWord(2, prev + 1);
                    changed = true;
                }
            }
        }
    } while (changed);
}

}  // namespace tint::spirv::reader::spvc
