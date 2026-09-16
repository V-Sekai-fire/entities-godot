#[compute]

#version 450

#VERSION_DEFINES

// AVBOIT integration: one workgroup per (x, y) column, Hillis-Steele
// prefix sum along Z, single workgroup avoids cross-workgroup barriers.

layout(local_size_x = 1, local_size_y = 1, local_size_z = 128) in;

layout(r32ui, set = 0, binding = 0) uniform restrict readonly uimage3D extinction_buffer;
layout(rgba8, set = 0, binding = 1) uniform restrict writeonly image3D integrated_buffer;

layout(set = 0, binding = 2, std140) uniform Params {
	uvec4 froxel_dims;
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

	uint raw = imageLoad(extinction_buffer, ivec3(xy, int(z))).x;
	shared_extinction[z] = float(raw) / 65536.0;
	memoryBarrierShared();
	barrier();

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

	float transmittance = exp(-shared_extinction[z]);
	imageStore(integrated_buffer, ivec3(xy, int(z)),
			vec4(transmittance, transmittance, transmittance, transmittance));
}
