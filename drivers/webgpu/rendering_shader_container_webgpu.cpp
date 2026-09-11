#ifdef WEBGPU_ENABLED

#include "rendering_shader_container_webgpu.h"

#include "core/error/error_macros.h"
#include "core/io/marshalls.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"

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

const uint32_t RenderingShaderContainerWebGpu::FORMAT_VERSION = 1;

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

bool RenderingShaderContainerWebGpu::_set_code_from_spirv(const ReflectShader &p_shader) {
	const LocalVector<ReflectShaderStage> &p_spirv = p_shader.shader_stages;

	String shader_name_str = String::utf8(shader_name.ptr(), shader_name.length());

	Vector<String> skip_shaders = {
		"GiShader",
		"ClusterDebugShaderRD:0",
		"BokehDofRasterShaderRD:0",
		"CubeToDpShaderRD:0",
		"VoxelGiDebugShaderRD",
		"Sdfgi",
	};

	for (const String &name : skip_shaders) {
		if (shader_name_str.contains(name)) {
			ERR_FAIL_V_MSG(false, vformat("Refusing to compile %s", name));
		}
	}

	// SPIR-V is handed straight to Dawn via emdawnwebgpu; Dawn does the
	// SPIR-V -> WGSL step internally.

	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());

	uint32_t global_idx = 0;
	for (uint32_t set_idx = 0; set_idx < p_shader.uniform_sets.size(); set_idx++) {
		const ReflectDescriptorSet &set_refl = p_shader.uniform_sets[set_idx];

		for (uint32_t binding_idx = 0; binding_idx < set_refl.size(); binding_idx++) {
			const ReflectUniform &uniform_refl = set_refl[binding_idx];
			UniformData &u = webgpu_uniform_data.write[global_idx];

			const SpvReflectDescriptorBinding &binding = uniform_refl.get_spv_reflect();
			u.image_format = (uint32_t)uniform_refl.image.format;
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

			u.base_hint = WebGpuBindingHint();
			u.corrections.clear();
			u.binding_hints.clear();

			global_idx++;
		}
	}

	shaders.resize(p_spirv.size());
	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		Span<uint32_t> stage_spirv = p_spirv[i].spirv();
		const uint8_t *src = (const uint8_t *)stage_spirv.ptr();
		uint32_t src_size = (uint32_t)(stage_spirv.size() * sizeof(uint32_t));

#ifdef DEBUG_SHADERS
		_debug_dump_shader(DEBUG_SHADERS_RAW_LOCATION, shader_name_str, p_spirv[i].shader_stage, src, src_size);
#endif

		Shader &shader = shaders.write[i];
		shader.shader_stage = p_spirv[i].shader_stage;
		shader.code_decompressed_size = src_size;
		shader.code_compressed_bytes.resize(src_size);

		uint32_t compressed_size = 0;
		bool compressed = compress_code(
				src, src_size,
				shader.code_compressed_bytes.ptrw(), &compressed_size, &shader.code_compression_flags);
		ERR_FAIL_COND_V_MSG(!compressed, false, vformat("Failed to compress SPIR-V for stage #%d.", i));

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
