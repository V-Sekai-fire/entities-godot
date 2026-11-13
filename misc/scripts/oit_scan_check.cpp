// Standalone check for oit_scan_math.h. No engine build needed:
//   c++ -std=c++17 -O1 -Wall -Wextra misc/scripts/oit_scan_check.cpp -I. -o /tmp/oit_scan_check && /tmp/oit_scan_check
// Exit 0 only when the model matches the sequential reference AND both planted faults are detected.

#include "servers/rendering/renderer_rd/effects/oit_math.h"
#include "servers/rendering/renderer_rd/effects/oit_scan_math.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

static uint32_t rng_state = 12345u;
static float rnd01() {
	rng_state = rng_state * 1664525u + 1013904223u;
	return float(rng_state >> 8) / float(1u << 24);
}

// Column of extinction values built from the real packing path, so the inputs have the same 16.16 rounding as the RD shaders.
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

// Fault 1: stride sequence off by one (starts at 2, so slice z never sees z-1).
static void scan_fault_stride(std::vector<float> &col) {
	std::vector<float> scratch(col.size());
	float *src = col.data(), *dst = scratch.data();
	for (uint32_t stride = 2; stride < col.size(); stride <<= 1) {
		oit_scan_pass(src, dst, uint32_t(col.size()), stride);
		std::swap(src, dst);
	}
	if (src != col.data()) {
		for (size_t z = 0; z < col.size(); z++) {
			col[z] = src[z];
		}
	}
}

// Fault 2: one pass too few (the last stride is skipped).
static void scan_fault_short(std::vector<float> &col) {
	std::vector<float> scratch(col.size());
	float *src = col.data(), *dst = scratch.data();
	uint32_t passes = oit_scan_pass_count(uint32_t(col.size())) - 1;
	uint32_t stride = 1;
	for (uint32_t p = 0; p < passes; p++, stride <<= 1) {
		oit_scan_pass(src, dst, uint32_t(col.size()), stride);
		std::swap(src, dst);
	}
	if (src != col.data()) {
		for (size_t z = 0; z < col.size(); z++) {
			col[z] = src[z];
		}
	}
}

int main() {
	int failures = 0;
	const uint32_t sizes[] = { 64, 128, 256, 512, 100 }; // 100 is not a power of two on purpose.
	const float densities[] = { 0.02f, 0.3f, 1.0f };
	double global_worst = 0.0;
	double tol = 1e-3; // Extinction is summed in float32; 512 terms of up to ~7 stay far inside this.
	int cases = 0;
	for (uint32_t n : sizes) {
		for (float d : densities) {
			for (int rep = 0; rep < 50; rep++) {
				std::vector<float> col = make_column(n, d);
				std::vector<float> ref = col;
				std::vector<float> scratch(n);
				oit_scan_column(col.data(), scratch.data(), n);
				double e = max_abs_err(col, ref);
				if (e > global_worst) {
					global_worst = e;
				}
				cases++;
				if (e > tol) {
					std::printf("FAIL n=%u density=%.2f rep=%d err=%g\n", n, d, rep, e);
					failures++;
				}
			}
		}
	}
	std::printf("scan matches sequential: %d cases, worst abs err %.3g (tol %.0e)\n", cases, global_worst, tol);

	// Pass count: log2 rounded up.
	struct {
		uint32_t n, want;
	} pc[] = { { 1, 0 }, { 2, 1 }, { 64, 6 }, { 100, 7 }, { 128, 7 }, { 129, 8 }, { 512, 9 } };
	for (auto &p : pc) {
		if (oit_scan_pass_count(p.n) != p.want) {
			std::printf("FAIL pass_count(%u)=%u want %u\n", p.n, oit_scan_pass_count(p.n), p.want);
			failures++;
		}
	}

	// Planted faults: the same comparison must flag them.
	int caught_stride = 0, caught_short = 0, trials = 40;
	for (int i = 0; i < trials; i++) {
		std::vector<float> a = make_column(128, 0.3f), b = a, ref = a;
		scan_fault_stride(a);
		scan_fault_short(b);
		if (max_abs_err(a, ref) > tol) {
			caught_stride++;
		}
		if (max_abs_err(b, ref) > tol) {
			caught_short++;
		}
	}
	std::printf("planted stride fault caught %d/%d, short-pass fault caught %d/%d\n", caught_stride, trials, caught_short, trials);
	if (caught_stride != trials || caught_short != trials) {
		std::printf("FAIL planted fault not always detected\n");
		failures++;
	}

	std::printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
	return failures ? 1 : 0;
}
