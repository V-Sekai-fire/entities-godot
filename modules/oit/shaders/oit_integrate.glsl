/**************************************************************************/
/*  oit_integrate.glsl                                                    */
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

// AVBOIT integration: one workgroup per (x, y) column walks the Z
// stack once and writes the running sum of extinction (i.e. the
// transmittance integral, in log space) into an RGBA8 buffer. A
// single workgroup handles a full column so the pass has no
// cross-workgroup barrier, which is the Quest 3 constraint that
// steers the layout (README).

#[compute]

#version 450

#VERSION_DEFINES

layout(local_size_x = 1, local_size_y = 1, local_size_z = 128) in;

layout(r32ui, set = 0, binding = 0) uniform restrict readonly uimage3D extinction_buffer;
layout(rgba8, set = 0, binding = 1) uniform restrict writeonly image3D integrated_buffer;

layout(set = 0, binding = 2, std140) uniform Params {
	uvec4 froxel_dims; // xyz: dims, w: slice_count in z
}
params;

shared float shared_extinction[256];

void main() {
	ivec2 xy = ivec2(gl_WorkGroupID.xy);
	uint z = gl_LocalInvocationID.z;

	if (xy.x >= int(params.froxel_dims.x) || xy.y >= int(params.froxel_dims.y)) {
		return;
	}

	uint slice_count = params.froxel_dims.z;
	if (z >= slice_count) {
		return;
	}

	// Load and unpack the raw extinction contributed by this slice.
	uint raw = imageLoad(extinction_buffer, ivec3(xy, int(z))).x;
	float extinction = float(raw) / 65536.0;
	shared_extinction[z] = extinction;
	memoryBarrierShared();
	barrier();

	// Hillis-Steele inclusive prefix sum along Z. slice_count is a
	// small power of two by construction (project setting enum: 64,
	// 128, 256, 512), so the fixed-strided loop is exact.
	for (uint stride = 1u; stride < slice_count; stride <<= 1u) {
		float partner = 0.0;
		if (z >= stride) {
			partner = shared_extinction[z - stride];
		}
		barrier();
		if (z >= stride) {
			shared_extinction[z] += partner;
		}
		barrier();
	}

	// Transmittance from integrated extinction and pack into unorm.
	// The stored value is transmittance itself, so a transparent
	// fragment can multiply its alpha by 1 - transmittance to get the
	// order-independent blend factor.
	float transmittance = exp(-shared_extinction[z]);
	imageStore(integrated_buffer, ivec3(xy, int(z)),
			vec4(transmittance, transmittance, transmittance, transmittance));
}
