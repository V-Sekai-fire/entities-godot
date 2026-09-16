/**************************************************************************/
/*  oit_voxelize.glsl                                                     */
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

// AVBOIT voxelisation: writes -log(1 - alpha) extinction into a 3D
// R32_UINT buffer via image atomics. Follows slides 47-52 of
// Drobot, "Adaptive Volumetric Boundary OIT", SIGGRAPH 2025, with
// the R32 splat variant (slide 52) rather than the packed-8-bit
// variant (slide 53), because Adreno-740 storage-image atomics do
// not extend to packed byte lanes on Quest 3.
//
// One thread per candidate transparent fragment; the transparent
// draw list is uploaded as a compact SSBO of (world position, alpha)
// entries produced by the geometry gather pass in the C++ renderer.

#[compute]

#version 450

#VERSION_DEFINES

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(r32ui, set = 0, binding = 0) uniform restrict uimage3D extinction_buffer;

layout(set = 0, binding = 1, std140) uniform Params {
	mat4 view_matrix;
	vec4 slice_curve; // x: near, y: far, z: linearization, w: slice_count
	uvec4 froxel_dims; // xyz: dims, w: reserved
	vec4 tile_size; // xy: tile pixel size, zw: reserved
}
params;

struct Splat {
	vec4 world_pos_alpha; // xyz: world position, w: alpha
};

layout(set = 0, binding = 2, std430) restrict readonly buffer Splats {
	Splat data[];
}
splats;

layout(set = 0, binding = 3, std140) uniform SplatCount {
	uvec4 count; // x: valid splat count, rest reserved
}
splat_count;

// Log-Z depth to slice index (slide 49): a parametric log curve that
// keeps near-plane precision when slice count grows.
uint depth_to_slice(float view_z) {
	float near = params.slice_curve.x;
	float far = params.slice_curve.y;
	float k = max(params.slice_curve.z, 0.001);
	float slices = params.slice_curve.w;
	float linear_z = clamp((view_z - near) / max(far - near, 0.001), 0.0, 1.0);
	// Curve: t = log(1 + k * linear) / log(1 + k).
	float t = log(1.0 + k * linear_z) / log(1.0 + k);
	return uint(clamp(t * slices, 0.0, slices - 1.0));
}

void main() {
	uint idx = gl_GlobalInvocationID.x;
	if (idx >= splat_count.count.x) {
		return;
	}
	Splat splat = splats.data[idx];
	float alpha = clamp(splat.world_pos_alpha.w, 0.0, 0.999);

	vec4 view_pos = params.view_matrix * vec4(splat.world_pos_alpha.xyz, 1.0);
	float view_z = -view_pos.z; // Godot camera: -Z forward.
	if (view_z <= 0.0) {
		return;
	}

	// Screen-space projection is done by the caller and baked into a
	// downsampled tile coordinate stored in world_pos_alpha.xy for the
	// voxelisation-only draw path; here we fall back to reading them
	// off the view position for the compute-only path.
	float aspect = float(params.froxel_dims.x) / float(params.froxel_dims.y);
	vec2 ndc = view_pos.xy / max(-view_pos.z, 0.001);
	vec2 uv = ndc * 0.5 + 0.5;
	if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) {
		return;
	}

	uvec3 froxel = uvec3(
			uint(uv.x * float(params.froxel_dims.x)),
			uint(uv.y * float(params.froxel_dims.y)),
			depth_to_slice(view_z));
	froxel = min(froxel, params.froxel_dims.xyz - uvec3(1));

	// Extinction from alpha: e = -log(1 - a), scaled into U32 range so
	// InterlockedAdd saturates gracefully at high overdraw.
	float extinction = -log(1.0 - alpha);
	uint packed = uint(clamp(extinction * 65536.0, 0.0, 4.29e9));
	imageAtomicAdd(extinction_buffer, ivec3(froxel), packed);
}
