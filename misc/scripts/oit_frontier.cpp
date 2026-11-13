// Frontier-learning harness for the Compatibility (WebGL2) OIT port. No engine build needed:
//   c++ -std=c++17 -O1 -Wall -Wextra misc/scripts/oit_frontier.cpp -I. -o /tmp/oit_frontier && /tmp/oit_frontier
//
// A level is a family of configurations with fixed attributes (target size, froxel tile in pixels, slice count, view count).
// A problem is one instance in it: a WebGL2 MAX_TEXTURE_SIZE (2048 is the guaranteed minimum) and a seed for the splat set.
// A problem is solved when the atlas fits, the texture stays inside the memory budget, and float additive accumulation
// matches the atomicAdd reference. Regret is 1[s >= 1] - s / n. A change stays only when total frontier success rises and no level loses.
// Exit 0 on a clean run; the verdict line says KEEP or REJECT for the candidate against the baseline.

#include "servers/rendering/renderer_rd/effects/oit_splat_math.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static uint32_t rng_state;
static float rnd01() {
	rng_state = rng_state * 1664525u + 1013904223u;
	return float(rng_state >> 8) / float(1u << 24);
}

struct Level {
	const char *target;
	uint32_t w, h, tile_px, slices, views;
};

// Variant 0: one slice per texel (the model as committed). Variant 1: four per texel. Variant 2: planted fault, the integrate pass reads through a different mapping than the splat wrote.
struct Variant {
	const char *name;
	uint32_t spt;
	bool swap_fault;
};

static const uint64_t BUDGET_BYTES = 128ull << 20;
static const uint32_t BYTES_PER_TEXEL = 16; // RGBA32F.

static bool accumulation_ok(const Level &lv, const Variant &v, uint32_t seed) {
	const uint32_t dx = 40, dy = 24, S = lv.slices;
	rng_state = seed;
	const int N = 4000;
	std::vector<uint32_t> idx(N);
	std::vector<float> ext(N);
	std::vector<float> per_froxel(size_t(dx) * dy * S, 0.0f);
	for (int i = 0; i < N; i++) {
		float u = rnd01(), vv = rnd01(), z = 0.05f + rnd01() * 70.0f, a = rnd01() * 1.1f;
		OITSplatCell c = oit_splat_cell(dx, dy, S, 0.1f, 60.0f, 8.0f, u, vv, z);
		idx[i] = (c.x * dy + c.y) * S + c.z;
		ext[i] = oit_splat_extinction(a);
		per_froxel[idx[i]] += ext[i];
	}
	// Packed atlas: texel x channel accumulators, indexed by the variant's placement.
	OITAtlasLayout l = oit_atlas_layout(dx, dy, oit_packed_tile_count(S, v.spt), 1, 4096);
	std::vector<float> atlas(size_t(oit_atlas_width(l)) * oit_atlas_height(l) * v.spt, 0.0f);
	for (int i = 0; i < N; i++) {
		uint32_t z = idx[i] % S, y = (idx[i] / S) % dy, x = idx[i] / S / dy;
		uint32_t tx, ty, ch;
		oit_atlas_texel_packed(l, v.spt, 0, x, y, z, tx, ty, ch);
		atlas[(size_t(ty) * oit_atlas_width(l) + tx) * v.spt + ch] += ext[i];
	}
	for (uint32_t x = 0; x < dx; x++) {
		for (uint32_t y = 0; y < dy; y++) {
			for (uint32_t z = 0; z < S; z++) {
				uint32_t tx, ty, ch;
				if (v.swap_fault) {
					oit_atlas_texel(l, 0, x, y, z % oit_packed_tile_count(S, v.spt), tx, ty);
					ch = std::min(z / oit_packed_tile_count(S, v.spt), v.spt - 1);
				} else {
					oit_atlas_texel_packed(l, v.spt, 0, x, y, z, tx, ty, ch);
				}
				float got = atlas[(size_t(ty) * oit_atlas_width(l) + tx) * v.spt + ch];
				float want = per_froxel[(size_t(x) * dy + y) * S + z];
				if (std::fabs(got - want) > 1e-4f * std::max(1.0f, want)) {
					return false;
				}
			}
		}
	}
	return true;
}

static bool fits(const Level &lv, const Variant &v, uint32_t max_tex) {
	uint32_t dx = (lv.w + lv.tile_px - 1) / lv.tile_px, dy = (lv.h + lv.tile_px - 1) / lv.tile_px;
	if (dx * lv.views > max_tex) {
		return false;
	}
	OITAtlasLayout l = oit_atlas_layout(dx, dy, oit_packed_tile_count(lv.slices, v.spt), lv.views, max_tex);
	if (oit_atlas_width(l) > max_tex || oit_atlas_height(l) > max_tex) {
		return false;
	}
	uint64_t bytes = uint64_t(oit_atlas_width(l)) * oit_atlas_height(l) * BYTES_PER_TEXEL;
	return bytes <= BUDGET_BYTES;
}

struct Result {
	std::vector<int> solved;
	int n = 0;
	int total() const {
		int t = 0;
		for (int s : solved) {
			t += s;
		}
		return t;
	}
};

static Result evaluate(const std::vector<Level> &levels, const Variant &v) {
	const uint32_t max_texes[] = { 2048, 4096, 8192 };
	Result r;
	for (const Level &lv : levels) {
		int s = 0, n = 0;
		for (uint32_t mt : max_texes) {
			for (uint32_t seed = 1; seed <= 5; seed++) {
				n++;
				if (fits(lv, v, mt) && accumulation_ok(lv, v, seed * 7919u + mt)) {
					s++;
				}
			}
		}
		r.solved.push_back(s);
		r.n = n;
	}
	return r;
}

int main() {
	std::vector<Level> levels;
	struct {
		const char *name;
		uint32_t w, h;
	} targets[] = { { "desktop 800x500", 800, 500 }, { "1080p", 1920, 1080 }, { "headset 1680x1760", 1680, 1760 } };
	for (auto &t : targets) {
		for (uint32_t tile : { 6u, 12u, 16u }) {
			for (uint32_t slices : { 64u, 128u }) {
				for (uint32_t views : { 1u, 2u }) {
					levels.push_back({ t.name, t.w, t.h, tile, slices, views });
				}
			}
		}
	}
	Variant base = { "one slice per texel", 1, false };
	Variant cand = { "four slices per texel", 4, false };
	Variant fault = { "planted fault: read mapping differs from write mapping", 4, true };

	Result rb = evaluate(levels, base);
	Result rc = evaluate(levels, cand);
	Result rf = evaluate(levels, fault);

	std::printf("%zu levels x %d problems each (3 MAX_TEXTURE_SIZE x 5 seeds); budget %llu MiB at %u B/texel\n\n", levels.size(), rb.n, (unsigned long long)(BUDGET_BYTES >> 20), BYTES_PER_TEXEL);
	std::printf("%-18s %4s %6s %6s | %-9s %-9s | regret(base)\n", "target", "tile", "slices", "views", "base s/n", "cand s/n");
	int regressed = 0;
	for (size_t i = 0; i < levels.size(); i++) {
		const Level &lv = levels[i];
		float regret = (rb.solved[i] >= 1 ? 1.0f : 0.0f) - float(rb.solved[i]) / float(rb.n);
		if (rc.solved[i] < rb.solved[i]) {
			regressed++;
		}
		if (regret > 0.0f || rc.solved[i] != rb.solved[i]) {
			std::printf("%-18s %4u %6u %6u | %2d/%-6d %2d/%-6d | %.2f\n", lv.target, lv.tile_px, lv.slices, lv.views, rb.solved[i], rb.n, rc.solved[i], rb.n, regret);
		}
	}
	int N = int(levels.size()) * rb.n;
	std::printf("\nfrontier success: base %d/%d, candidate %d/%d, planted fault %d/%d\n", rb.total(), N, rc.total(), N, rf.total(), N);
	bool keep = rc.total() > rb.total() && regressed == 0;
	std::printf("candidate '%s': levels that lost problems = %d -> %s\n", cand.name, regressed, keep ? "KEEP" : "REJECT");
	bool fault_rejected = !(rf.total() > rb.total());
	std::printf("planted fault is %s by the same rule\n", fault_rejected ? "REJECTED" : "ACCEPTED (the rule cannot fail)");
	if (!fault_rejected || !keep) {
		return 1;
	}
	return 0;
}
