/**************************************************************************/
/*  rendering_shader_container_webgpu.cpp                                 */
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

#ifdef WEBGPU_ENABLED

#include "rendering_shader_container_webgpu.h"

#include "core/error/error_macros.h"
#include "core/io/marshalls.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "drivers/webgpu/webgpu_binding_remap.h"
#include "drivers/webgpu/webgpu_platform.h"
#include "drivers/webgpu/webgpu_tint.h"

#include <thirdparty/spirv-reflect/spirv_reflect.h>

#define DEBUG_SHADERS

#ifdef DEBUG_SHADERS
#define DEBUG_SHADERS_RAW_LOCATION "/tmp/shader"

#include <cstdio>

static void _debug_dump_shader(const char *p_dir, const String &p_shader_name, RenderingDeviceCommons::ShaderStage p_stage, const uint8_t *p_data, size_t p_size) {
	String safe = p_shader_name.is_empty() ? String("unnamed") : p_shader_name;
	safe = safe.replace_char(':', '_').replace_char('/', '_').replace_char(' ', '_');
	String stage_suffix;
	switch (p_stage) {
		case RenderingDeviceCommons::SHADER_STAGE_VERTEX:
			stage_suffix = "vert";
			break;
		case RenderingDeviceCommons::SHADER_STAGE_FRAGMENT:
			stage_suffix = "frag";
			break;
		case RenderingDeviceCommons::SHADER_STAGE_COMPUTE:
			stage_suffix = "comp";
			break;
		default:
			stage_suffix = String("stage") + itos((int)p_stage);
			break;
	}
	String path = String(p_dir) + "/" + safe + "_" + stage_suffix + ".spv";
	FILE *f = fopen(path.utf8().get_data(), "wb");
	if (!f) {
		print_line("[WGPU][DEBUG_SHADERS] failed to open ", path);
		return;
	}
	if (p_size > 0) {
		fwrite(p_data, 1, p_size, f);
	}
	fclose(f);
}
#endif

// 2: stages hold WGSL from tint (1 held SPIR-V).
const uint32_t RenderingShaderContainerWebGpu::FORMAT_VERSION = 2;

uint32_t RenderingShaderContainerWebGpu::_format() const {
	// 'WGPU' LE
	return 0x55504757;
}

uint32_t RenderingShaderContainerWebGpu::_format_version() const {
	return FORMAT_VERSION;
}

static RenderingDeviceCommons::TextureType _texture_type_from_spv(SpvDim p_dim, bool p_arrayed) {
	using RDC = RenderingDeviceCommons;
	switch (p_dim) {
		case SpvDim1D:
			return p_arrayed ? RDC::TEXTURE_TYPE_1D_ARRAY : RDC::TEXTURE_TYPE_1D;
		case SpvDim2D:
		case SpvDimRect:
		case SpvDimSubpassData:
			return p_arrayed ? RDC::TEXTURE_TYPE_2D_ARRAY : RDC::TEXTURE_TYPE_2D;
		case SpvDim3D:
			return RDC::TEXTURE_TYPE_3D;
		case SpvDimCube:
			return p_arrayed ? RDC::TEXTURE_TYPE_CUBE_ARRAY : RDC::TEXTURE_TYPE_CUBE;
		case SpvDimBuffer:
			return RDC::TEXTURE_TYPE_2D;
		default:
			return RDC::TEXTURE_TYPE_2D;
	}
}

void RenderingShaderContainerWebGpu::_set_from_shader_reflection_post(const ReflectShader &p_shader) {
	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());
}

// The corrections the driver's _remap_binding_indices() reads to number WebGPU bindings: an array
// of N handles becomes N bindings, each element of a combined image sampler a texture then a
// sampler, and every later binding in the set moves up by the bindings added before it.
// _webgpu_plan() gives tint's reader the same numbering.
static void _webgpu_corrections(RenderingDeviceCommons::UniformType p_type, uint32_t p_length, Vector<uint32_t> &r_corrections) {
	using RDC = RenderingDeviceCommons;
	r_corrections.clear();
	switch (p_type) {
		case RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE:
			r_corrections.push_back(WEBGPU_BINDING_REMAP_SPLIT_COMBINED);
			for (uint32_t i = 1; i < p_length; i++) {
				r_corrections.push_back(WEBGPU_BINDING_REMAP_SPLIT_BINDING_ARRAY);
			}
			break;
		case RDC::UNIFORM_TYPE_SAMPLER:
		case RDC::UNIFORM_TYPE_TEXTURE:
		case RDC::UNIFORM_TYPE_IMAGE:
			for (uint32_t i = 1; i < p_length; i++) {
				r_corrections.push_back(WEBGPU_BINDING_REMAP_SPLIT_BINDING_ARRAY);
			}
			break;
		default:
			break;
	}
}

// Appends the WebGPU binding of each element of a uniform, numbered as
// _remap_binding_indices() does from _webgpu_corrections(), and returns the bindings it added.
static uint32_t _webgpu_plan(uint32_t p_set, uint32_t p_binding, RenderingDeviceCommons::UniformType p_type, uint32_t p_length, uint32_t p_offset, std::vector<WebGpuTintPlanEntry> &r_plan) {
	using RDC = RenderingDeviceCommons;
	const uint32_t first = p_binding + p_offset;
	switch (p_type) {
		case RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
			const uint32_t n = MAX(p_length, 1u);
			for (uint32_t k = 0; k < n; k++) {
				r_plan.push_back({ p_set, p_binding, k, false, first + 2 * k });
				r_plan.push_back({ p_set, p_binding, k, true, first + 2 * k + 1 });
			}
			return 2 * n - 1;
		}
		case RDC::UNIFORM_TYPE_SAMPLER:
		case RDC::UNIFORM_TYPE_TEXTURE:
		case RDC::UNIFORM_TYPE_IMAGE: {
			const uint32_t n = MAX(p_length, 1u);
			for (uint32_t k = 0; k < n; k++) {
				r_plan.push_back({ p_set, p_binding, k, p_type == RDC::UNIFORM_TYPE_SAMPLER, first + k });
			}
			return n - 1;
		}
		default:
			r_plan.push_back({ p_set, p_binding, 0, false, first });
			return 0;
	}
}

static WebGpuBindingHint _webgpu_hint(const WebGpuTintBinding &p_binding) {
	WebGpuBindingHint hint;
	if (p_binding.kind == WebGpuTintBinding::KIND_SAMPLER) {
		hint.type = WebGpuBindingHintType::SAMPLER;
		switch (p_binding.sampler_type) {
			case WebGpuTintBinding::SAMPLER_COMPARISON:
				hint.sampler.sampler_type = WGPUSamplerBindingType_Comparison;
				break;
			case WebGpuTintBinding::SAMPLER_NON_FILTERING:
				hint.sampler.sampler_type = WGPUSamplerBindingType_NonFiltering;
				break;
			default:
				hint.sampler.sampler_type = WGPUSamplerBindingType_Filtering;
				break;
		}
	} else {
		hint.type = WebGpuBindingHintType::TEXTURE;
		switch (p_binding.sample_type) {
			case WebGpuTintBinding::SAMPLE_DEPTH:
				hint.texture.sample_type = WGPUTextureSampleType_Depth;
				break;
			case WebGpuTintBinding::SAMPLE_UNFILTERABLE_FLOAT:
				hint.texture.sample_type = WGPUTextureSampleType_UnfilterableFloat;
				break;
			case WebGpuTintBinding::SAMPLE_SINT:
				hint.texture.sample_type = WGPUTextureSampleType_Sint;
				break;
			case WebGpuTintBinding::SAMPLE_UINT:
				hint.texture.sample_type = WGPUTextureSampleType_Uint;
				break;
			default:
				hint.texture.sample_type = WGPUTextureSampleType_Float;
				break;
		}
		hint.texture.multisampled = p_binding.multisampled ? 1 : 0;
	}
	return hint;
}

// Two stages may use one binding differently (one samples it, one only loads it): keep the one
// that constrains the layout more.
static void _webgpu_merge(WebGpuTintBinding &r_into, const WebGpuTintBinding &p_from) {
	if (r_into.kind == WebGpuTintBinding::KIND_SAMPLER) {
		if (p_from.sampler_type == WebGpuTintBinding::SAMPLER_COMPARISON ||
				(p_from.sampler_type == WebGpuTintBinding::SAMPLER_NON_FILTERING && r_into.sampler_type == WebGpuTintBinding::SAMPLER_FILTERING)) {
			r_into.sampler_type = p_from.sampler_type;
		}
	} else if (r_into.sample_type == WebGpuTintBinding::SAMPLE_UNFILTERABLE_FLOAT || p_from.sample_type == WebGpuTintBinding::SAMPLE_DEPTH) {
		if (p_from.sample_type != WebGpuTintBinding::SAMPLE_FLOAT) {
			r_into.sample_type = p_from.sample_type;
		}
	}
	r_into.multisampled = r_into.multisampled || p_from.multisampled;
}

// Combined image samplers that hold depth but are read as plain float textures through a linear
// sampler. Vulkan allows that; WebGPU cannot filter depth, and the SPIR-V does not say the texture
// holds depth. Their layouts take an unfilterable-float texture and a non-filtering sampler, and
// the driver binds a nearest copy of the sampler Godot passes.
struct WebGpuDepthRead {
	const char *shader; // ShaderRD name, before ":<variant>".
	uint32_t set;
	uint32_t binding;
};
static const WebGpuDepthRead WEBGPU_DEPTH_READS[] = {
	{ "CubeToDpShaderRD", 0, 0 }, // The omni light's cube shadow, copied into the shadow atlas.
};

static bool _webgpu_is_depth_read(const String &p_shader_name, uint32_t p_set, uint32_t p_binding) {
	const String shader = p_shader_name.get_slicec(':', 0);
	for (const WebGpuDepthRead &r : WEBGPU_DEPTH_READS) {
		if (shader == r.shader && p_set == r.set && p_binding == r.binding) {
			return true;
		}
	}
	return false;
}

bool RenderingShaderContainerWebGpu::_set_code_from_spirv(const ReflectShader &p_shader) {
	const LocalVector<ReflectShaderStage> &p_spirv = p_shader.shader_stages;

	String shader_name_str = String::utf8(shader_name.ptr(), shader_name.length());

	// The bind group the push constant block goes to; the driver binds its emulation buffer there.
	const uint32_t push_constant_group = webgpu_immediates_set(p_shader.uniform_sets.size());

	// One binding plan for all stages, from the whole shader's uniforms.
	std::vector<WebGpuTintPlanEntry> plan;
	for (uint32_t set_idx = 0; set_idx < p_shader.uniform_sets.size(); set_idx++) {
		const ReflectDescriptorSet &set_refl = p_shader.uniform_sets[set_idx];
		uint32_t offset = 0;
		for (uint32_t binding_idx = 0; binding_idx < set_refl.size(); binding_idx++) {
			const ReflectUniform &uniform_refl = set_refl[binding_idx];
			offset += _webgpu_plan(set_idx, uniform_refl.binding, uniform_refl.type, uniform_refl.length, offset, plan);
		}
	}

	// SPIR-V -> WGSL for each stage, with the textures and samplers each stage uses.
	HashMap<uint64_t, WebGpuTintBinding> used;
	Vector<CharString> wgsl_stages;
	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		Span<uint32_t> stage_spirv = p_spirv[i].spirv();

#ifdef DEBUG_SHADERS
		_debug_dump_shader(DEBUG_SHADERS_RAW_LOCATION, shader_name_str, p_spirv[i].shader_stage, (const uint8_t *)stage_spirv.ptr(), stage_spirv.size() * sizeof(uint32_t));
#endif

		std::string wgsl;
		std::string error;
		std::vector<WebGpuTintBinding> bindings;
		bool ok = webgpu_tint_spirv_to_wgsl(stage_spirv.ptr(), stage_spirv.size(), plan, push_constant_group,
				WEBGPU_PUSH_CONSTANT_EMULATION_BINDING, WEBGPU_MAX_IMMEDIATE_SIZE, wgsl, bindings, error);
		ERR_FAIL_COND_V_MSG(!ok, false, vformat("SPIR-V to WGSL failed for %s stage %s: %s", shader_name_str, String(RenderingDeviceCommons::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]), String::utf8(error.c_str())));

		for (const WebGpuTintBinding &b : bindings) {
			uint64_t key = (uint64_t(b.group) << 32) | b.binding;
			WebGpuTintBinding *existing = used.getptr(key);
			if (existing) {
				_webgpu_merge(*existing, b);
			} else {
				used.insert(key, b);
			}
		}
		CharString text;
		text.resize_uninitialized(wgsl.size() + 1);
		memcpy(text.ptrw(), wgsl.data(), wgsl.size());
		text.ptrw()[wgsl.size()] = 0;
		wgsl_stages.push_back(text);
	}

	// Corrections and binding hints for each Godot uniform.
	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());

	uint32_t global_idx = 0;
	for (uint32_t set_idx = 0; set_idx < p_shader.uniform_sets.size(); set_idx++) {
		const ReflectDescriptorSet &set_refl = p_shader.uniform_sets[set_idx];

		uint32_t offset = 0; // WebGPU bindings added by earlier uniforms of this set.
		for (uint32_t binding_idx = 0; binding_idx < set_refl.size(); binding_idx++) {
			const ReflectUniform &uniform_refl = set_refl[binding_idx];
			UniformData &u = webgpu_uniform_data.write[global_idx];

			const SpvReflectDescriptorBinding &binding = uniform_refl.get_spv_reflect();
			u.image_format = (uint32_t)uniform_refl.texture_format;
			u.texture_image_type = (uint32_t)_texture_type_from_spv((SpvDim)binding.image.dim, binding.image.arrayed != 0);
			u.texture_is_multisample = binding.image.ms != 0 ? 1u : 0u;
			u.texture_sample_type = (uint32_t)RDC::ShaderUniform::TextureSampleType::Float;

			bool non_writable = (binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) ||
					(binding.block.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE);
			bool non_readable = (binding.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE) ||
					(binding.block.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE);
			if (!non_writable && !non_readable) {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::ReadWrite;
			} else if (non_writable && !non_readable) {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::ReadOnly;
			} else if (!non_writable && non_readable) {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::WriteOnly;
			} else {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::ReadOnly;
			}

			_webgpu_corrections(uniform_refl.type, uniform_refl.length, u.corrections);
			u.base_hint = WebGpuBindingHint();
			u.binding_hints.clear();

			// Element k of this uniform is WebGPU binding first + k. Unused elements stay UNUSED,
			// which the driver reads as "prune this layout entry".
			const uint32_t first = uniform_refl.binding + offset;
			const uint32_t count = 1 + (uint32_t)u.corrections.size(); // Hints the driver reads.
			bool have_base = false;
			for (uint32_t k = 0; k < count; k++) {
				const WebGpuTintBinding *b = used.getptr((uint64_t(set_idx) << 32) | (first + k));
				WebGpuBindingHint hint = b ? _webgpu_hint(*b) : WebGpuBindingHint();
				if (k > 0) {
					u.binding_hints.push_back(hint);
				}
				// The base hint describes the whole uniform: element 0 for a combined image
				// sampler (the texture), else any used element of the array.
				const bool combined = uniform_refl.type == RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
				if (b && !have_base && (k == 0 || !combined)) {
					u.base_hint = hint;
					have_base = true;
				}
			}
			if (uniform_refl.type == RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE && uniform_refl.length <= 1 &&
					u.base_hint.type == WebGpuBindingHintType::TEXTURE && u.binding_hints.size() == 1 &&
					_webgpu_is_depth_read(shader_name_str, set_idx, uniform_refl.binding)) {
				u.base_hint.texture.sample_type = WGPUTextureSampleType_UnfilterableFloat;
				u.binding_hints.write[0].type = WebGpuBindingHintType::SAMPLER;
				u.binding_hints.write[0].sampler.sampler_type = WGPUSamplerBindingType_NonFiltering;
			}
			offset += uniform_refl.type == RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE ? 2 * MAX(uniform_refl.length, 1u) - 1 : (uint32_t)u.corrections.size();
			global_idx++;
		}
	}

	shaders.resize(wgsl_stages.size());
	for (int64_t i = 0; i < wgsl_stages.size(); i++) {
		const uint8_t *src = (const uint8_t *)wgsl_stages[i].get_data();
		uint32_t src_size = (uint32_t)wgsl_stages[i].length();

		Shader &shader = shaders.write[i];
		shader.shader_stage = p_spirv[i].shader_stage;
		shader.code_decompressed_size = src_size;
		shader.code_compressed_bytes.resize(src_size);

		uint32_t compressed_size = 0;
		bool compressed = compress_code(
				src, src_size,
				shader.code_compressed_bytes.ptrw(), &compressed_size, &shader.code_compression_flags);
		ERR_FAIL_COND_V_MSG(!compressed, false, vformat("Failed to compress WGSL for stage #%d.", i));

		shader.code_compressed_bytes.resize(compressed_size);
	}

	return true;
}

// Size of the non-arrayed items we store in container extra data.
static constexpr uint32_t WEBGPU_UNIFORM_EXTRA_BASE_SIZE =
		sizeof(WebGpuBindingHint) + 5 * sizeof(uint32_t);

uint32_t RenderingShaderContainerWebGpu::_to_bytes_reflection_binding_uniform_extra_data(uint8_t *p_bytes, uint32_t p_index) const {
	const UniformData &u = webgpu_uniform_data[p_index];
	uint32_t correction_count = (uint32_t)u.corrections.size();
	uint32_t total = WEBGPU_UNIFORM_EXTRA_BASE_SIZE + sizeof(uint32_t) + correction_count * sizeof(uint32_t) + correction_count * sizeof(WebGpuBindingHint);
	if (p_bytes == nullptr) {
		return total;
	}

	uint8_t *p = p_bytes;
	memcpy(p, &u.base_hint, sizeof(WebGpuBindingHint));
	p += sizeof(WebGpuBindingHint);
	encode_uint32(u.image_format, p);
	p += sizeof(uint32_t);
	encode_uint32(u.image_access, p);
	p += sizeof(uint32_t);
	encode_uint32(u.texture_image_type, p);
	p += sizeof(uint32_t);
	encode_uint32(u.texture_sample_type, p);
	p += sizeof(uint32_t);
	encode_uint32(u.texture_is_multisample, p);
	p += sizeof(uint32_t);
	encode_uint32(correction_count, p);
	p += sizeof(uint32_t);

	for (uint32_t k = 0; k < correction_count; k++) {
		encode_uint32(u.corrections[k], p);
		p += sizeof(uint32_t);
	}
	for (uint32_t k = 0; k < correction_count; k++) {
		memcpy(p, &u.binding_hints[k], sizeof(WebGpuBindingHint));
		p += sizeof(WebGpuBindingHint);
	}

	return total;
}

uint32_t RenderingShaderContainerWebGpu::_from_bytes_reflection_binding_uniform_extra_data_start(const uint8_t *p_bytes) {
	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());
	return 0;
}

uint32_t RenderingShaderContainerWebGpu::_from_bytes_reflection_binding_uniform_extra_data(const uint8_t *p_bytes, uint32_t p_index) {
	UniformData &u = webgpu_uniform_data.write[p_index];

	const uint8_t *p = p_bytes;
	memcpy(&u.base_hint, p, sizeof(WebGpuBindingHint));
	p += sizeof(WebGpuBindingHint);
	u.image_format = decode_uint32(p);
	p += sizeof(uint32_t);
	u.image_access = decode_uint32(p);
	p += sizeof(uint32_t);
	u.texture_image_type = decode_uint32(p);
	p += sizeof(uint32_t);
	u.texture_sample_type = decode_uint32(p);
	p += sizeof(uint32_t);
	u.texture_is_multisample = decode_uint32(p);
	p += sizeof(uint32_t);
	uint32_t correction_count = decode_uint32(p);
	p += sizeof(uint32_t);

	u.corrections.resize(correction_count);
	for (uint32_t k = 0; k < correction_count; k++) {
		u.corrections.write[k] = decode_uint32(p);
		p += sizeof(uint32_t);
	}
	u.binding_hints.resize(correction_count);
	for (uint32_t k = 0; k < correction_count; k++) {
		memcpy(&u.binding_hints.write[k], p, sizeof(WebGpuBindingHint));
		p += sizeof(WebGpuBindingHint);
	}

	return WEBGPU_UNIFORM_EXTRA_BASE_SIZE + sizeof(uint32_t) + correction_count * (sizeof(uint32_t) + sizeof(WebGpuBindingHint));
}

RenderingShaderContainerWebGpu::RenderingShaderContainerWebGpu() {}

Ref<RenderingShaderContainer> RenderingShaderContainerFormatWebGpu::create_container() const {
	return memnew(RenderingShaderContainerWebGpu);
}

RenderingDeviceCommons::ShaderLanguageVersion RenderingShaderContainerFormatWebGpu::get_shader_language_version() const {
	return SHADER_LANGUAGE_VULKAN_VERSION_1_1;
}

RenderingDeviceCommons::ShaderSpirvVersion RenderingShaderContainerFormatWebGpu::get_shader_spirv_version() const {
	return SHADER_SPIRV_VERSION_1_3;
}

RenderingShaderContainerFormatWebGpu::RenderingShaderContainerFormatWebGpu() {}
RenderingShaderContainerFormatWebGpu::~RenderingShaderContainerFormatWebGpu() {}

#endif // WEBGPU_ENABLED
