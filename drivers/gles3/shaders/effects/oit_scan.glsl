/* clang-format off */
#[modes]

mode_local = #define MODE_LOCAL
mode_tiles = #define MODE_TILES
mode_carry = #define MODE_CARRY

#[specializations]

#[vertex]

layout(location = 0) in vec2 vertex_attrib;
/* clang-format on */

void main() {
	gl_Position = vec4(vertex_attrib, 1.0, 1.0);
}

/* clang-format off */
#[fragment]

layout(location = 0) out highp vec4 frag_color;
/* clang-format on */

// Packed scan, four slices per texel; model and checks in oit_scan_math.h (oit_scan_packed_*).
// Tiles of a column sit side by side in x, grid_width apart.

uniform highp sampler2D source;
uniform highp int grid_width;
uniform highp int stride;

void main() {
	ivec2 p = ivec2(gl_FragCoord.xy);
	int tile = p.x / grid_width;
	vec4 v = texelFetch(source, p, 0);

#ifdef MODE_LOCAL
	v.y += v.x;
	v.z += v.y;
	v.w += v.z;
#endif

#ifdef MODE_TILES
	if (tile >= stride) {
		v.w += texelFetch(source, ivec2(p.x - stride * grid_width, p.y), 0).w;
	}
#endif

#ifdef MODE_CARRY
	if (tile > 0) {
		float carry = texelFetch(source, ivec2(p.x - grid_width, p.y), 0).w;
		v.xyz += vec3(carry);
	}
#endif

	frag_color = v;
}
