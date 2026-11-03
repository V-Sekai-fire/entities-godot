/**************************************************************************/
/*  webgpu_tint.h                                                         */
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

// SPIR-V to WGSL with thirdparty/tint. This header is plain C++17 so that the rest of the driver
// does not include tint, which needs C++20.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct WebGpuTintBinding {
	enum Kind : uint32_t {
		KIND_TEXTURE,
		KIND_SAMPLER,
		KIND_OTHER,
	};
	enum SampleType : uint32_t {
		SAMPLE_FLOAT,
		SAMPLE_UNFILTERABLE_FLOAT,
		SAMPLE_DEPTH,
		SAMPLE_SINT,
		SAMPLE_UINT,
	};
	enum SamplerType : uint32_t {
		SAMPLER_FILTERING,
		SAMPLER_NON_FILTERING,
		SAMPLER_COMPARISON,
	};

	uint32_t group = 0;
	uint32_t binding = 0;
	Kind kind = KIND_OTHER;
	SampleType sample_type = SAMPLE_FLOAT;
	bool multisampled = false;
	SamplerType sampler_type = SAMPLER_FILTERING;
};

// The WGSL binding of one element of a Godot uniform (see Options::binding_plan in tint's reader).
struct WebGpuTintPlanEntry {
	uint32_t group = 0;
	uint32_t binding = 0;
	uint32_t element = 0;
	bool sampler = false;
	uint32_t wgsl_binding = 0;
};

bool webgpu_tint_spirv_to_wgsl(const uint32_t *p_words, size_t p_word_count, const std::vector<WebGpuTintPlanEntry> &p_plan, uint32_t p_push_constant_group,
		uint32_t p_push_constant_binding, uint32_t p_push_constant_max_size, std::string &r_wgsl,
		std::vector<WebGpuTintBinding> &r_bindings, std::vector<uint64_t> &r_read, std::string &r_error);
