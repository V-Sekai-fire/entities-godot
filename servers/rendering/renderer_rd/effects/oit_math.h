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

// Views share one froxel grid packed side by side along X (Oit/Views.lean).
inline uint32_t oit_packed_column(uint32_t p_dim_x, uint32_t p_view, uint32_t p_x) {
	return p_view * p_dim_x + p_x;
}

// Mirrors oit_transmittance_in_front: the eye's screen u clamped to its half-texel band, then offset into its slab.
inline float oit_packed_u(uint32_t p_dim_x, uint32_t p_view_count, uint32_t p_view, float p_u) {
	float half = 0.5f / float(p_dim_x);
	float c = p_u < half ? half : (p_u > 1.0f - half ? 1.0f - half : p_u);
	return (c + float(p_view)) / float(p_view_count);
}

inline uint32_t oit_sampled_column(uint32_t p_dim_x, uint32_t p_view_count, float p_packed_u) {
	return uint32_t(std::floor(p_packed_u * float(p_dim_x * p_view_count)));
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

// The premultiplied pair the resolve fragment writes; the framebuffer blends it as color + background * (1 - alpha).
inline void oit_resolve_out(const OITAccum &p_accum, float &r_color, float &r_alpha) {
	float total = std::exp(-p_accum.extinction);
	r_color = p_accum.alpha > 0.0f ? p_accum.color * (1.0f - total) / p_accum.alpha : 0.0f;
	r_alpha = 1.0f - total;
}

inline float oit_resolve(const OITAccum &p_accum, float p_background) {
	float color, alpha;
	oit_resolve_out(p_accum, color, alpha);
	return color + p_background * (1.0f - alpha);
}

// Under MSAA the resolve folds one accumulator per sample into the mean of their resolves.
inline float oit_resolve_samples(const OITAccum *p_samples, uint32_t p_sample_count, float p_background) {
	float color = 0.0f;
	float alpha = 0.0f;
	for (uint32_t s = 0; s < p_sample_count; s++) {
		float sc, sa;
		oit_resolve_out(p_samples[s], sc, sa);
		color += sc / float(p_sample_count);
		alpha += sa / float(p_sample_count);
	}
	return color + p_background * (1.0f - alpha);
}

// Mirrors Oit.SliceCurve.autoRange in the Lean spec (lean/Oit/SliceCurve.lean), which pins the bounds, the 1 cm floor and the thin-layer separation.
// The slice range for a view of translucent geometry whose view-space depths run from p_min_z to p_max_z (metres in front of the camera):
// the bounds grown by a margin, never nearer than 1 cm, with the curve that gives every slice the same relative thickness (see below). Returns false when there is nothing
// in front of the camera, and the caller keeps the fixed planes.
inline bool oit_auto_range(float p_min_z, float p_max_z, float p_margin, float &r_near, float &r_far, float &r_linearization) {
	if (!(p_max_z > 0.0f) || !(p_max_z >= p_min_z)) {
		return false;
	}
	r_near = p_min_z - p_margin > 0.01f ? p_min_z - p_margin : 0.01f;
	r_far = p_max_z + p_margin;
	if (r_far < r_near + 0.001f) {
		r_far = r_near + 0.001f;
	}
	// k = span / near turns log(1 + k t) / log(1 + k) into log(z / near) / log(far / near): the same relative resolution at every depth, ln(far / near) / slices of the depth per slice. For a narrow range (hair, about 1.3 m or less) k is small and the curve is nearly linear; for a wide one (planets) it is log depth, so the range works at any scale.
	float k = (r_far - r_near) / r_near;
	r_linearization = k < 0.001f ? 0.001f : (k > 1.0e18f ? 1.0e18f : k);
	return true;
}
