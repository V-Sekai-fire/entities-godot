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

#include "src/tint/lang/spirv/reader/lower/handle_params.h"

#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/clone_context.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/user_call.h"
#include "src/tint/lang/core/ir/var.h"

namespace tint::spirv::reader::lower {
namespace {

bool IsHandlePointer(const core::type::Type* type) {
    auto* ptr = type->As<core::type::Pointer>();
    return ptr && ptr->AddressSpace() == core::AddressSpace::kHandle;
}

/// @returns the module-scope variable @p value is the result of, or nullptr
core::ir::Var* RootVar(core::ir::Module& ir, core::ir::Value* value) {
    auto* res = value->As<core::ir::InstructionResult>();
    if (!res) {
        return nullptr;
    }
    auto* var = res->Instruction()->As<core::ir::Var>();
    if (!var || var->Block() != ir.root_block) {
        return nullptr;
    }
    return var;
}

struct State {
    core::ir::Module& ir;

    /// (callee, the variable bound to each parameter or nullptr) -> specialized copy
    std::map<std::pair<core::ir::Function*, std::vector<core::ir::Var*>>, core::ir::Function*>
        variants{};

    core::ir::Function* Variant(core::ir::Function* callee,
                                const std::vector<core::ir::Var*>& bound) {
        auto key = std::make_pair(callee, bound);
        if (auto it = variants.find(key); it != variants.end()) {
            return it->second;
        }
        auto* fn = core::ir::CloneContext{ir}.Clone(callee);
        if (auto name = ir.NameOf(callee); name.IsValid()) {
            ir.SetName(fn, ir.symbols.New(name.Name()));
        }
        Vector<core::ir::FunctionParam*, 8> kept;
        auto params = fn->Params();
        for (size_t i = 0; i < params.Length(); i++) {
            if (bound[i]) {
                params[i]->ReplaceAllUsesWith(bound[i]->Result());
                params[i]->Destroy();
            } else {
                kept.Push(params[i]);
            }
        }
        fn->SetParams(std::move(kept));
        ir.functions.Push(fn);
        variants[key] = fn;
        return fn;
    }

    /// Rewrites the calls in @p fn that pass a variable to a handle pointer parameter.
    /// @returns true if a call was rewritten
    bool Specialize(core::ir::Function* fn) {
        std::vector<core::ir::UserCall*> calls;
        core::ir::Traverse(fn->Block(), [&](core::ir::UserCall* call) { calls.push_back(call); });
        bool changed = false;
        for (auto* call : calls) {
            auto* callee = call->Target();
            auto params = callee->Params();
            auto args = call->Args();
            std::vector<core::ir::Var*> bound(params.Length(), nullptr);
            bool any = false;
            for (size_t i = 0; i < params.Length() && i < args.size(); i++) {
                if (IsHandlePointer(params[i]->Type())) {
                    bound[i] = RootVar(ir, args[i]);
                    any = any || bound[i] != nullptr;
                }
            }
            if (!any) {
                continue;
            }
            Vector<core::ir::Value*, 8> new_args;
            for (size_t i = 0; i < args.size(); i++) {
                if (i >= bound.size() || !bound[i]) {
                    new_args.Push(args[i]);
                }
            }
            auto* variant = Variant(callee, bound);
            call->SetTarget(variant);
            call->SetArgs(std::move(new_args));
            changed = true;
        }
        return changed;
    }

    Result<SuccessType> Process() {
        // Specialize until no call passes a variable to a handle pointer parameter. A copy can
        // itself call with a parameter that is now a variable, so repeat over all functions.
        for (bool changed = true; changed;) {
            changed = false;
            Vector<core::ir::Function*, 16> fns(ir.functions);
            for (auto* fn : fns) {
                changed = Specialize(fn) || changed;
            }
        }

        // Keep the functions reachable from the entry points.
        std::unordered_set<core::ir::Function*> live;
        std::vector<core::ir::Function*> work;
        for (auto* fn : ir.functions) {
            if (fn->IsEntryPoint()) {
                live.insert(fn);
                work.push_back(fn);
            }
        }
        while (!work.empty()) {
            auto* fn = work.back();
            work.pop_back();
            core::ir::Traverse(fn->Block(), [&](core::ir::UserCall* call) {
                if (live.insert(call->Target()).second) {
                    work.push_back(call->Target());
                }
            });
        }
        Vector<core::ir::Function*, 16> kept;
        for (auto* fn : ir.functions) {
            if (live.count(fn)) {
                kept.Push(fn);
            } else {
                fn->Destroy();
            }
        }
        ir.functions = std::move(kept);

        for (auto* fn : ir.functions) {
            for (auto* param : fn->Params()) {
                if (IsHandlePointer(param->Type())) {
                    return Failure("a texture or sampler function parameter is not passed a "
                                   "module-scope variable");
                }
            }
        }
        return Success;
    }
};

}  // namespace

Result<SuccessType> HandleParams(core::ir::Module& ir) {
    return State{ir}.Process();
}

}  // namespace tint::spirv::reader::lower
