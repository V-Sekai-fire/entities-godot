#ifndef OIT_LOOKUP_INC_GLSL
#define OIT_LOOKUP_INC_GLSL

#ifndef OIT_SET
#define OIT_SET 3
#endif

layout(set = OIT_SET, binding = 0) uniform sampler3D oit_transmittance;

layout(set = OIT_SET, binding = 1, std140) uniform OITParams {
	mat4 view_matrix;
	vec4 slice_curve;
	uvec4 froxel_dims;
}
oit_params;

float oit_depth_to_slice_uv(float view_z) {
	float near = oit_params.slice_curve.x;
	float far = oit_params.slice_curve.y;
	float k = max(oit_params.slice_curve.z, 0.001);
	float linear_z = clamp((view_z - near) / max(far - near, 0.001), 0.0, 1.0);
	return log(1.0 + k * linear_z) / log(1.0 + k);
}

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

#endif
