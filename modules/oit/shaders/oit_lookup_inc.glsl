/**************************************************************************/
/*  oit_lookup_inc.glsl                                                   */
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

// Scene-shader include: sample the integrated-transmittance buffer at
// a world-space position and return the transmittance factor. The
// transparent fragment multiplies its alpha by (1 - transmittance) to
// get an order-independent contribution.
//
// The Forward+ and Mobile scene shaders each #include this and call
// oit_apply(view_pos_z, world_pos, alpha) before writing gl_FragColor.

#ifndef OIT_LOOKUP_INC_GLSL
#define OIT_LOOKUP_INC_GLSL

// Bound by RenderForwardMobile / RenderForwardClustered at a set the
// scene shader reserves for global effects. The engine hook stores
// the descriptor and inclusive-scan produced by oit_integrate.glsl.
// Set/binding numbers come from the C++ side; the include declares
// only the interface and expects the caller to append -DOIT_SET=N.

#ifndef OIT_SET
#define OIT_SET 3
#endif

layout(set = OIT_SET, binding = 0) uniform sampler3D oit_transmittance;

layout(set = OIT_SET, binding = 1, std140) uniform OITParams {
	mat4 view_matrix;
	vec4 slice_curve; // x: near, y: far, z: linearization, w: slice_count
	uvec4 froxel_dims; // xyz: dims, w: enabled flag
}
oit_params;

float oit_depth_to_slice_uv(float view_z) {
	float near = oit_params.slice_curve.x;
	float far = oit_params.slice_curve.y;
	float k = max(oit_params.slice_curve.z, 0.001);
	float linear_z = clamp((view_z - near) / max(far - near, 0.001), 0.0, 1.0);
	return log(1.0 + k * linear_z) / log(1.0 + k);
}

// Modulates a transparent fragment's alpha by the accumulated
// transmittance at its 3D position. If OIT is disabled the buffer's
// enabled flag is zero and the fragment passes through unchanged.
float oit_apply(vec3 world_pos, float alpha) {
	if (oit_params.froxel_dims.w == 0u) {
		return alpha;
	}
	vec4 view_pos = oit_params.view_matrix * vec4(world_pos, 1.0);
	float view_z = -view_pos.z;
	if (view_z <= 0.0) {
		return alpha;
	}
	vec2 ndc = view_pos.xy / max(-view_pos.z, 0.001);
	vec3 uvw = vec3(ndc * 0.5 + 0.5, oit_depth_to_slice_uv(view_z));
	if (any(lessThan(uvw.xy, vec2(0.0))) || any(greaterThan(uvw.xy, vec2(1.0)))) {
		return alpha;
	}
	float transmittance = texture(oit_transmittance, uvw).r;
	return alpha * transmittance;
}

#endif // OIT_LOOKUP_INC_GLSL
