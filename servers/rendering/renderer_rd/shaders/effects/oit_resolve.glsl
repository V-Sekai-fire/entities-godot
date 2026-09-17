#[vertex]

#version 450

#VERSION_DEFINES

#ifdef USE_MULTIVIEW
#extension GL_EXT_multiview : enable
#endif

void main() {
	vec2 base_arr[3] = vec2[](vec2(-1.0, -1.0), vec2(-1.0, 3.0), vec2(3.0, -1.0));
	gl_Position = vec4(base_arr[gl_VertexIndex], 0.0, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

#ifdef USE_MULTIVIEW
#extension GL_EXT_multiview : enable
#define ViewIndex gl_ViewIndex
#ifdef USE_MULTISAMPLE
layout(set = 0, binding = 0) uniform sampler2DMSArray accumulated_color;
layout(set = 0, binding = 1) uniform sampler2DMSArray accumulated_extinction;
#else
layout(set = 0, binding = 0) uniform sampler2DArray accumulated_color;
layout(set = 0, binding = 1) uniform sampler2DArray accumulated_extinction;
#endif
#else
#define ViewIndex 0
#ifdef USE_MULTISAMPLE
layout(set = 0, binding = 0) uniform sampler2DMS accumulated_color;
layout(set = 0, binding = 1) uniform sampler2DMS accumulated_extinction;
#else
layout(set = 0, binding = 0) uniform sampler2D accumulated_color;
layout(set = 0, binding = 1) uniform sampler2D accumulated_extinction;
#endif
#endif

#ifdef USE_MULTISAMPLE
layout(push_constant, std430) uniform Params {
	uint sample_count;
	uint pad[3];
}
params;
#endif

layout(location = 0) out vec4 frag_color;

// Composites the weighted transparency over the background already in the framebuffer:
// C = accum.rgb * (1 - T) / accum.a + background * T, with T = exp(-extinction).
vec4 resolve_sample(vec4 accum, float extinction) {
	float transmittance = exp(-extinction);
	vec3 color = accum.a > 0.0 ? accum.rgb * (1.0 - transmittance) / accum.a : vec3(0.0);
	return vec4(color, 1.0 - transmittance);
}

void main() {
#ifdef USE_MULTIVIEW
	ivec3 coord = ivec3(gl_FragCoord.xy, ViewIndex);
#else
	ivec2 coord = ivec2(gl_FragCoord.xy);
#endif
#ifdef USE_MULTISAMPLE
	// The mean of the per-sample resolves, blended once over the pixel.
	vec4 sum = vec4(0.0);
	for (uint s = 0; s < params.sample_count; s++) {
		sum += resolve_sample(texelFetch(accumulated_color, coord, int(s)), texelFetch(accumulated_extinction, coord, int(s)).r);
	}
	frag_color = sum / float(params.sample_count);
#else
	frag_color = resolve_sample(texelFetch(accumulated_color, coord, 0), texelFetch(accumulated_extinction, coord, 0).r);
#endif
}
