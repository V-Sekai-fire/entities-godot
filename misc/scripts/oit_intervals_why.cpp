// Prints the merged intervals and where fragments land, for the 5-band +-100% case in oit_compare_intervals.cpp. Build: c++ -std=c++17 -O1 misc/scripts/oit_intervals_why.cpp -I. -o /tmp/why
#include "servers/rendering/renderer_rd/effects/oit_math.h"
#include <cstdio>
int main() {
	// 5 bands 1.5-40 m, padded +-100% +-2 m, as the harness builds them.
	struct B { float lo, hi; } bands[] = { { 1.5f, 1.7f }, { 4.0f, 4.3f }, { 10.0f, 10.3f }, { 22.0f, 22.4f }, { 40.0f, 40.5f } };
	OITIntervalSet set;
	for (auto &b : bands) {
		float w = b.hi - b.lo, pad = 1.0f * w + 2.0f, lo = b.lo - pad, hi = b.hi + pad;
		if (lo < 0) lo = 0;
		set.add(lo - 0.02f, hi + 0.02f);
	}
	float r[OIT_MAX_INTERVALS][4];
	int n = oit_interval_ranges(set, 128, 0.0f, r);
	std::printf("padded intervals after merge: %d\n", n);
	for (int i = 0; i < n; i++) std::printf("  [%d] near %.2f far %.2f k %.2f first slice %.0f (share %d)\n", i, r[i][0], r[i][1], r[i][2], r[i][3], 128 / n);
	// Depth of the actual fragments per band, and where each band's centre lands.
	for (auto &b : bands) { float z = 0.5f * (b.lo + b.hi); std::printf("  fragment depth %.2f -> slice %.1f\n", z, oit_interval_slice(r, n, 128, z)); }
}
