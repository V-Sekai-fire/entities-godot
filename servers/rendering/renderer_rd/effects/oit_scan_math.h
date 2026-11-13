/**************************************************************************/
/*  oit_scan_math.h                                                       */
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

// Models the fragment-only froxel integrate for the Compatibility (GLES3 / WebGL2) renderer.
// WebGL2 has no compute, storage buffers or atomics, so the compute prefix sum in
// oit_integrate.glsl becomes log2(slice_count) ping-pong passes. In pass k each slice
// adds the slice 2^k behind it (Hillis-Steele). Each pass writes only its own texel.

// Number of ping-pong passes needed to scan p_slice_count slices.
inline uint32_t oit_scan_pass_count(uint32_t p_slice_count) {
	uint32_t passes = 0;
	for (uint32_t span = 1; span < p_slice_count; span <<= 1) {
		passes++;
	}
	return passes;
}

// One ping-pong pass: p_out[z] = p_in[z] + (z >= p_stride ? p_in[z - p_stride] : 0).
// Every output depends only on the previous buffer, so it is a legal fragment shader.
inline void oit_scan_pass(const float *p_in, float *p_out, uint32_t p_slice_count, uint32_t p_stride) {
	for (uint32_t z = 0; z < p_slice_count; z++) {
		float v = p_in[z];
		if (z >= p_stride) {
			v += p_in[z - p_stride];
		}
		p_out[z] = v;
	}
}

// Inclusive prefix sum of one froxel column by the pass sequence the shader runs.
// p_scratch must hold p_slice_count floats. The result is left in p_column.
inline void oit_scan_column(float *p_column, float *p_scratch, uint32_t p_slice_count) {
	float *src = p_column;
	float *dst = p_scratch;
	for (uint32_t stride = 1; stride < p_slice_count; stride <<= 1) {
		oit_scan_pass(src, dst, p_slice_count, stride);
		float *t = src;
		src = dst;
		dst = t;
	}
	if (src != p_column) {
		for (uint32_t z = 0; z < p_slice_count; z++) {
			p_column[z] = src[z];
		}
	}
}

// Transmittance stored for slice z: exp(-inclusive extinction up to z). Matches the imageStore in oit_integrate.glsl.
inline float oit_scan_transmittance(float p_inclusive_extinction) {
	return std::exp(-p_inclusive_extinction);
}
