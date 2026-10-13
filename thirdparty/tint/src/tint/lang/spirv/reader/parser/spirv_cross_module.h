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

// A small in-memory SPIR-V model for the SPIR-V reader, built from SPIRV-Cross'
// spirv_cross::Parser / spirv_cross::ParsedIR. It mirrors the subset of the SPIRV-Tools
// `spvtools::opt` API that parser.cc used, so the parser does not depend on SPIRV-Tools.
//
// This header deliberately does not include any SPIRV-Cross header: SPIRV-Cross uses the C++03
// `spirv.hpp` header, the parser uses the C++11 `spirv.hpp11` header, and both define
// `namespace spv` behind the same include guard. Only spirv_cross_module.cc sees SPIRV-Cross.

#ifndef SRC_TINT_LANG_SPIRV_READER_PARSER_SPIRV_CROSS_MODULE_H_
#define SRC_TINT_LANG_SPIRV_READER_PARSER_SPIRV_CROSS_MODULE_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef TINT_SPIRV_CROSS_MODULE_IMPL
#include "spirv/unified1/spirv.hpp11"
#endif

namespace tint::spirv::reader::spvc {

/// A SPIR-V instruction, stored as its raw words. Every operand is one word; the reader only
/// builds instructions that have no literal string operands.
class Instruction {
  public:
    Instruction() = default;
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
    /// @returns the number of operands, including the result type and result ID
    uint32_t NumOperands() const { return static_cast<uint32_t>(words_.size() - 1); }
    /// @returns the number of operand words, including the result type and result ID
    uint32_t NumOperandWords() const { return NumOperands(); }
    /// @returns the number of operands, excluding the result type and result ID
    uint32_t NumInOperands() const { return NumOperands() - FirstInOperand(); }
    /// @returns the number of operand words, excluding the result type and result ID
    uint32_t NumInOperandWords() const { return NumInOperands(); }
    /// @param i the operand index, counting the result type and result ID
    /// @returns the operand word
    uint32_t GetSingleWordOperand(uint32_t i) const { return words_[1 + i]; }
    /// @param i the operand index, not counting the result type and result ID
    /// @returns the operand word
    uint32_t GetSingleWordInOperand(uint32_t i) const {
        return words_[1 + FirstInOperand() + i];
    }

  private:
    uint32_t FirstInOperand() const { return (has_type_ ? 1u : 0u) + (has_result_ ? 1u : 0u); }

    std::vector<uint32_t> words_;
    uint32_t opcode_ = 0;
    bool has_type_ = false;
    bool has_result_ = false;
};

/// A SPIR-V basic block. The OpLabel is not part of the instruction list; the merge instruction
/// (if any) and the terminator are.
class BasicBlock {
  public:
    /// @returns the block label ID
    uint32_t id() const { return id_; }
    /// @returns the first instruction
    std::vector<Instruction>::const_iterator begin() const { return insts_.begin(); }
    /// @returns the end of the instruction list
    std::vector<Instruction>::const_iterator end() const { return insts_.end(); }
    /// @returns the OpSelectionMerge or OpLoopMerge instruction, or nullptr
    const Instruction* GetMergeInst() const;
    /// @returns the OpLoopMerge instruction, or nullptr
    const Instruction* GetLoopMergeInst() const;
    /// @returns the merge block ID of the merge instruction, or 0
    uint32_t MergeBlockIdIfAny() const;
    /// @returns the continue target ID of the OpLoopMerge instruction, or 0
    uint32_t ContinueBlockIdIfAny() const;
    /// @returns true if the block ends with OpReturn or OpReturnValue
    bool IsReturn() const;
    /// Calls @p f for each successor label of the terminator.
    void ForEachSuccessorLabel(const std::function<void(uint32_t)>& f) const;

  private:
    friend class Module;
    uint32_t id_ = 0;
    std::vector<Instruction> insts_;
};

/// A SPIR-V function definition.
class Function {
  public:
    /// @returns the function result ID
    uint32_t result_id() const { return id_; }
    /// @returns the function return type ID
    uint32_t type_id() const { return return_type_; }
    /// Calls @p f with each OpFunctionParameter instruction.
    void ForEachParam(const std::function<void(const Instruction*)>& f) const;
    /// @returns the entry block
    const BasicBlock* entry() const { return blocks_.empty() ? nullptr : blocks_.front().get(); }
    /// @param id a block label ID
    /// @returns the block with label @p id, or nullptr
    const BasicBlock* FindBlock(uint32_t id) const;

  private:
    friend class Module;
    uint32_t id_ = 0;
    uint32_t return_type_ = 0;
    std::vector<Instruction> params_;
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
    std::unordered_map<uint32_t, const BasicBlock*> block_by_id_;
};

/// A decoration with its first literal operand (0 if there is none).
struct Decoration {
    /// The spv::Decoration value
    uint32_t kind = 0;
    /// The first operand of the decoration
    uint32_t value = 0;
};

/// A SPIR-V type declaration.
struct Type {
    /// The kind of type
    enum Kind {
        kVoid,
        kBool,
        kInteger,
        kFloat,
        kVector,
        kMatrix,
        kArray,
        kRuntimeArray,
        kStruct,
        kPointer,
        kSampler,
        kImage,
        kSampledImage,
        kUnsupported,
    };
    /// The image operands of an OpTypeImage, also used for OpTypeSampledImage
    struct ImageInfo {
        uint32_t sampled_type = 0;
        uint32_t dim = 0;
        uint32_t depth = 0;
        bool arrayed = false;
        bool multisampled = false;
        uint32_t sampled = 0;
        uint32_t format = 0;
        /// The access qualifier (ReadOnly if absent)
        uint32_t access = 0;
    };

    Kind kind = kUnsupported;
    /// The type ID
    uint32_t id = 0;
    /// Bit width of integer and float types
    uint32_t width = 0;
    /// Signedness of integer types
    bool is_signed = false;
    /// Element type of vectors, matrices and arrays, pointee type of pointers
    uint32_t element_type = 0;
    /// Element count of vectors, column count of matrices, length of arrays
    uint32_t count = 0;
    /// For arrays, true if the length is a specialization constant (`count` is then its ID)
    bool count_is_spec_constant = false;
    /// Storage class of pointers
    uint32_t storage_class = 0;
    /// Member types of structures
    std::vector<uint32_t> members;
    /// Member decorations of structures
    std::vector<std::vector<Decoration>> member_decorations;
    /// Image information
    ImageInfo image;
    /// A description of the type, for diagnostics
    std::string description;
};

/// A SPIR-V constant value (non-specialization constants only).
struct Constant {
    /// The constant type ID
    uint32_t type_id = 0;
    /// The scalar bits for bool, integer and float constants
    uint32_t bits = 0;
    /// The components of vector, matrix, array and structure constants
    std::vector<Constant> components;
};

/// An entry point declaration.
struct EntryPoint {
    /// The spv::ExecutionModel
    uint32_t model = 0;
    /// The entry point function ID
    uint32_t function = 0;
    /// The entry point name
    std::string name;
};

/// An OpExtInstImport declaration.
struct ExtInstImport {
    /// The result ID
    uint32_t id = 0;
    /// The instruction set name
    std::string name;
};

class Module;

/// Structured control flow queries over a function.
class CFG {
  public:
    /// @param module the module
    explicit CFG(const Module* module) : module_(module) {}
    /// Computes the structured order of the blocks reachable from @p root, stopping at @p end.
    /// Matches spvtools::opt::CFG::ComputeStructuredOrder.
    void ComputeStructuredOrder(const Function* func,
                                const BasicBlock* root,
                                const BasicBlock* end,
                                std::list<const BasicBlock*>* order) const;

  private:
    const Module* module_;
};

/// The SPIR-V module, built from SPIRV-Cross' parsed IR.
class Module {
  public:
    ~Module();

    /// Parses @p words with spirv_cross::Parser.
    /// @param words the SPIR-V binary
    /// @param count the number of words
    /// @param error set to the error message on failure
    /// @returns the module, or nullptr on failure
    static std::unique_ptr<Module> Build(const uint32_t* words, size_t count, std::string* error);

    /// @returns the names of the OpExtension declarations
    const std::vector<std::string>& extensions() const { return extensions_; }
    /// @returns the OpExtInstImport declarations
    const std::vector<ExtInstImport>& ext_inst_imports() const { return ext_inst_imports_; }
    /// @returns the OpName names, keyed by ID
    const std::unordered_map<uint32_t, std::string>& names() const { return names_; }
    /// @returns the OpMemberName names, keyed by structure ID
    const std::unordered_map<uint32_t, std::vector<std::string>>& member_names() const {
        return member_names_;
    }
    /// @returns the module-scope OpSpecConstantTrue, OpSpecConstantFalse, OpSpecConstant,
    /// OpSpecConstantComposite, OpSpecConstantOp, OpUndef and OpVariable instructions, in
    /// declaration order within each group
    const std::vector<Instruction>& types_values() const { return types_values_; }
    /// @returns the function definitions in declaration order
    const std::vector<std::unique_ptr<Function>>& functions() const { return functions_; }
    /// @returns the entry points
    const std::vector<EntryPoint>& entry_points() const { return entry_points_; }
    /// @returns OpExecutionMode instructions (function, mode, literals...)
    const std::vector<Instruction>& execution_modes() const { return execution_modes_; }

    /// @param id a result ID
    /// @returns the decorations on @p id
    std::vector<Decoration> GetDecorationsFor(uint32_t id) const;
    /// @param id a type ID
    /// @returns the type, or nullptr
    const Type* GetType(uint32_t id) const;
    /// @param id a result ID
    /// @returns the constant if @p id is a non-specialization constant
    std::optional<Constant> FindDeclaredConstant(uint32_t id) const;
    /// @returns the control flow graph helper
    const CFG* cfg() const { return &cfg_; }

  private:
    struct Impl;
    Module();

    std::unique_ptr<Impl> impl_;
    CFG cfg_{this};
    std::vector<std::string> extensions_;
    std::vector<ExtInstImport> ext_inst_imports_;
    std::unordered_map<uint32_t, std::string> names_;
    std::unordered_map<uint32_t, std::vector<std::string>> member_names_;
    std::vector<Instruction> types_values_;
    std::vector<std::unique_ptr<Function>> functions_;
    std::vector<EntryPoint> entry_points_;
    std::vector<Instruction> execution_modes_;
    mutable std::unordered_map<uint32_t, std::unique_ptr<Type>> types_;
};

}  // namespace tint::spirv::reader::spvc

#endif  // SRC_TINT_LANG_SPIRV_READER_PARSER_SPIRV_CROSS_MODULE_H_
