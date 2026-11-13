#pragma once

#include "servers/rendering/renderer_rd/effects/oit_math.h"

#include <cmath>
#include <cstdint>

// Models the splat step of the froxel OIT for the Compatibility (GLES3 / WebGL2) renderer.
// The RD version does atomicAdd(extinction[flat_index], oit_pack_extinction(alpha)) from a compute shader.
// WebGL2 cannot scatter from a fragment, so each splat is drawn as a GL point: the vertex shader places it
// on one texel of a 2D atlas and additive blending sums the extinction. The atlas holds one tile per depth slice.

struct OITAtlasLayout {
	uint32_t dim_x = 0; // Froxels per tile along X, for one view.
	uint32_t dim_y = 0;
	uint32_t slices = 0;
	uint32_t view_count = 1;
	uint32_t tiles_x = 0; // Tiles per atlas row.
	uint32_t tiles_y = 0;
};

// Smallest near-square tile grid that holds p_slices tiles, within p_max_texture_size.
inline OITAtlasLayout oit_atlas_layout(uint32_t p_dim_x, uint32_t p_dim_y, uint32_t p_slices, uint32_t p_view_count, uint32_t p_max_texture_size) {
	OITAtlasLayout l;
	l.dim_x = p_dim_x;
	l.dim_y = p_dim_y;
	l.slices = p_slices;
	l.view_count = p_view_count;
	uint32_t row_cap = p_max_texture_size / (p_dim_x * p_view_count);
	l.tiles_x = 1;
	while (l.tiles_x * l.tiles_x < p_slices && l.tiles_x < row_cap) {
		l.tiles_x++;
	}
	l.tiles_y = (p_slices + l.tiles_x - 1) / l.tiles_x;
	return l;
}

inline uint32_t oit_atlas_width(const OITAtlasLayout &p_l) {
	return p_l.tiles_x * p_l.dim_x * p_l.view_count;
}

inline uint32_t oit_atlas_height(const OITAtlasLayout &p_l) {
	return p_l.tiles_y * p_l.dim_y;
}

// Texel of froxel (view, x, y, z). Views sit side by side inside a tile, as oit_packed_column packs them in the RD buffer.
inline void oit_atlas_texel(const OITAtlasLayout &p_l, uint32_t p_view, uint32_t p_x, uint32_t p_y, uint32_t p_z, uint32_t &r_tx, uint32_t &r_ty) {
	uint32_t tile_col = p_z % p_l.tiles_x;
	uint32_t tile_row = p_z / p_l.tiles_x;
	r_tx = tile_col * (p_l.dim_x * p_l.view_count) + oit_packed_column(p_l.dim_x, p_view, p_x);
	r_ty = tile_row * p_l.dim_y + p_y;
}

// The froxel one splat falls in. Mirrors the placement in oit_voxelize.glsl: uv clamped into the grid, depth through the log curve.
struct OITSplatCell {
	uint32_t x, y, z;
};

inline OITSplatCell oit_splat_cell(uint32_t p_dim_x, uint32_t p_dim_y, uint32_t p_slices, float p_near, float p_far, float p_k, float p_u, float p_v, float p_view_z) {
	OITSplatCell c;
	c.x = uint32_t(p_u * float(p_dim_x));
	c.y = uint32_t(p_v * float(p_dim_y));
	c.z = oit_depth_to_slice(p_near, p_far, p_k, p_slices, p_view_z);
	if (c.x > p_dim_x - 1) {
		c.x = p_dim_x - 1;
	}
	if (c.y > p_dim_y - 1) {
		c.y = p_dim_y - 1;
	}
	return c;
}

// Extinction a splat adds, in float (the blend unit's view). The RD buffer holds oit_pack_extinction(alpha) / 65536.
inline float oit_splat_extinction(float p_alpha) {
	float a = p_alpha < 0.0f ? 0.0f : (p_alpha > 0.999f ? 0.999f : p_alpha);
	return -std::log(1.0f - a);
}
