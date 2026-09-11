#[versions]

default = "";
msaa = "#define USE_MSAA";

#[vertex]

#version 450

#VERSION_DEFINES

void main() {
	vec2 pos = vec2(
			(gl_VertexIndex == 1) ? 3.0 : -1.0,
			(gl_VertexIndex == 2) ? 3.0 : -1.0);
	gl_Position = vec4(pos, 0.0, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

layout(push_constant, std430) uniform Params {
	float ldr_scale;
	float _pad0;
	float _pad1;
	float _pad2;
}
params;

#ifdef USE_MSAA
layout(set = 0, binding = 0) uniform sampler2DMS accum_ms;
layout(set = 0, binding = 1) uniform sampler2DMS revealage_ms;
#else
layout(set = 0, binding = 0) uniform sampler2D accum;
layout(set = 0, binding = 1) uniform sampler2D revealage;
#endif

layout(location = 0) out vec4 out_color;

void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);

#ifdef USE_MSAA
	vec4 a = texelFetch(accum_ms, coord, gl_SampleID);
	float r = texelFetch(revealage_ms, coord, gl_SampleID).r;
#else
	vec4 a = texelFetch(accum, coord, 0);
	float r = texelFetch(revealage, coord, 0).r;
#endif

	vec3 rgb = a.rgb / max(a.a, 1e-6);
	float visibility = 1.0 - r;
	out_color = vec4(rgb * visibility * params.ldr_scale, visibility);
}
