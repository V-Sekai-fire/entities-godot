// Compares two ways to get order-independent transparency without compute shaders. No engine build needed:
//   c++ -std=c++17 -O1 -Wall -Wextra misc/scripts/oit_compare.cpp -I. -o /tmp/oit_compare && /tmp/oit_compare
//
// Answer key: every fragment of a pixel sorted far to near and composited with "over" (exact).
//   froxel : the engine's design (oit_math.h): slice extinction, prefix scan, transmittance lookup, weighted resolve.
//   sorted : depth peeling, the fragment-only way to build a per-pixel sorted list. Keeps the nearest K layers, drops the rest.
// A level is (fragments per pixel, alpha profile, depth spread). A problem is one pixel. Solved when |error| <= 0.01.
// Controls: the answer key against itself, froxel without the transmittance lookup, and a peel with K = 1. All three must fail.
// Cost is a model (bytes and pass counts), not a GPU measurement.

#include "servers/rendering/renderer_rd/effects/oit_math.h"
#include "servers/rendering/renderer_rd/effects/oit_scan_math.h"
#include "servers/rendering/renderer_rd/effects/oit_splat_math.h"

#include <algorithm>
#include <cstdio>
#include <vector>

struct Frag {
	float z, a, c;
};

static uint32_t rng_state = 20261112u;
static float rnd01() {
	rng_state = rng_state * 1664525u + 1013904223u;
	return float(rng_state >> 8) / float(1u << 24);
}

static float over_far_to_near(std::vector<Frag> f, float bg) {
	std::sort(f.begin(), f.end(), [](const Frag &x, const Frag &y) { return x.z > y.z; });
	float out = bg;
	for (const Frag &g : f) {
		out = g.c * g.a + out * (1.0f - g.a);
	}
	return out;
}

static float sorted_peel(std::vector<Frag> f, float bg, size_t k) {
	std::sort(f.begin(), f.end(), [](const Frag &x, const Frag &y) { return x.z < y.z; });
	if (f.size() > k) {
		f.resize(k);
	}
	return over_far_to_near(f, bg);
}

// Weighted blended OIT (McGuire and Bavoil 2013): no sort, one pass, two targets.
//   accum = sum(c * a * w), weight = sum(a * w), reveal = prod(1 - a); out = accum / weight * (1 - reveal) + bg * reveal.
// w from the paper's depth weight (eq. 10 form); p_flat uses w = 1 as a control for the depth weight.
static float wboit(const std::vector<Frag> &f, float bg, bool p_flat) {
	float accum = 0.0f, wsum = 0.0f, reveal = 1.0f;
	for (const Frag &g : f) {
		float zr = g.z / 200.0f;
		float dw = 0.03f / (1e-5f + zr * zr * zr * zr);
		dw = dw < 1e-2f ? 1e-2f : (dw > 3e3f ? 3e3f : dw);
		float w = p_flat ? 1.0f : g.a * dw;
		accum += g.c * g.a * w;
		wsum += g.a * w;
		reveal *= (1.0f - g.a);
	}
	float avg = wsum > 1e-5f ? accum / wsum : 0.0f;
	return avg * (1.0f - reveal) + bg * reveal;
}

static const uint32_t SLICES = 128;
static const float NEAR_P = 0.1f, FAR_P = 60.0f, CURVE_K = 8.0f;

static float froxel_range(const std::vector<Frag> &f, float bg, bool use_lookup, float r_near, float r_far, float r_k) {
	std::vector<float> col(SLICES, 0.0f), scratch(SLICES), trans(SLICES);
	std::vector<uint32_t> slice(f.size());
	for (size_t i = 0; i < f.size(); i++) {
		slice[i] = oit_depth_to_slice(r_near, r_far, r_k, SLICES, f[i].z);
		col[slice[i]] += oit_splat_extinction(f[i].a);
	}
	oit_scan_column(col.data(), scratch.data(), SLICES);
	for (uint32_t z = 0; z < SLICES; z++) {
		trans[z] = oit_scan_transmittance(col[z]);
	}
	OITAccum acc;
	for (size_t i = 0; i < f.size(); i++) {
		float tf = use_lookup ? oit_lookup_transmittance(trans.data(), slice[i]) : 1.0f;
		oit_accumulate(acc, f[i].c, f[i].a, tf);
	}
	return oit_resolve(acc, bg);
}

static float froxel(const std::vector<Frag> &f, float bg, bool use_lookup) {
	return froxel_range(f, bg, use_lookup, NEAR_P, FAR_P, CURVE_K);
}

// The engine's auto-range (rendering/oit/auto_range, margin 0.02 m): bounds of the translucent geometry, from the scene's
// depth extent (the engine takes them from the instances' world bounds, so the scene's own spread stands in for them).
static float froxel_auto(const std::vector<Frag> &f, float bg, bool use_lookup, float scene_lo, float scene_hi) {
	float n, fa, k;
	if (!oit_auto_range(scene_lo, scene_hi, 0.02f, n, fa, k)) {
		return froxel(f, bg, use_lookup);
	}
	return froxel_range(f, bg, use_lookup, n, fa, k);
}

struct Profile {
	const char *name;
	float lo, hi;
};
struct Spread {
	const char *name;
	float lo, hi;
};

int main() {
	const int counts[] = { 2, 4, 8, 16, 32 };
	const Profile profiles[] = { { "low a .05-.2 (hair)", 0.05f, 0.2f }, { "mid a .2-.6", 0.2f, 0.6f }, { "high a .6-.95 (glass)", 0.6f, 0.95f } };
	const Spread spreads[] = { { "wide", 0.5f, 50.0f }, { "mid", 10.0f, 14.0f }, { "tight", 10.0f, 10.2f } };
	const int SEEDS = 100;
	const float TOL = 0.01f;

	enum { A_FROXEL, CTRL_NOLOOKUP, B_K4, B_K8, B_K16, CTRL_K1, C_WBOIT, C_FLAT, A_AUTO, CTRL_AUTO_NL, METHODS };
	const char *names[METHODS] = { "froxel", "ctrl: froxel, no lookup", "sorted K=4", "sorted K=8", "sorted K=16", "ctrl: sorted K=1", "weighted blended (no sort)", "weighted blended, w=1", "froxel + auto-range", "ctrl: auto-range, no lookup" };
	int solved_total[METHODS] = {};
	double err_sum[METHODS] = {}, err_max[METHODS] = {};
	int n_total = 0;
	int truth_fail = 0;

	std::printf("%-5s %-22s | %7s %7s %7s %7s %7s %7s | solved of 300 (3 spreads x 100 pixels)\n", "frags", "alpha profile", "froxel", "fx+auto", "sorted4", "sorted8", "sorted16", "wboit");
	// Rows by fragments x alpha; the same pixels also feed the per-spread table.
	int by_spread[3][METHODS] = {};
	for (int n : counts) {
		for (const Profile &p : profiles) {
			int row[METHODS] = {};
			for (int si = 0; si < 3; si++) {
				for (int s = 0; s < SEEDS; s++) {
					std::vector<Frag> f(n);
					for (Frag &g : f) {
						g.z = spreads[si].lo + rnd01() * (spreads[si].hi - spreads[si].lo);
						g.a = p.lo + rnd01() * (p.hi - p.lo);
						g.c = rnd01();
					}
					float bg = rnd01();
					float want = over_far_to_near(f, bg);
					if (std::fabs(sorted_peel(f, bg, size_t(n)) - want) > 1e-6f) {
						truth_fail++;
					}
					float got[METHODS];
					got[A_FROXEL] = froxel(f, bg, true);
					got[CTRL_NOLOOKUP] = froxel(f, bg, false);
					got[B_K4] = sorted_peel(f, bg, 4);
					got[B_K8] = sorted_peel(f, bg, 8);
					got[B_K16] = sorted_peel(f, bg, 16);
					got[CTRL_K1] = sorted_peel(f, bg, 1);
					got[C_WBOIT] = wboit(f, bg, false);
					got[C_FLAT] = wboit(f, bg, true);
					got[A_AUTO] = froxel_auto(f, bg, true, spreads[si].lo, spreads[si].hi);
					got[CTRL_AUTO_NL] = froxel_auto(f, bg, false, spreads[si].lo, spreads[si].hi);
					for (int m = 0; m < METHODS; m++) {
						double e = std::fabs(double(got[m]) - double(want));
						err_sum[m] += e;
						err_max[m] = std::max(err_max[m], e);
						if (e <= TOL) {
							row[m]++;
							by_spread[si][m]++;
							solved_total[m]++;
						}
					}
					n_total++;
				}
			}
			std::printf("%-5d %-22s | %7d %7d %7d %7d %7d %7d |\n", n, p.name, row[A_FROXEL], row[A_AUTO], row[B_K4], row[B_K8], row[B_K16], row[C_WBOIT]);
		}
	}
	std::printf("\nby depth spread (solved of 1500 each):\n%-8s | %7s %7s %7s %7s %7s %7s\n", "spread", "froxel", "fx+auto", "sorted4", "sorted8", "sorted16", "wboit");
	for (int si = 0; si < 3; si++) {
		std::printf("%-8s | %7d %7d %7d %7d %7d %7d\n", spreads[si].name, by_spread[si][A_FROXEL], by_spread[si][A_AUTO], by_spread[si][B_K4], by_spread[si][B_K8], by_spread[si][B_K16], by_spread[si][C_WBOIT]);
	}
	std::printf("\n%-26s %8s %12s %12s\n", "method", "solved", "mean |err|", "max |err|");
	for (int m = 0; m < METHODS; m++) {
		std::printf("%-26s %4d/%d %12.5f %12.5f\n", names[m], solved_total[m], n_total, err_sum[m] / n_total, err_max[m]);
	}

	// Cost model at headset size: 1680 x 1760 per eye, 2 eyes, 6 px froxel tile, 128 slices.
	const uint64_t W = 1680, H = 1760, V = 2, PIX = W * H * V;
	uint32_t dx = (1680 + 5) / 6, dy = (1760 + 5) / 6;
	OITAtlasLayout l = oit_atlas_layout(dx, dy, oit_packed_tile_count(SLICES, 4), V, 8192);
	uint64_t atlas = uint64_t(oit_atlas_width(l)) * oit_atlas_height(l) * 16; // RGBA32F, four slices a texel.
	uint64_t accum = PIX * (8 + 4); // RGBA16F colour+alpha and R32F extinction.
	uint64_t froxel_bytes = atlas * 2 + accum; // The scan ping-pongs between two atlases.
	std::printf("\ncost model, headset %llux%llu x%llu eyes:\n", (unsigned long long)W, (unsigned long long)H, (unsigned long long)V);
	std::printf("  froxel : atlas %ux%u texels = %.0f MiB, x2 for the scan, + accumulate targets %.0f MiB => %.0f MiB; transparent geometry drawn 2x (1 depth-only splat, 1 shaded); shaded fragments ~1x\n", oit_atlas_width(l), oit_atlas_height(l), double(atlas) / 1048576.0, double(accum) / 1048576.0, double(froxel_bytes) / 1048576.0);
	for (int k : { 4, 8, 16 }) {
		uint64_t b = PIX * 8 * uint64_t(k); // RGBA8 colour + R32F depth per layer.
		std::printf("  sorted K=%-2d: %llu layers x (RGBA8 + R32F) = %.0f MiB; transparent geometry drawn %dx, all shaded (upper bound; early-z may cut it); shaded fragments up to ~%dx\n", k, (unsigned long long)k, double(b) / 1048576.0, k, k);
	}

	std::printf("  weighted blended: RGBA16F accum + R8 reveal = %.0f MiB; transparent geometry drawn 1x, shaded 1x; no atlas, no scan\n", double(PIX * (8 + 1)) / 1048576.0);

	bool ok = true;
	if (truth_fail) {
		std::printf("FAIL answer key disagrees with a full-depth peel on %d pixels\n", truth_fail);
		ok = false;
	}
	if (!(solved_total[CTRL_NOLOOKUP] < solved_total[A_FROXEL]) || !(solved_total[CTRL_K1] < solved_total[B_K4]) || !(solved_total[CTRL_AUTO_NL] < solved_total[A_AUTO])) {
		std::printf("FAIL a control did not score worse than its method, so the comparison cannot tell good from bad\n");
		ok = false;
	}
	std::printf(ok ? "RESULT: PASS (controls fail as they must)\n" : "RESULT: FAIL\n");
	return ok ? 0 : 1;
}
