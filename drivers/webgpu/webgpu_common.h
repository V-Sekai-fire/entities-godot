#ifndef WEBGPU_COMMON_H
#define WEBGPU_COMMON_H

#include <cstdint>

enum class WebGpuBindingHintType {
	UNUSED = 0,
	SAMPLER = 2,
	TEXTURE = 3,
};

struct WebGpuBindingSamplerHint {
	uint32_t sampler_type = 0;
};

struct WebGpuBindingTextureHint {
	uint32_t sample_type = 0;
	uint32_t multisampled = 0;
};

// NOTE: This doesn't need to be here if it turns out that we don't need to store these in `ShaderInfo`
// I don't think we do, but we do store them right now for debugging.
struct WebGpuBindingHint {
	WebGpuBindingHintType type = WebGpuBindingHintType::UNUSED;
	union {
		WebGpuBindingSamplerHint sampler;
		WebGpuBindingTextureHint texture;
	};
	WebGpuBindingHint() {}
};

#define WEBGPU_MAX_BIND_GROUPS 4
// This should just be 128 up to minUniformBufferOffsetAlignment.
#define WEBGPU_MAX_IMMEDIATE_SIZE 256

#define WEBGPU_PUSH_CONSTANT_EMULATION_BUFFER_ENTRIES 8192

#define WEBGPU_PUSH_CONSTANT_EMULATION_BINDING 256

// The bind group that holds the push constant emulation uniform buffer: one past the uniform sets
// the shader uses, capped at the last bind group. The SPIR-V to WGSL step is told to put the push
// constant block at @group(webgpu_immediates_set(n)) @binding(WEBGPU_PUSH_CONSTANT_EMULATION_BINDING)
// and the driver binds `push_constant_emulation_buffer` there.
inline uint32_t webgpu_immediates_set(uint32_t p_uniform_set_count) {
	return p_uniform_set_count < (uint32_t)(WEBGPU_MAX_BIND_GROUPS - 1) ? p_uniform_set_count : (uint32_t)(WEBGPU_MAX_BIND_GROUPS - 1);
}

#endif
