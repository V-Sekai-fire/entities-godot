/**************************************************************************/
/*  oit_math.h                                                            */
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

#include <cmath>
#include <cstdint>

// Mirrors the GLSL oit_depth_to_slice in oit_voxelize.glsl / scene_forward_mobile_inc.glsl.
// Kept in C++ so it can be tested without a live RenderingDevice.
inline float oit_clamp01(float p_v) {
	return p_v < 0.0f ? 0.0f : (p_v > 1.0f ? 1.0f : p_v);
}

inline float oit_slice_uv(float p_near, float p_far, float p_linearization, float p_view_z) {
	float k = p_linearization < 0.001f ? 0.001f : p_linearization;
	float span = p_far - p_near;
	if (span < 0.001f) {
		span = 0.001f;
	}
	float linear_z = oit_clamp01((p_view_z - p_near) / span);
	return std::log(1.0f + k * linear_z) / std::log(1.0f + k);
}

inline uint32_t oit_depth_to_slice(float p_near, float p_far, float p_linearization, uint32_t p_slice_count, float p_view_z) {
	float t = oit_slice_uv(p_near, p_far, p_linearization, p_view_z);
	float scaled = t * float(p_slice_count);
	float upper = float(p_slice_count) - 1.0f;
	float clamped = scaled < 0.0f ? 0.0f : (scaled > upper ? upper : scaled);
	return uint32_t(clamped);
}

inline uint32_t oit_pack_extinction(float p_alpha) {
	float a = p_alpha < 0.0f ? 0.0f : (p_alpha > 0.999f ? 0.999f : p_alpha);
	float scaled = -std::log(1.0f - a) * 65536.0f;
	float clamped = scaled < 0.0f ? 0.0f : (scaled > 4.29e9f ? 4.29e9f : scaled);
	return uint32_t(clamped);
}

inline uint32_t oit_flat_index(uint32_t p_dim_y, uint32_t p_dim_z, uint32_t p_x, uint32_t p_y, uint32_t p_z) {
	return (p_x * p_dim_y + p_y) * p_dim_z + p_z;
}

// Mirrors oit_apply: slice z reads the inclusive prefix of slice z - 1, and slice 0 reads 1.
inline float oit_lookup_transmittance(const float *p_column, uint32_t p_slice) {
	return p_slice == 0 ? 1.0f : p_column[p_slice - 1];
}

// The three accumulation targets of the weighted resolve (oit_resolve.glsl), one channel of colour.
struct OITAccum {
	float color = 0.0f;
	float alpha = 0.0f;
	float extinction = 0.0f;
};

inline void oit_accumulate(OITAccum &p_accum, float p_color, float p_alpha, float p_transmittance_in_front) {
	float a = p_alpha > 0.999f ? 0.999f : (p_alpha < 0.0f ? 0.0f : p_alpha);
	p_accum.color += p_color * p_alpha * p_transmittance_in_front;
	p_accum.alpha += p_alpha * p_transmittance_in_front;
	p_accum.extinction += -std::log(1.0f - a);
}

inline float oit_resolve(const OITAccum &p_accum, float p_background) {
	float total = std::exp(-p_accum.extinction);
	float transparent = p_accum.alpha > 0.0f ? p_accum.color * (1.0f - total) / p_accum.alpha : 0.0f;
	return transparent + p_background * total;
}
