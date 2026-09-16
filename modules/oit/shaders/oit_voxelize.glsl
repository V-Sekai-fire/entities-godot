#[compute]

#version 450

#VERSION_DEFINES

// AVBOIT voxelisation: Drobot, SIGGRAPH 2025, slide 52 (R32 splat).

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(r32ui, set = 0, binding = 0) uniform restrict uimage3D extinction_buffer;

layout(set = 0, binding = 1, std140) uniform Params {
	mat4 view_matrix;
	vec4 slice_curve;
	uvec4 froxel_dims;
	vec4 tile_size;
}
params;

struct Splat {
	vec4 world_pos_alpha;
};

layout(set = 0, binding = 2, std430) restrict readonly buffer Splats {
	Splat data[];
}
splats;

layout(set = 0, binding = 3, std140) uniform SplatCount {
	uvec4 count;
}
splat_count;

uint depth_to_slice(float view_z) {
	float near = params.slice_curve.x;
	float far = params.slice_curve.y;
	float k = max(params.slice_curve.z, 0.001);
	float slices = params.slice_curve.w;
	float linear_z = clamp((view_z - near) / max(far - near, 0.001), 0.0, 1.0);
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
	float view_z = -view_pos.z;
	if (view_z <= 0.0) {
		return;
	}

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

	float extinction = -log(1.0 - alpha);
	uint packed = uint(clamp(extinction * 65536.0, 0.0, 4.29e9));
	imageAtomicAdd(extinction_buffer, ivec3(froxel), packed);
}
