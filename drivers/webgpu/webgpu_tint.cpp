/**************************************************************************/
/*  webgpu_tint.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

// Compiled as C++20 on its own (see SCsub); includes no Godot headers.

#include "webgpu_tint.h"

#include "src/tint/api/tint.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/load.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/sampler.h"
#include "src/tint/lang/spirv/reader/reader.h"
#include "src/tint/lang/wgsl/writer/writer.h"

#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <tuple>

namespace {

// The module-scope variable a texture or sampler value is loaded from, or nullptr.
const tint::core::ir::Var *root_var(const tint::core::ir::Value *p_value) {
	for (int depth = 0; depth < 16 && p_value; depth++) {
		auto *res = p_value->As<tint::core::ir::InstructionResult>();
		if (!res) {
			return nullptr;
		}
		auto *inst = res->Instruction();
		if (auto *var = inst->As<tint::core::ir::Var>()) {
			return var;
		}
		if (auto *load = inst->As<tint::core::ir::Load>()) {
			p_value = load->From();
		} else if (auto *let = inst->As<tint::core::ir::Let>()) {
			p_value = let->Value();
		} else {
			return nullptr;
		}
	}
	return nullptr;
}

const tint::core::type::Type *handle_type(const tint::core::ir::Var *p_var) {
	auto *ptr = p_var->Result()->Type()->As<tint::core::type::Pointer>();
	return ptr ? ptr->StoreType() : nullptr;
}

bool is_compare(tint::core::BuiltinFn p_fn) {
	return p_fn == tint::core::BuiltinFn::kTextureSampleCompare ||
			p_fn == tint::core::BuiltinFn::kTextureSampleCompareLevel ||
			p_fn == tint::core::BuiltinFn::kTextureGatherCompare;
}

} // namespace

bool webgpu_tint_spirv_to_wgsl(const uint32_t *p_words, size_t p_word_count, const std::vector<WebGpuTintPlanEntry> &p_plan, uint32_t p_push_constant_group,
		uint32_t p_push_constant_binding, uint32_t p_push_constant_max_size, std::string &r_wgsl,
		std::vector<WebGpuTintBinding> &r_bindings, std::string &r_error) {
	static std::once_flag init;
	std::call_once(init, [] { tint::Initialize(); });

	std::vector<uint32_t> words(p_words, p_words + p_word_count);
	tint::spirv::reader::Options options;
	options.push_constant_binding = tint::BindingPoint{ p_push_constant_group, p_push_constant_binding };
	options.push_constant_max_size = p_push_constant_max_size;
	std::map<std::tuple<uint32_t, uint32_t, uint32_t, bool>, uint32_t> plan;
	for (const WebGpuTintPlanEntry &e : p_plan) {
		plan[{ e.group, e.binding, e.element, e.sampler }] = e.wgsl_binding;
	}
	if (!plan.empty()) {
		options.binding_plan = [&plan](uint32_t p_group, uint32_t p_binding, uint32_t p_element, bool p_sampler) -> std::optional<uint32_t> {
			auto it = plan.find({ p_group, p_binding, p_element, p_sampler });
			if (it == plan.end()) {
				return std::nullopt;
			}
			return it->second;
		};
	}
	auto ir = tint::spirv::reader::ReadIR(words, options);
	if (ir != tint::Success) {
		r_error = "spirv reader: " + ir.Failure().reason;
		return false;
	}

	// Textures and samplers the module uses, then how they pair up in texture builtin calls.
	std::map<const tint::core::ir::Var *, WebGpuTintBinding> handles;
	for (auto *inst : *ir->root_block) {
		auto *var = inst->As<tint::core::ir::Var>();
		if (!var || !var->BindingPoint() || var->Result()->NumUsages() == 0) {
			continue;
		}
		const tint::core::type::Type *type = handle_type(var);
		WebGpuTintBinding b;
		b.group = var->BindingPoint()->group;
		b.binding = var->BindingPoint()->binding;
		if (auto *sampler = type ? type->As<tint::core::type::Sampler>() : nullptr) {
			b.kind = WebGpuTintBinding::KIND_SAMPLER;
			b.sampler_type = sampler->IsComparison() ? WebGpuTintBinding::SAMPLER_COMPARISON : WebGpuTintBinding::SAMPLER_FILTERING;
		} else if (type && type->Is<tint::core::type::DepthTexture>()) {
			b.kind = WebGpuTintBinding::KIND_TEXTURE;
			b.sample_type = WebGpuTintBinding::SAMPLE_DEPTH;
		} else if (type && type->Is<tint::core::type::DepthMultisampledTexture>()) {
			b.kind = WebGpuTintBinding::KIND_TEXTURE;
			b.sample_type = WebGpuTintBinding::SAMPLE_DEPTH;
			b.multisampled = true;
		} else if (auto *sampled = type ? type->As<tint::core::type::SampledTexture>() : nullptr) {
			b.kind = WebGpuTintBinding::KIND_TEXTURE;
			const tint::core::type::Type *el = sampled->Type();
			// f32 textures are unfilterable unless a sampling call pairs them with a sampler below.
			b.sample_type = el->IsSignedIntegerScalar() ? WebGpuTintBinding::SAMPLE_SINT
					: el->IsUnsignedIntegerScalar()		? WebGpuTintBinding::SAMPLE_UINT
														: WebGpuTintBinding::SAMPLE_UNFILTERABLE_FLOAT;
		} else if (auto *ms = type ? type->As<tint::core::type::MultisampledTexture>() : nullptr) {
			b.kind = WebGpuTintBinding::KIND_TEXTURE;
			const tint::core::type::Type *el = ms->Type();
			b.sample_type = el->IsSignedIntegerScalar() ? WebGpuTintBinding::SAMPLE_SINT
					: el->IsUnsignedIntegerScalar()		? WebGpuTintBinding::SAMPLE_UINT
														: WebGpuTintBinding::SAMPLE_UNFILTERABLE_FLOAT;
			b.multisampled = true;
		} else {
			continue;
		}
		handles[var] = b;
	}

	// A sampler used with a texture that cannot be filtered must be non-filtering; an f32 texture
	// sampled with a filtering sampler is filterable.
	std::set<const tint::core::ir::Var *> unfilterable_samplers;
	std::vector<std::pair<const tint::core::ir::Var *, const tint::core::ir::Var *>> pairs;
	for (auto *inst : ir->Instructions()) {
		auto *call = inst->As<tint::core::ir::CoreBuiltinCall>();
		if (!call) {
			continue;
		}
		const tint::core::ir::Var *texture = nullptr;
		const tint::core::ir::Var *sampler = nullptr;
		for (auto *arg : call->Args()) {
			const tint::core::ir::Var *var = root_var(arg);
			auto it = var ? handles.find(var) : handles.end();
			if (it == handles.end()) {
				continue;
			}
			if (it->second.kind == WebGpuTintBinding::KIND_TEXTURE && !texture) {
				texture = var;
			} else if (it->second.kind == WebGpuTintBinding::KIND_SAMPLER && !sampler) {
				sampler = var;
			}
		}
		if (!texture || !sampler || is_compare(call->Func())) {
			continue;
		}
		pairs.push_back({ texture, sampler });
	}
	for (auto &[texture, sampler] : pairs) {
		WebGpuTintBinding &t = handles[texture];
		if (t.sample_type == WebGpuTintBinding::SAMPLE_UNFILTERABLE_FLOAT && !t.multisampled) {
			t.sample_type = WebGpuTintBinding::SAMPLE_FLOAT;
		}
	}
	for (auto &[texture, sampler] : pairs) {
		const WebGpuTintBinding &t = handles[texture];
		if (t.sample_type != WebGpuTintBinding::SAMPLE_FLOAT) {
			unfilterable_samplers.insert(sampler);
		}
	}
	for (const tint::core::ir::Var *sampler : unfilterable_samplers) {
		WebGpuTintBinding &s = handles[sampler];
		if (s.sampler_type == WebGpuTintBinding::SAMPLER_FILTERING) {
			s.sampler_type = WebGpuTintBinding::SAMPLER_NON_FILTERING;
		}
	}

	tint::wgsl::writer::Options writer_options;
	writer_options.allow_non_uniform_derivatives = true;
	writer_options.allow_non_uniform_subgroup_operations = true;
	writer_options.disable_unreachable_code_warning = true;
	writer_options.allowed_features = tint::wgsl::AllowedFeatures::Everything();
	auto result = tint::wgsl::writer::WgslFromIR(ir.Get(), writer_options);
	if (result != tint::Success) {
		r_error = "wgsl writer: " + result.Failure().reason;
		return false;
	}
	r_wgsl = result->wgsl;
	r_bindings.clear();
	for (auto &[var, b] : handles) {
		r_bindings.push_back(b);
	}
	return true;
}
