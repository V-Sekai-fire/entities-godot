// Single auto-range against the engine's depth intervals, for scenes of several translucent instances. No engine build needed:
//   c++ -std=c++17 -O1 -Wall -Wextra misc/scripts/oit_compare_intervals.cpp -I. -o /tmp/oit_compare_intervals && /tmp/oit_compare_intervals
//
// A scene is N instances, each a depth band with an AABB looser than its fragments (the engine takes bounds from instance AABBs).
// Every instance holds a few fragments. Answer key: all fragments sorted far to near, composited with "over".
//   single   : one oit_auto_range over the union of the bands (what iteration 4 measured).
//   intervals: OITIntervalSet as render_forward_mobile.cpp:3911-3935 builds it, slices shared out by oit_interval_ranges, looked up with oit_interval_slice.
// Both use 128 slices. A problem is one pixel; solved when |error| <= 0.01.
// Controls: both with no transmittance lookup must score worse; intervals fed one merged band must equal single.

#include "servers/rendering/renderer_rd/effects/oit_math.h"
#include "servers/rendering/renderer_rd/effects/oit_scan_math.h"
#include "servers/rendering/renderer_rd/effects/oit_splat_math.h"

#include <algorithm>
#include <cstdio>
#include <vector>

struct Frag {
	float z, a, c;
};

static uint32_t rng_state = 20261113u;
static float rnd01() {
	rng_state = rng_state * 1664525u + 1013904223u;
	return float(rng_state >> 8) / float(1u << 24);
}

static float truth(std::vector<Frag> f, float bg) {
	std::sort(f.begin(), f.end(), [](const Frag &x, const Frag &y) { return x.z > y.z; });
	float o = bg;
	for (const Frag &g : f) {
		o = g.c * g.a + o * (1.0f - g.a);
	}
	return o;
}

static const int SLICES = 128;

// slice_of: returns a fractional slice position for a depth.
template <typename F>
static float run(const std::vector<Frag> &f, float bg, bool lookup, F slice_of) {
	std::vector<float> col(SLICES, 0.0f), scratch(SLICES), trans(SLICES);
	std::vector<uint32_t> sl(f.size());
	for (size_t i = 0; i < f.size(); i++) {
		float p = slice_of(f[i].z);
		uint32_t s = uint32_t(p < 0.0f ? 0.0f : (p > float(SLICES - 1) ? float(SLICES - 1) : p));
		sl[i] = s;
		col[s] += oit_splat_extinction(f[i].a);
	}
	oit_scan_column(col.data(), scratch.data(), SLICES);
	for (int z = 0; z < SLICES; z++) {
		trans[z] = oit_scan_transmittance(col[z]);
	}
	OITAccum acc;
	for (size_t i = 0; i < f.size(); i++) {
		oit_accumulate(acc, f[i].c, f[i].a, lookup ? oit_lookup_transmittance(trans.data(), sl[i]) : 1.0f);
	}
	return oit_resolve(acc, bg);
}

struct Band {
	float lo, hi; // Where the fragments are.
};

int main() {
	struct Layout {
		const char *name;
		std::vector<Band> bands;
	};
	const Layout layouts[] = {
		{ "1 band (hair)", { { 10.0f, 10.2f } } },
		{ "2 bands, 3 m apart", { { 5.0f, 5.3f }, { 8.0f, 8.3f } } },
		{ "3 bands, 4-20 m", { { 4.0f, 4.3f }, { 9.0f, 9.3f }, { 20.0f, 20.3f } } },
		{ "5 bands, 1.5-40 m", { { 1.5f, 1.7f }, { 4.0f, 4.3f }, { 10.0f, 10.3f }, { 22.0f, 22.4f }, { 40.0f, 40.5f } } },
		{ "9 bands (merge cap 8)", { { 1.0f, 1.1f }, { 2.0f, 2.1f }, { 3.2f, 3.3f }, { 5.0f, 5.1f }, { 7.0f, 7.1f }, { 10.0f, 10.1f }, { 14.0f, 14.1f }, { 20.0f, 20.1f }, { 30.0f, 30.1f } } },
	};
	struct Pad {
		const char *name;
		float frac, abs;
	};
	const Pad pads[] = { { "exact", 0.0f, 0.0f }, { "+-25% +-0.1m", 0.25f, 0.1f }, { "+-100% +-2m", 1.0f, 2.0f } };
	const float alphas[][2] = { { 0.05f, 0.2f }, { 0.2f, 0.6f }, { 0.6f, 0.95f } };
	const int frags_per_band[] = { 2, 6 };
	const int SEEDS = 100;
	const float MARGIN = 0.02f;
	const float TOL = 0.01f;

	int tot[2] = {}, tot_nl[2] = {}, n_all = 0, mismatch = 0;
	std::printf("%-24s %-14s | %6s %9s | %6s %9s  (of %d)\n", "layout", "AABB padding", "single", "intervals", "s-nolk", "i-nolk", 2 * 3 * SEEDS);
	for (const Layout &L : layouts) {
		for (const Pad &P : pads) {
			int solved[2] = {}, solved_nl[2] = {}, n = 0;
			for (int fb : frags_per_band) {
				for (const auto &al : alphas) {
					for (int s = 0; s < SEEDS; s++) {
						std::vector<Frag> f;
						OITIntervalSet set;
						float lo_all = 1e30f, hi_all = -1e30f;
						for (const Band &b : L.bands) {
							for (int i = 0; i < fb; i++) {
								f.push_back({ b.lo + rnd01() * (b.hi - b.lo), al[0] + rnd01() * (al[1] - al[0]), rnd01() });
							}
							float w = b.hi - b.lo, pad = P.frac * w + P.abs;
							float s_lo = b.lo - pad, s_hi = b.hi + pad;
							if (s_lo < 0.0f) {
								s_lo = 0.0f;
							}
							lo_all = std::min(lo_all, s_lo);
							hi_all = std::max(hi_all, s_hi);
							set.add(s_lo - std::max(MARGIN, s_lo * 2.0e-6f), s_hi + std::max(MARGIN, s_hi * 2.0e-6f));
						}
						float bg = rnd01(), want = truth(f, bg);

						float nr, fr, k;
						oit_auto_range(lo_all, hi_all, MARGIN, nr, fr, k);
						auto single = [&](float z) { return float(SLICES) * oit_slice_uv(nr, fr, k, z); };
						float ranges[OIT_MAX_INTERVALS][4];
						int cnt = oit_interval_ranges(set, SLICES, 0.0f, ranges);
						auto inter = [&](float z) { return cnt > 1 ? oit_interval_slice(ranges, cnt, SLICES, z) : single(z); };

						float r[2] = { run(f, bg, true, single), run(f, bg, true, inter) };
						float rn[2] = { run(f, bg, false, single), run(f, bg, false, inter) };
						for (int m = 0; m < 2; m++) {
							solved[m] += std::fabs(r[m] - want) <= TOL;
							solved_nl[m] += std::fabs(rn[m] - want) <= TOL;
						}
						if (cnt <= 1 && std::fabs(r[0] - r[1]) > 1e-6f) {
							mismatch++;
						}
						n++;
					}
				}
			}
			std::printf("%-24s %-14s | %6d %9d | %6d %9d\n", L.name, P.name, solved[0], solved[1], solved_nl[0], solved_nl[1]);
			for (int m = 0; m < 2; m++) {
				tot[m] += solved[m];
				tot_nl[m] += solved_nl[m];
			}
			n_all += n;
		}
	}
	std::printf("\ntotal of %d: single %d, intervals %d | no-lookup controls: single %d, intervals %d\n", n_all, tot[0], tot[1], tot_nl[0], tot_nl[1]);

	bool ok = true;
	if (!(tot_nl[0] < tot[0]) || !(tot_nl[1] < tot[1])) {
		std::printf("FAIL a no-lookup control did not score worse\n");
		ok = false;
	}
	if (mismatch) {
		std::printf("FAIL with one interval the interval path differs from the single range on %d pixels\n", mismatch);
		ok = false;
	}
	std::printf(ok ? "RESULT: PASS\n" : "RESULT: FAIL\n");
	return ok ? 0 : 1;
}
