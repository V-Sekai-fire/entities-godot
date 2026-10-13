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

#include "src/tint/lang/spirv/reader/lower/depth_sample.h"

#include <vector>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/type/depth_texture.h"

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::spirv::reader::lower {
namespace {

struct State {
    core::ir::Module& ir;
    core::ir::Builder b{ir};
    core::type::Manager& ty{ir.Types()};

    bool IsSample(core::BuiltinFn fn) {
        return fn == core::BuiltinFn::kTextureSample || fn == core::BuiltinFn::kTextureSampleBias ||
               fn == core::BuiltinFn::kTextureSampleLevel ||
               fn == core::BuiltinFn::kTextureSampleGrad;
    }

    void Replace(core::ir::CoreBuiltinCall* call, const core::type::DepthTexture* tex_ty) {
        auto args = call->Args();
        core::ir::Value* tex = args[0];
        core::ir::Value* coords = args[2];
        size_t idx = 3;
        core::ir::Value* layer = nullptr;
        if (tex_ty->Dim() == core::type::TextureDimension::k2dArray) {
            layer = args[idx++];
        }
        core::ir::Value* level = nullptr;
        switch (call->Func()) {
            case core::BuiltinFn::kTextureSampleLevel:
                level = args[idx++];
                break;
            case core::BuiltinFn::kTextureSampleBias:
                idx += 1;
                break;
            case core::BuiltinFn::kTextureSampleGrad:
                idx += 2;
                break;
            default:
                break;
        }
        core::ir::Value* offset = idx < args.size() ? args[idx] : nullptr;

        b.InsertBefore(call, [&] {
            auto* vec2f = ty.vec2f();
            auto* vec2i = ty.vec2i();
            if (!level) {
                level = b.Constant(0_i);
            } else if (!level->Type()->Is<core::type::I32>()) {
                level = b.Convert(ty.i32(), level);
            }
            // Texel space, with texel centers at integers.
            auto* dims = b.Call(ty.vec2u(), core::BuiltinFn::kTextureDimensions, tex, level);
            auto* p = b.Subtract(b.Multiply(coords, b.Convert(vec2f, dims)),
                                 b.Splat(vec2f, 0.5_f));
            auto* p0 = b.Call(vec2f, core::BuiltinFn::kFloor, p);
            auto* f = b.Subtract(p, p0);
            core::ir::Value* i0 = b.Convert(vec2i, p0);
            if (offset) {
                i0 = b.Add(i0, offset);
            }
            auto* max_i = b.Subtract(b.Convert(vec2i, dims), b.Splat(vec2i, 1_i));
            auto* zero = b.Splat(vec2i, 0_i);
            auto* lo = b.Call(vec2i, core::BuiltinFn::kClamp, i0, zero, max_i);
            auto* hi = b.Call(vec2i, core::BuiltinFn::kClamp, b.Add(i0, b.Splat(vec2i, 1_i)),
                              zero, max_i);
            auto* lo_x = b.Swizzle(ty.i32(), lo, {0u});
            auto* lo_y = b.Swizzle(ty.i32(), lo, {1u});
            auto* hi_x = b.Swizzle(ty.i32(), hi, {0u});
            auto* hi_y = b.Swizzle(ty.i32(), hi, {1u});
            auto load = [&](core::ir::Value* x, core::ir::Value* y) -> core::ir::Value* {
                auto* c = b.Construct(vec2i, x, y);
                if (layer) {
                    return b.Call(ty.f32(), core::BuiltinFn::kTextureLoad, tex, c, layer, level);
                }
                return b.Call(ty.f32(), core::BuiltinFn::kTextureLoad, tex, c, level);
            };
            auto* fx = b.Swizzle(ty.f32(), f, {0u});
            auto* fy = b.Swizzle(ty.f32(), f, {1u});
            auto* top = b.Call(ty.f32(), core::BuiltinFn::kMix, load(lo_x, lo_y),
                               load(hi_x, lo_y), fx);
            auto* bottom = b.Call(ty.f32(), core::BuiltinFn::kMix, load(lo_x, hi_y),
                                  load(hi_x, hi_y), fx);
            auto* res = b.Call(ty.f32(), core::BuiltinFn::kMix, top, bottom, fy);
            call->Result()->ReplaceAllUsesWith(res);
        });
        call->Destroy();
    }

    Result<SuccessType> Process() {
        std::vector<std::pair<core::ir::CoreBuiltinCall*, const core::type::DepthTexture*>> calls;
        for (auto* inst : ir.Instructions()) {
            auto* call = inst->As<core::ir::CoreBuiltinCall>();
            if (!call || !IsSample(call->Func()) || call->Args().size() < 3) {
                continue;
            }
            auto* tex_ty = call->Args()[0]->Type()->As<core::type::DepthTexture>();
            if (!tex_ty || (tex_ty->Dim() != core::type::TextureDimension::k2d &&
                            tex_ty->Dim() != core::type::TextureDimension::k2dArray)) {
                continue;
            }
            calls.push_back({call, tex_ty});
        }
        for (auto& [call, tex_ty] : calls) {
            Replace(call, tex_ty);
        }
        return Success;
    }
};

}  // namespace

Result<SuccessType> DepthSample(core::ir::Module& ir) {
    return State{ir}.Process();
}

}  // namespace tint::spirv::reader::lower
