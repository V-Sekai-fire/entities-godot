#[vertex]

#version 450

#VERSION_DEFINES

struct VertexData {
	vec3 position;
	float _pad0;
};

struct InstanceData {
	vec4 albedo;
	mat4 world;
};

layout(set = 0, binding = 0, std140) uniform CameraUBO {
	mat4 projection;
	mat4 view;
	mat4 inv_view;
	vec4 viewport; // xy=size, zw=inv_size
	float average_depth_complexity;
	float _pad0;
	float _pad1;
	float _pad2;
}
camera;

layout(set = 0, binding = 1, std430) readonly buffer VertexBuffer {
	VertexData data[];
}
vertices;

layout(set = 0, binding = 2, std430) readonly buffer InstanceBuffer {
	InstanceData data[];
}
instances;

layout(location = 0) out vec4 out_color;

void main() {
	VertexData v = vertices.data[gl_VertexIndex];
	InstanceData inst = instances.data[gl_InstanceIndex];

	vec4 world_pos = inst.world * vec4(v.position, 1.0);
	gl_Position = camera.projection * camera.view * world_pos;

	out_color = inst.albedo;
}

#[fragment]

#version 450

#VERSION_DEFINES

layout(set = 0, binding = 0, std140) uniform CameraUBO {
	mat4 projection;
	mat4 view;
	mat4 inv_view;
	vec4 viewport;
	float average_depth_complexity;
	float _pad0;
	float _pad1;
	float _pad2;
}
camera;

layout(set = 0, binding = 3) uniform sampler2D scene_depth;

layout(location = 0) in vec4 in_color;

layout(location = 0) out vec4 accum;
layout(location = 1) out float revealage;

void main() {
	float opaque_z = texelFetch(scene_depth, ivec2(gl_FragCoord.xy), 0).r;
	if (opaque_z > gl_FragCoord.z) {
		discard;
	}

	float a = in_color.a;
	float depth_term = 1.0 + gl_FragCoord.z * 0.1 * camera.average_depth_complexity;
	float w = clamp(pow(a + 0.01, 3.0) * 32.0 / depth_term, 0.01, 32.0);

	accum = vec4(in_color.rgb * a, a) * w;
	revealage = a;
}
