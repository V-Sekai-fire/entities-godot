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

// Depth intervals. One global auto range gives every slice the same relative thickness, so a far window
// next to hair coarsens the hair layers until they merge. The translucent bounds are merged into up to
// OIT_MAX_INTERVALS depth intervals instead, and the slices are shared out between them: the depths
// where nothing translucent exists get no slices. One interval is the same curve as oit_auto_range.
// Mirrors Oit.SliceCurve.intervalSlice (the shader's oit_slice_pos).
#define OIT_MAX_INTERVALS 8

struct OITIntervalSet {
	float lo[OIT_MAX_INTERVALS + 1];
	float hi[OIT_MAX_INTERVALS + 1];
	int count = 0;

	void add(float p_lo, float p_hi) {
		if (!(p_hi > 0.0f) || !(p_hi >= p_lo)) {
			return;
		}
		// Merge with everything it touches; a grown interval can touch more, so repeat.
		bool changed = true;
		while (changed) {
			changed = false;
			for (int i = 0; i < count; i++) {
				if (p_lo <= hi[i] && p_hi >= lo[i]) {
					p_lo = p_lo < lo[i] ? p_lo : lo[i];
					p_hi = p_hi > hi[i] ? p_hi : hi[i];
					lo[i] = lo[count - 1];
					hi[i] = hi[count - 1];
					count--;
					changed = true;
					break;
				}
			}
		}
		lo[count] = p_lo;
		hi[count] = p_hi;
		count++;
		if (count > OIT_MAX_INTERVALS) {
			sort();
			// Join the two neighbours with the smallest gap in relative terms (a gap of 1 mm at 2 m is
			// worth less than a gap of 10 m at 20 m).
			int best = 0;
			float best_gap = 1.0e30f;
			for (int i = 0; i + 1 < count; i++) {
				float a = hi[i] > 0.01f ? hi[i] : 0.01f;
				float gap = std::log((lo[i + 1] > 0.01f ? lo[i + 1] : 0.01f) / a);
				if (gap < best_gap) {
					best_gap = gap;
					best = i;
				}
			}
			hi[best] = hi[best + 1];
			for (int i = best + 1; i + 1 < count; i++) {
				lo[i] = lo[i + 1];
				hi[i] = hi[i + 1];
			}
			count--;
		}
	}

	void sort() {
		for (int i = 1; i < count; i++) {
			float l = lo[i];
			float h = hi[i];
			int j = i - 1;
			while (j >= 0 && lo[j] > l) {
				lo[j + 1] = lo[j];
				hi[j + 1] = hi[j];
				j--;
			}
			lo[j + 1] = l;
			hi[j + 1] = h;
		}
	}
};

// Fills r_ranges[i] = {near, far, linearization, first slice} for the sorted intervals, an equal share of
// the slices each. Returns the interval count (0 when there is nothing to fit).
inline int oit_interval_ranges(OITIntervalSet &p_set, int p_slices, float p_margin, float r_ranges[OIT_MAX_INTERVALS][4]) {
	p_set.sort();
	int n = p_set.count;
	if (n == 0) {
		return 0;
	}
	int share = p_slices / n;
	for (int i = 0; i < n; i++) {
		float near_z, far_z, k;
		oit_auto_range(p_set.lo[i], p_set.hi[i], p_margin, near_z, far_z, k);
		r_ranges[i][0] = near_z;
		r_ranges[i][1] = far_z;
		r_ranges[i][2] = k;
		r_ranges[i][3] = float(share * i);
	}
	return n;
}

// The slice coordinate (0 to slices) of a view depth; mirrors the shader.
inline float oit_interval_slice(const float p_ranges[OIT_MAX_INTERVALS][4], int p_count, int p_slices, float p_z) {
	int i = 0;
	while (i + 1 < p_count && p_z > p_ranges[i][1]) {
		i++;
	}
	float end = i + 1 < p_count ? p_ranges[i + 1][3] : float(p_slices);
	float k = p_ranges[i][2] > 0.001f ? p_ranges[i][2] : 0.001f;
	float span = p_ranges[i][1] - p_ranges[i][0];
	float lin = (p_z - p_ranges[i][0]) / (span > 0.001f ? span : 0.001f);
	lin = lin < 0.0f ? 0.0f : (lin > 1.0f ? 1.0f : lin);
	float t = std::log(1.0f + k * lin) / std::log(1.0f + k);
	return p_ranges[i][3] + t * (end - p_ranges[i][3]);
}
