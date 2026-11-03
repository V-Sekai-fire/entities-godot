#[compute]

#version 450

#VERSION_DEFINES

// One workgroup per (x, y) column. The 128 threads each integrate up to four consecutive slices, so a column can hold up to 512
// (Metal caps local_size_z below 128, so Z cannot be the thread index and 128 threads on X were the old slice limit).
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0, std430) restrict readonly buffer Extinction {
	uint data[];
}
extinction;
layout(rgba8, set = 0, binding = 1) uniform restrict writeonly image3D integrated_buffer;

layout(set = 0, binding = 2, std140) uniform Params {
	uvec4 froxel_dims;
}
params;

shared float shared_sums[128];

void main() {
	ivec2 xy = ivec2(gl_WorkGroupID.xy);
	if (xy.x >= int(params.froxel_dims.x) || xy.y >= int(params.froxel_dims.y)) {
		return;
	}

	uint t = gl_LocalInvocationID.x;
	uint slice_count = params.froxel_dims.z;
	uint per_thread = (slice_count + 127u) / 128u; // 1 to 4
	uint first = t * per_thread;
	uint column = (uint(xy.x) * params.froxel_dims.y + uint(xy.y)) * slice_count;

	float vals[4];
	float local_sum = 0.0;
	for (uint i = 0u; i < 4u; i++) {
		float v = 0.0;
		if (i < per_thread && first + i < slice_count) {
			v = float(extinction.data[column + first + i]) / 65536.0;
		}
		vals[i] = v;
		local_sum += v;
	}

	// Inclusive scan of the per-thread sums, then each thread walks its own slices from the sum of the threads before it.
	shared_sums[t] = local_sum;
	memoryBarrierShared();
	barrier();
	for (uint stride = 1u; stride < 128u; stride <<= 1u) {
		float partner = 0.0;
		if (t >= stride) {
			partner = shared_sums[t - stride];
		}
		barrier();
		if (t >= stride) {
			shared_sums[t] += partner;
		}
		barrier();
	}

	float running = shared_sums[t] - local_sum;
	for (uint i = 0u; i < 4u; i++) {
		if (i < per_thread && first + i < slice_count) {
			running += vals[i];
			float transmittance = exp(-running);
			imageStore(integrated_buffer, ivec3(xy, int(first + i)), vec4(transmittance));
		}
	}
}
