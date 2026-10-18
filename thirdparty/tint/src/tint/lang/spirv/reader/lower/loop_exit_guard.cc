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

#include "src/tint/lang/spirv/reader/lower/loop_exit_guard.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/exit_loop.h"
#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/switch.h"

namespace tint::spirv::reader::lower {
namespace {

// SPIR-V lets a block inside a switch branch straight to the merge of the loop around the switch
// (Slang wraps every loop body in `switch 0 { default: ... }` to lower `continue`). WGSL's `break`
// inside a switch leaves only the switch, so the IR forbids an exit_loop that jumps over a switch.
// Rewrite each such exit_loop as: set a guard, leave the innermost control, and after every
// control on the way out to the outermost switch, `if guard { exit <next control> }`; after that
// switch, `if guard { exit_loop }`. Values that the jumped-over controls return are zero on this
// path, which is never read: the loop exits right after.
struct State {
    core::ir::Module& ir;
    core::ir::Builder b{ir};
    core::type::Manager& ty{ir.Types()};

    // Guard var per outermost switch, and the controls already followed by a guard check.
    std::unordered_map<core::ir::ControlInstruction*, core::ir::Var*> guards;
    std::unordered_set<core::ir::ControlInstruction*> checked;

    core::ir::Instruction* ExitWithZeros(core::ir::ControlInstruction* ctrl) {
        Vector<core::ir::Value*, 4> args;
        for (auto* r : ctrl->Results()) {
            args.Push(b.Zero(r->Type()));
        }
        return b.Exit(ctrl, std::move(args));
    }

    void GuardAfter(core::ir::ControlInstruction* after, core::ir::Var* guard,
                    core::ir::ControlInstruction* exit_of) {
        if (!checked.insert(after).second) {
            return;
        }
        b.InsertAfter(after, [&] {
            auto* cond = b.Load(guard);
            auto* if_ = b.If(cond);
            b.Append(if_->True(), [&] { ExitWithZeros(exit_of); });
            b.Append(if_->False(), [&] { b.ExitIf(if_); });
        });
    }

    void Fix(core::ir::ExitLoop* exit) {
        auto* loop = exit->Loop();
        // The controls between the exit and its loop, innermost first.
        std::vector<core::ir::ControlInstruction*> chain;
        for (auto* ctrl = exit->Block()->Parent(); ctrl && ctrl != loop; ctrl = ctrl->Block()->Parent()) {
            chain.push_back(ctrl);
        }
        size_t outer = chain.size();
        for (size_t i = 0; i < chain.size(); ++i) {
            if (chain[i]->Is<core::ir::Switch>() || chain[i]->Is<core::ir::Loop>()) {
                outer = i;
            }
        }
        if (outer == chain.size() || !exit->Args().empty()) {
            return;  // Nothing jumped over but `if`s (allowed), or loop results to carry (unhandled).
        }
        auto* top = chain[outer];
        auto*& guard = guards[top];
        if (!guard) {
            b.InsertBefore(top, [&] { guard = b.Var("exit_guard", false); });
        }
        b.InsertBefore(exit, [&] {
            b.Store(guard, true);
            ExitWithZeros(chain[0]);
        });
        exit->Destroy();
        for (size_t i = 0; i < outer; ++i) {
            GuardAfter(chain[i], guard, chain[i + 1]);
        }
        GuardAfter(top, guard, loop);
    }

    Result<SuccessType> Process() {
        std::vector<core::ir::ExitLoop*> exits;
        for (auto* inst : ir.Instructions()) {
            if (auto* exit = inst->As<core::ir::ExitLoop>()) {
                exits.push_back(exit);
            }
        }
        for (auto* exit : exits) {
            Fix(exit);
        }
        return Success;
    }
};

}  // namespace

Result<SuccessType> LoopExitGuard(core::ir::Module& ir) {
    return State{ir}.Process();
}

}  // namespace tint::spirv::reader::lower
