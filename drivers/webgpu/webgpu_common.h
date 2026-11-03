/**************************************************************************/
/*  webgpu_common.h                                                       */
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

#pragma once

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

inline uint32_t webgpu_immediates_set(uint32_t p_uniform_set_count) {
	return p_uniform_set_count < (uint32_t)(WEBGPU_MAX_BIND_GROUPS - 1) ? p_uniform_set_count : (uint32_t)(WEBGPU_MAX_BIND_GROUPS - 1);
}
