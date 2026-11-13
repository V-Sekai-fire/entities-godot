// Froxel with the engine auto-range given bounds looser than the scene, as instance AABBs would give. No engine build needed:
//   c++ -std=c++17 -O1 misc/scripts/oit_compare_loose.cpp -I. -o /tmp/oit_compare_loose && /tmp/oit_compare_loose
#include "servers/rendering/renderer_rd/effects/oit_math.h"
#include "servers/rendering/renderer_rd/effects/oit_scan_math.h"
#include "servers/rendering/renderer_rd/effects/oit_splat_math.h"
#include <algorithm>
#include <cstdio>
#include <vector>
struct Frag { float z, a, c; };
static uint32_t st = 20261112u;
static float r01() { st = st * 1664525u + 1013904223u; return float(st >> 8) / float(1u << 24); }
static float truth(std::vector<Frag> f, float bg) { std::sort(f.begin(), f.end(), [](const Frag &x, const Frag &y) { return x.z > y.z; }); float o = bg; for (auto &g : f) o = g.c * g.a + o * (1 - g.a); return o; }
static float fx(const std::vector<Frag> &f, float bg, float n, float fa, float k, bool lookup) {
	const uint32_t S = 128; std::vector<float> col(S, 0), sc(S), tr(S); std::vector<uint32_t> sl(f.size());
	for (size_t i = 0; i < f.size(); i++) { sl[i] = oit_depth_to_slice(n, fa, k, S, f[i].z); col[sl[i]] += oit_splat_extinction(f[i].a); }
	oit_scan_column(col.data(), sc.data(), S);
	for (uint32_t z = 0; z < S; z++) tr[z] = oit_scan_transmittance(col[z]);
	OITAccum acc; for (size_t i = 0; i < f.size(); i++) oit_accumulate(acc, f[i].c, f[i].a, lookup ? oit_lookup_transmittance(tr.data(), sl[i]) : 1.0f);
	return oit_resolve(acc, bg);
}
int main() {
	struct { const char *n; float lo, hi; } sp[] = { { "wide", 0.5f, 50 }, { "mid", 10, 14 }, { "tight", 10, 10.2f } };
	struct { const char *n; float pad_frac, pad_abs; } loose[] = { { "exact", 0, 0 }, { "+-25% +-0.1m", 0.25f, 0.1f }, { "+-50% +-1m", 0.5f, 1.0f }, { "+-100% +-2m", 1.0f, 2.0f } };
	const int counts[] = { 2, 4, 8, 16, 32 }; const float al[][2] = { { 0.05f, 0.2f }, { 0.2f, 0.6f }, { 0.6f, 0.95f } };
	std::printf("%-14s | solved of 4500 | mean |err|   (tight spread alone, of 1500)\n", "range bounds");
	for (auto &L : loose) {
		int solved = 0, tight = 0, N = 0; double es = 0;
		st = 20261112u;
		for (int n : counts) for (auto &a : al) for (int si = 0; si < 3; si++) for (int s = 0; s < 100; s++) {
			std::vector<Frag> f(n);
			for (auto &g : f) { g.z = sp[si].lo + r01() * (sp[si].hi - sp[si].lo); g.a = a[0] + r01() * (a[1] - a[0]); g.c = r01(); }
			float bg = r01(); float want = truth(f, bg);
			float w = sp[si].hi - sp[si].lo, lo = sp[si].lo - (L.pad_frac * w + L.pad_abs), hi = sp[si].hi + (L.pad_frac * w + L.pad_abs);
			float nr, fr, k; float got = oit_auto_range(lo, hi, 0.02f, nr, fr, k) ? fx(f, bg, nr, fr, k, true) : -1;
			double e = std::fabs(double(got) - double(want)); es += e; N++;
			if (e <= 0.01) { solved++; if (si == 2) tight++; }
		}
		std::printf("%-14s | %5d          | %.5f      (%d)\n", L.n, solved, es / N, tight);
	}
}
