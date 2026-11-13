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
#include <utility>
#include <vector>

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

// Packed layout: four slices share one RGBA texel (tile z / 4, channel z % 4), as oit_atlas_texel_packed stores them.
// The scan runs on the tiles. Every pass reads only the previous buffer and writes only its own texel, so each is a legal fragment shader.
//   local: each texel becomes the inclusive prefix of its own four channels, so channel 3 holds the tile total.
//   tiles: Hillis-Steele over the tiles on channel 3 only; channels 0-2 are copied. Afterwards channel 3 of tile t is the inclusive sum of tile totals 0..t.
//   carry: tile t adds channel 3 of tile t - 1 to channels 0-2. Channel 3 already includes the carry.
// For 128 slices that is 1 + 5 + 1 = 7 passes, the same as the unpacked scan.
struct OITScanTexel {
	float v[4];
};

inline void oit_scan_packed_pass_local(const OITScanTexel *p_in, OITScanTexel *p_out, uint32_t p_tiles) {
	for (uint32_t t = 0; t < p_tiles; t++) {
		float run = 0.0f;
		for (uint32_t c = 0; c < 4; c++) {
			run += p_in[t].v[c];
			p_out[t].v[c] = run;
		}
	}
}

inline void oit_scan_packed_pass_tiles(const OITScanTexel *p_in, OITScanTexel *p_out, uint32_t p_tiles, uint32_t p_stride) {
	for (uint32_t t = 0; t < p_tiles; t++) {
		p_out[t] = p_in[t];
		if (t >= p_stride) {
			p_out[t].v[3] += p_in[t - p_stride].v[3];
		}
	}
}

inline void oit_scan_packed_pass_carry(const OITScanTexel *p_in, OITScanTexel *p_out, uint32_t p_tiles) {
	for (uint32_t t = 0; t < p_tiles; t++) {
		float carry = t > 0 ? p_in[t - 1].v[3] : 0.0f;
		p_out[t].v[0] = p_in[t].v[0] + carry;
		p_out[t].v[1] = p_in[t].v[1] + carry;
		p_out[t].v[2] = p_in[t].v[2] + carry;
		p_out[t].v[3] = p_in[t].v[3];
	}
}

inline uint32_t oit_scan_packed_pass_count(uint32_t p_slice_count) {
	return 2 + oit_scan_pass_count((p_slice_count + 3) / 4);
}

// Inclusive prefix sum of one column of p_slice_count slices through the packed pass sequence. The result is left in p_column.
inline void oit_scan_packed_column(float *p_column, uint32_t p_slice_count) {
	uint32_t tiles = (p_slice_count + 3) / 4;
	std::vector<OITScanTexel> a(tiles), b(tiles);
	for (uint32_t t = 0; t < tiles; t++) {
		a[t] = OITScanTexel{ { 0.0f, 0.0f, 0.0f, 0.0f } };
	}
	for (uint32_t z = 0; z < p_slice_count; z++) {
		a[z / 4].v[z % 4] = p_column[z];
	}
	oit_scan_packed_pass_local(a.data(), b.data(), tiles);
	std::swap(a, b);
	for (uint32_t stride = 1; stride < tiles; stride <<= 1) {
		oit_scan_packed_pass_tiles(a.data(), b.data(), tiles, stride);
		std::swap(a, b);
	}
	oit_scan_packed_pass_carry(a.data(), b.data(), tiles);
	std::swap(a, b);
	for (uint32_t z = 0; z < p_slice_count; z++) {
		p_column[z] = a[z / 4].v[z % 4];
	}
}
