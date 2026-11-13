// Standalone check for the packed scan in oit_scan_math.h (four slices per RGBA texel). No engine build needed:
//   c++ -std=c++17 -O1 -Wall -Wextra misc/scripts/oit_scan_packed_check.cpp -I. -o /tmp/oit_scan_packed_check && /tmp/oit_scan_packed_check
// Exit 0 only when the packed scan matches the sequential prefix sum on every case AND every planted fault is caught on every trial.
// Faults: (1) the carry pass is skipped, (2) the carry reads the tile's own total instead of its left neighbour's,
// (3) the tile scan is one pass short. Fault trials use density 1.0 and slice counts that fill the last tile, so each fault changes a read value.

#include "servers/rendering/renderer_rd/effects/oit_math.h"
#include "servers/rendering/renderer_rd/effects/oit_scan_math.h"

#include <cstdio>
#include <vector>

static uint32_t rng_state = 20261113u;
static float rnd01() {
	rng_state = rng_state * 1664525u + 1013904223u;
	return float(rng_state >> 8) / float(1u << 24);
}

// Inputs go through the real packing path, so they carry the same 16.16 rounding as the RD shaders.
static std::vector<float> make_column(uint32_t n, float density) {
	std::vector<float> c(n, 0.0f);
	for (uint32_t z = 0; z < n; z++) {
		if (rnd01() < density) {
			c[z] = float(oit_pack_extinction(rnd01())) / 65536.0f;
		}
	}
	return c;
}

static double max_abs_err(const std::vector<float> &model, const std::vector<float> &col) {
	double run = 0.0, worst = 0.0;
	for (size_t z = 0; z < col.size(); z++) {
		run += double(col[z]);
		double e = std::fabs(double(model[z]) - run);
		if (e > worst) {
			worst = e;
		}
	}
	return worst;
}

enum Fault { NONE, NO_CARRY, OWN_TOTAL_CARRY, SHORT_TILE_SCAN };

// The same pass sequence as oit_scan_packed_column, with one fault switched in.
static void packed_with_fault(std::vector<float> &col, Fault f) {
	uint32_t n = uint32_t(col.size()), tiles = (n + 3) / 4;
	std::vector<OITScanTexel> a(tiles), b(tiles);
	for (uint32_t t = 0; t < tiles; t++) {
		a[t] = OITScanTexel{ { 0.0f, 0.0f, 0.0f, 0.0f } };
	}
	for (uint32_t z = 0; z < n; z++) {
		a[z / 4].v[z % 4] = col[z];
	}
	oit_scan_packed_pass_local(a.data(), b.data(), tiles);
	std::swap(a, b);
	uint32_t passes = oit_scan_pass_count(tiles);
	if (f == SHORT_TILE_SCAN && passes > 0) {
		passes--;
	}
	uint32_t stride = 1;
	for (uint32_t p = 0; p < passes; p++, stride <<= 1) {
		oit_scan_packed_pass_tiles(a.data(), b.data(), tiles, stride);
		std::swap(a, b);
	}
	if (f == OWN_TOTAL_CARRY) {
		for (uint32_t t = 0; t < tiles; t++) {
			float carry = a[t].v[3];
			b[t].v[0] = a[t].v[0] + carry;
			b[t].v[1] = a[t].v[1] + carry;
			b[t].v[2] = a[t].v[2] + carry;
			b[t].v[3] = a[t].v[3];
		}
		std::swap(a, b);
	} else if (f != NO_CARRY) {
		oit_scan_packed_pass_carry(a.data(), b.data(), tiles);
		std::swap(a, b);
	}
	for (uint32_t z = 0; z < n; z++) {
		col[z] = a[z / 4].v[z % 4];
	}
}

int main() {
	int failures = 0;
	const double tol = 1e-3;

	// 1. Matches the sequential prefix sum, including slice counts that are not a multiple of four.
	const uint32_t counts[] = { 1, 2, 3, 4, 5, 7, 8, 9, 12, 16, 31, 32, 33, 63, 64, 100, 127, 128, 129, 200, 256 };
	const float densities[] = { 0.05f, 0.3f, 1.0f };
	int cases = 0;
	double global_worst = 0.0;
	for (uint32_t n : counts) {
		for (float d : densities) {
			for (int rep = 0; rep < 25; rep++) {
				std::vector<float> col = make_column(n, d);
				std::vector<float> got = col;
				oit_scan_packed_column(got.data(), n);
				double e = max_abs_err(got, col);
				if (e > global_worst) {
					global_worst = e;
				}
				if (e > tol) {
					std::printf("FAIL n=%u density=%.2f rep=%d err=%g\n", n, d, rep, e);
					failures++;
				}
				cases++;
			}
		}
	}
	std::printf("packed scan matches sequential: %d cases, worst abs err %.3g (tol %.0e)\n", cases, global_worst, tol);

	// 2. Pass counts: packed against unpacked.
	struct {
		uint32_t n, want_packed;
	} counts_want[] = { { 1, 2 }, { 4, 2 }, { 5, 3 }, { 32, 5 }, { 64, 6 }, { 128, 7 }, { 256, 8 } };
	for (auto &p : counts_want) {
		uint32_t got = oit_scan_packed_pass_count(p.n);
		std::printf("slices %3u: packed %u passes, unpacked %u passes\n", p.n, got, oit_scan_pass_count(p.n));
		if (got != p.want_packed) {
			std::printf("FAIL packed pass_count(%u)=%u want %u\n", p.n, got, p.want_packed);
			failures++;
		}
	}

	// 3. Planted faults, each must be caught on every trial.
	// Counts whose last tile holds four slices. With a partial last tile (33 or 129 slices, one slice in the last tile)
	// a short tile scan only damages channel 3 of that tile, which nothing reads: channel 3 is read only as the carry of
	// the next tile, and there is none. That fault cannot change an output there, so those counts would test nothing.
	const uint32_t fault_counts[] = { 12, 36, 64, 100, 128, 132, 200 };
	const char *names[] = { "", "carry skipped", "carry reads own total", "tile scan one pass short" };
	for (int fi = 1; fi <= 3; fi++) {
		int caught = 0, trials = 0;
		for (uint32_t n : fault_counts) {
			for (int rep = 0; rep < 30; rep++) {
				std::vector<float> col = make_column(n, 1.0f);
				std::vector<float> got = col;
				packed_with_fault(got, Fault(fi));
				if (max_abs_err(got, col) > tol) {
					caught++;
				}
				trials++;
			}
		}
		std::printf("planted fault \"%s\" caught %d/%d\n", names[fi], caught, trials);
		if (caught != trials) {
			std::printf("FAIL planted fault not always detected\n");
			failures++;
		}
	}

	// 4. The fault harness with no fault must agree with the real function, or the faults above prove nothing.
	{
		std::vector<float> col = make_column(128, 1.0f), a = col, b = col;
		oit_scan_packed_column(a.data(), 128);
		packed_with_fault(b, NONE);
		bool same = true;
		for (size_t z = 0; z < a.size(); z++) {
			same = same && a[z] == b[z];
		}
		std::printf("fault harness without a fault equals oit_scan_packed_column: %s\n", same ? "yes" : "NO");
		if (!same) {
			failures++;
		}
	}

	std::printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
	return failures ? 1 : 0;
}
