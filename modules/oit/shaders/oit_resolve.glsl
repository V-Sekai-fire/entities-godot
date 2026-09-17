#[vertex]

#version 450

#VERSION_DEFINES

void main() {
	vec2 base_arr[3] = vec2[](vec2(-1.0, -1.0), vec2(-1.0, 3.0), vec2(3.0, -1.0));
	gl_Position = vec4(base_arr[gl_VertexIndex], 0.0, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

layout(set = 0, binding = 0) uniform sampler2D accumulated_color;
layout(set = 0, binding = 1) uniform sampler2D accumulated_extinction;

layout(location = 0) out vec4 frag_color;

// Composites the weighted transparency over the background already in the framebuffer:
// C = accum.rgb * (1 - T) / accum.a + background * T, with T = exp(-extinction).
void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);
	vec4 accum = texelFetch(accumulated_color, coord, 0);
	float extinction = texelFetch(accumulated_extinction, coord, 0).r;
	float transmittance = exp(-extinction);
	vec3 color = accum.a > 0.0 ? accum.rgb * (1.0 - transmittance) / accum.a : vec3(0.0);
	frag_color = vec4(color, 1.0 - transmittance);
}
