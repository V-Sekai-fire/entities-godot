// Standalone check for oit_splat_math.h. No engine build needed:
//   c++ -std=c++17 -O1 -Wall -Wextra misc/scripts/oit_splat_check.cpp -I. -o /tmp/oit_splat_check && /tmp/oit_splat_check
// Exit 0 only when the atlas placement is a bijection, float accumulation matches the atomicAdd reference,
// and every planted fault is detected.

#include "servers/rendering/renderer_rd/effects/oit_splat_math.h"

#include <algorithm>
#include <cstdio>
#include <vector>

static uint32_t rng_state = 987654321u;
static float rnd01() {
	rng_state = rng_state * 1664525u + 1013904223u;
	return float(rng_state >> 8) / float(1u << 24);
}

// Atlas placement variants. Variant 0 is the model; the others are planted faults.
static void place(int variant, const OITAtlasLayout &l, uint32_t view, uint32_t x, uint32_t y, uint32_t z, uint32_t &tx, uint32_t &ty) {
	switch (variant) {
		case 0:
			oit_atlas_texel(l, view, x, y, z, tx, ty);
			return;
		case 1: // Tile row computed with tiles_y instead of tiles_x: only wrong when the two differ.
			tx = (z % l.tiles_x) * (l.dim_x * l.view_count) + oit_packed_column(l.dim_x, view, x);
			ty = (z / l.tiles_y) * l.dim_y + y;
			return;
		case 2: // Views not offset inside the tile: both eyes land on the same texels.
			tx = (z % l.tiles_x) * (l.dim_x * l.view_count) + x;
			ty = (z / l.tiles_x) * l.dim_y + y;
			return;
	}
}

static bool is_bijection(int variant, const OITAtlasLayout &l) {
	uint32_t w = oit_atlas_width(l), h = oit_atlas_height(l);
	std::vector<uint8_t> seen(size_t(w) * h, 0);
	for (uint32_t v = 0; v < l.view_count; v++) {
		for (uint32_t z = 0; z < l.slices; z++) {
			for (uint32_t y = 0; y < l.dim_y; y++) {
				for (uint32_t x = 0; x < l.dim_x; x++) {
					uint32_t tx, ty;
					place(variant, l, v, x, y, z, tx, ty);
					if (tx >= w || ty >= h) {
						return false;
					}
					uint8_t &s = seen[size_t(ty) * w + tx];
					if (s) {
						return false;
					}
					s = 1;
				}
			}
		}
	}
	return true;
}

struct Splat {
	float u, v, view_z, alpha;
};

int main() {
	int failures = 0;

	// 1. Atlas placement is a bijection for the shapes we expect, and the shape where tiles_x != tiles_y is covered.
	struct {
		uint32_t dx, dy, s, views;
	} shapes[] = { { 40, 24, 64, 1 }, { 40, 24, 128, 1 }, { 40, 24, 128, 2 }, { 32, 32, 100, 2 }, { 64, 36, 128, 1 } };
	bool any_nonsquare = false;
	for (auto &sh : shapes) {
		OITAtlasLayout l = oit_atlas_layout(sh.dx, sh.dy, sh.s, sh.views, 4096);
		any_nonsquare |= (l.tiles_x != l.tiles_y);
		bool ok = is_bijection(0, l);
		std::printf("atlas %ux%u x%u slices, %u view(s): %ux%u tiles, %ux%u texels, bijection %s\n", sh.dx, sh.dy, sh.s, sh.views, l.tiles_x, l.tiles_y, oit_atlas_width(l), oit_atlas_height(l), ok ? "yes" : "NO");
		if (!ok) {
			failures++;
		}
		if (oit_atlas_width(l) > 4096 || oit_atlas_height(l) > 4096) {
			std::printf("FAIL atlas exceeds 4096\n");
			failures++;
		}
	}
	if (!any_nonsquare) {
		std::printf("FAIL no shape had tiles_x != tiles_y, so fault 1 cannot show\n");
		failures++;
	}

	// 2. Planted placement faults must break the bijection on at least one shape.
	for (int variant = 1; variant <= 2; variant++) {
		int caught = 0, total = 0;
		for (auto &sh : shapes) {
			OITAtlasLayout l = oit_atlas_layout(sh.dx, sh.dy, sh.s, sh.views, 4096);
			total++;
			if (!is_bijection(variant, l)) {
				caught++;
			}
		}
		std::printf("planted placement fault %d broke the bijection on %d/%d shapes\n", variant, caught, total);
		if (caught == 0) {
			std::printf("FAIL fault %d never detected\n", variant);
			failures++;
		}
	}

	// 3. Accumulation: float additive blending vs the atomicAdd fixed-point reference, per froxel.
	const uint32_t dx = 40, dy = 24, S = 128;
	const float near_p = 0.1f, far_p = 60.0f, k = 8.0f;
	const int N = 20000;
	std::vector<Splat> splats(N);
	for (auto &s : splats) {
		s.u = rnd01();
		s.v = rnd01();
		s.view_z = 0.05f + rnd01() * 70.0f; // Includes values nearer than near and farther than far.
		s.alpha = rnd01() * 1.1f; // Includes alpha above 0.999.
	}
	std::vector<uint32_t> ref(size_t(dx) * dy * S, 0);
	std::vector<float> acc(size_t(dx) * dy * S, 0.0f);
	std::vector<float> acc_rev(size_t(dx) * dy * S, 0.0f);
	for (int i = 0; i < N; i++) {
		const Splat &s = splats[i];
		OITSplatCell c = oit_splat_cell(dx, dy, S, near_p, far_p, k, s.u, s.v, s.view_z);
		size_t idx = (size_t(c.x) * dy + c.y) * S + c.z;
		ref[idx] += oit_pack_extinction(s.alpha);
		acc[idx] += oit_splat_extinction(s.alpha);
	}
	for (int i = N - 1; i >= 0; i--) {
		const Splat &s = splats[i];
		OITSplatCell c = oit_splat_cell(dx, dy, S, near_p, far_p, k, s.u, s.v, s.view_z);
		acc_rev[(size_t(c.x) * dy + c.y) * S + c.z] += oit_splat_extinction(s.alpha);
	}
	// Each splat is quantised to 1/65536 in the reference, so a froxel with n splats can differ by n / 65536.
	std::vector<uint32_t> count(ref.size(), 0);
	for (int i = 0; i < N; i++) {
		OITSplatCell c = oit_splat_cell(dx, dy, S, near_p, far_p, k, splats[i].u, splats[i].v, splats[i].view_z);
		count[(size_t(c.x) * dy + c.y) * S + c.z]++;
	}
	double worst_ratio = 0.0, worst_order = 0.0;
	uint32_t max_count = 0, occupied = 0;
	for (size_t i = 0; i < ref.size(); i++) {
		if (!count[i]) {
			continue;
		}
		occupied++;
		max_count = std::max(max_count, count[i]);
		double bound = double(count[i]) / 65536.0 + 1e-4 * std::max(1.0, double(acc[i]));
		double err = std::fabs(double(ref[i]) / 65536.0 - double(acc[i]));
		worst_ratio = std::max(worst_ratio, err / bound);
		worst_order = std::max(worst_order, std::fabs(double(acc[i]) - double(acc_rev[i])) / std::max(1.0, double(acc[i])));
	}
	std::printf("accumulation: %u occupied froxels, max %u splats in one froxel, worst error / bound = %.3f, worst order dependence (relative) = %.2g\n", occupied, max_count, worst_ratio, worst_order);
	if (worst_ratio > 1.0) {
		std::printf("FAIL float accumulation exceeds the quantisation bound\n");
		failures++;
	}
	if (worst_order > 1e-5) {
		std::printf("FAIL accumulation depends on draw order beyond float rounding\n");
		failures++;
	}

	// 4. Planted accumulation faults: linear depth instead of the log curve, and alpha not clamped before the log.
	{
		std::vector<float> lin(ref.size(), 0.0f);
		for (int i = 0; i < N; i++) {
			const Splat &s = splats[i];
			OITSplatCell c = oit_splat_cell(dx, dy, S, near_p, far_p, k, s.u, s.v, s.view_z);
			float t = (s.view_z - near_p) / (far_p - near_p);
			t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
			c.z = std::min(uint32_t(t * float(S)), S - 1);
			lin[(size_t(c.x) * dy + c.y) * S + c.z] += oit_splat_extinction(s.alpha);
		}
		size_t diff = 0;
		for (size_t i = 0; i < ref.size(); i++) {
			if (std::fabs(double(lin[i]) - double(acc[i])) > 1e-3) {
				diff++;
			}
		}
		std::printf("planted linear-depth fault changes %zu froxels\n", diff);
		if (diff == 0) {
			std::printf("FAIL linear-depth fault not detected\n");
			failures++;
		}
		float bad = -std::log(1.0f - 1.0f); // Unclamped alpha 1.0 gives infinity.
		std::printf("unclamped alpha=1 would add %g (the model adds %g)\n", double(bad), double(oit_splat_extinction(1.0f)));
		if (!std::isinf(bad) || std::isinf(oit_splat_extinction(1.0f))) {
			std::printf("FAIL alpha clamp not exercised\n");
			failures++;
		}
	}

	std::printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
	return failures ? 1 : 0;
}
