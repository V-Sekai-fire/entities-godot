/**************************************************************************/
/*  test_oit.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_oit)

#include "core/config/project_settings.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"
#include "servers/rendering/renderer_rd/effects/oit_math.h"

#include <thirdparty/witness-cpp/include/witness/ladder.h>

#include <cstring>

namespace TestOIT {

TEST_CASE("[OIT] Project settings register the AVBOIT surface") {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	REQUIRE(ps != nullptr);
	CHECK(ps->has_setting("rendering/oit/enabled"));
	CHECK(ps->has_setting("rendering/oit/slice_count"));
	CHECK(ps->has_setting("rendering/oit/tile_size"));
	CHECK(ps->has_setting("rendering/oit/splat_mode"));
	CHECK(ps->has_setting("rendering/oit/near_plane"));
	CHECK(ps->has_setting("rendering/oit/far_plane"));
	CHECK(ps->has_setting("rendering/oit/linearization_factor"));
	CHECK(bool(ps->get_setting("rendering/oit/enabled")) == false);
	CHECK(int(ps->get_setting("rendering/oit/slice_count")) == 128);
}

TEST_CASE("[OIT] depth_to_slice: near plane collapses to slice 0") {
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 0.05f) == 0u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 0.1f) == 0u);
}

TEST_CASE("[OIT] depth_to_slice: far plane clamps to slice N-1") {
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 500.0f) == 127u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 9999.0f) == 127u);
}

TEST_CASE("[OIT] depth_to_slice: near-field gets more slices than far-field") {
	uint32_t near_slice = oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 5.0f);
	uint32_t mid_slice = oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 250.0f);
	uint32_t far_slice = oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 495.0f);
	CHECK(near_slice > 0u);
	CHECK(near_slice < mid_slice);
	CHECK(mid_slice < far_slice);
	CHECK((far_slice - mid_slice) < mid_slice);
}

TEST_CASE("[OIT] depth_to_slice: negative control catches a constant regression") {
	uint32_t a = oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 1.0f);
	uint32_t b = oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 100.0f);
	CHECK(a != b);
}

static const float oit_table16_z[16] = { 0.15f, 0.3f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f, 350.0f, 420.0f, 470.0f, 495.0f };

static void oit_check_table16(float p_k, const uint32_t (&p_expected)[16]) {
	for (int i = 0; i < 16; i++) {
		CAPTURE(p_k);
		CAPTURE(i);
		CHECK(oit_depth_to_slice(0.1f, 500.0f, p_k, 128, oit_table16_z[i]) == p_expected[i]);
	}
}

TEST_CASE("[OIT] depth_to_slice: sixteen-point tables match the Lean spec") {
	oit_check_table16(0.5f, { 0, 0, 0, 0, 0, 1, 2, 4, 9, 19, 38, 71, 94, 110, 121, 126 });
	oit_check_table16(100.0f, { 0, 1, 2, 4, 8, 15, 26, 39, 55, 72, 90, 109, 118, 123, 126, 127 });
	oit_check_table16(1000.0f, { 1, 6, 10, 19, 29, 40, 52, 64, 77, 90, 102, 115, 121, 124, 126, 127 });
	oit_check_table16(10000.0f, { 9, 22, 30, 40, 50, 60, 70, 80, 89, 99, 109, 118, 123, 125, 127, 127 });
}

TEST_CASE("[OIT] depth_to_slice: linearization sweep at seven depths") {
	const float z[7] = { 1.0f, 10.0f, 50.0f, 100.0f, 250.0f, 400.0f, 495.0f };
	const uint32_t k005[7] = { 0, 2, 13, 26, 64, 102, 126 };
	const uint32_t k2[7] = { 0, 4, 21, 39, 80, 111, 127 };
	for (int i = 0; i < 7; i++) {
		CAPTURE(i);
		CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.05f, 128, z[i]) == k005[i]);
		CHECK(oit_depth_to_slice(0.1f, 500.0f, 2.0f, 128, z[i]) == k2[i]);
	}
}

TEST_CASE("[OIT] depth_to_slice: doubling slice_count doubles the index at view_z 100") {
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 64, 100.0f) == 15u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 100.0f) == 30u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 256, 100.0f) == 60u);
}

TEST_CASE("[OIT] depth_to_slice: control, k = 0.5 is flat where k = 1000 is not") {
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 1.0f) == oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, 2.0f));
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 1000.0f, 128, 1.0f) < oit_depth_to_slice(0.1f, 500.0f, 1000.0f, 128, 2.0f));
	CHECK(double(GLOBAL_GET("rendering/oit/linearization_factor")) == doctest::Approx(1000.0));
}

TEST_CASE("[OIT] depth_to_slice: adversarial inputs stay bounded") {
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, -1.0f) == 0u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.5f, 128, -100.0f) == 0u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.0f, 128, 0.1f) == 0u);
	CHECK(oit_depth_to_slice(0.1f, 500.0f, 0.0f, 128, 500.0f) == 127u);
	CHECK(oit_depth_to_slice(5.0f, 5.0f, 0.5f, 128, 5.0f) == 0u);
	CHECK(oit_depth_to_slice(5.0f, 5.0f, 0.5f, 128, 6.0f) == 127u);
}

TEST_CASE("[OIT] pack_extinction: ten-row table matches the Lean spec") {
	const float alpha[10] = { 0.0f, 0.001f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 0.99f, 0.999f, 1.0f };
	const uint32_t packed[10] = { 0, 65, 6904, 18853, 45426, 90852, 150902, 301804, 452707, 452707 };
	for (int i = 0; i < 10; i++) {
		CAPTURE(i);
		CHECK(oit_pack_extinction(alpha[i]) == packed[i]);
		if (i > 0) {
			CHECK(oit_pack_extinction(alpha[i]) >= oit_pack_extinction(alpha[i - 1]));
		}
	}
	CHECK(oit_pack_extinction(-1.0f) == 0u);
	CHECK(oit_pack_extinction(7.0f) == 452707u);
}

TEST_CASE("[OIT] pack_extinction: summing packed values composites alpha within one unit") {
	const float pairs[5][2] = { { 0.5f, 0.5f }, { 0.25f, 0.75f }, { 0.1f, 0.9f }, { 0.3f, 0.3f }, { 0.5f, 0.25f } };
	const uint32_t gap[5] = { 0, 0, 1, 0, 0 };
	for (int i = 0; i < 5; i++) {
		CAPTURE(i);
		uint32_t s = oit_pack_extinction(pairs[i][0]) + oit_pack_extinction(pairs[i][1]);
		uint32_t c = oit_pack_extinction(1.0f - (1.0f - pairs[i][0]) * (1.0f - pairs[i][1]));
		CHECK((s >= c ? s - c : c - s) == gap[i]);
	}
	CHECK(oit_pack_extinction(0.5f) + oit_pack_extinction(0.5f) != oit_pack_extinction(1.0f));
}

TEST_CASE("[OIT] flat_index: matches the Lean layout at 320x180x128") {
	CHECK(oit_flat_index(180, 128, 0, 0, 0) == 0u);
	CHECK(oit_flat_index(180, 128, 0, 0, 127) == 127u);
	CHECK(oit_flat_index(180, 128, 0, 1, 0) == 128u);
	CHECK(oit_flat_index(180, 128, 1, 0, 0) == 180u * 128u);
	CHECK(oit_flat_index(180, 128, 319, 179, 127) == 320u * 180u * 128u - 1u);
	for (uint32_t z = 0; z < 127; z++) {
		CHECK(oit_flat_index(180, 128, 7, 9, z + 1) == oit_flat_index(180, 128, 7, 9, z) + 1);
	}
}

TEST_CASE("[OIT] flat_index: every froxel of a 5x3x8 grid gets its own slot") {
	LocalVector<bool> seen;
	seen.resize(5 * 3 * 8);
	for (uint32_t i = 0; i < seen.size(); i++) {
		seen[i] = false;
	}
	for (uint32_t x = 0; x < 5; x++) {
		for (uint32_t y = 0; y < 3; y++) {
			for (uint32_t z = 0; z < 8; z++) {
				uint32_t i = oit_flat_index(3, 8, x, y, z);
				REQUIRE(i < seen.size());
				CHECK_FALSE(seen[i]);
				seen[i] = true;
			}
		}
	}
	CHECK(((1 + 0) * 8 + 0) == ((0 + 1) * 8 + 0));
	CHECK(oit_flat_index(3, 8, 1, 0, 0) != oit_flat_index(3, 8, 0, 1, 0));
}

struct SplatFragment {
	uint32_t x;
	uint32_t y;
	float view_z;
	float alpha;
};

static const uint32_t SPLAT_DIMS[3] = { 4, 3, 8 };

static uint32_t splat_froxel(const SplatFragment &p_f) {
	uint32_t z = oit_depth_to_slice(0.1f, 100.0f, 1000.0f, SPLAT_DIMS[2], p_f.view_z);
	return oit_flat_index(SPLAT_DIMS[1], SPLAT_DIMS[2], p_f.x, p_f.y, z);
}

static LocalVector<uint32_t> splat_grid(const LocalVector<SplatFragment> &p_frags) {
	LocalVector<uint32_t> grid;
	grid.resize(SPLAT_DIMS[0] * SPLAT_DIMS[1] * SPLAT_DIMS[2]);
	for (uint32_t i = 0; i < grid.size(); i++) {
		grid[i] = 0;
	}
	for (const SplatFragment &f : p_frags) {
		if (f.view_z > 0.0f) {
			grid[splat_froxel(f)] += oit_pack_extinction(f.alpha);
		}
	}
	return grid;
}

static LocalVector<SplatFragment> splat_samples() {
	LocalVector<SplatFragment> frags;
	frags.push_back({ 1, 1, 1.0f, 0.5f });
	frags.push_back({ 1, 1, 3.0f, 0.5f });
	frags.push_back({ 1, 1, 9.0f, 0.5f });
	frags.push_back({ 2, 0, 3.0f, 0.25f });
	frags.push_back({ 2, 0, 3.0f, 0.75f });
	frags.push_back({ 0, 2, -1.0f, 0.9f });
	frags.push_back({ 3, 2, 99.0f, 0.1f });
	return frags;
}

TEST_CASE("[OIT] splat: froxel totals match the Lean reference on the shared samples") {
	LocalVector<SplatFragment> frags = splat_samples();
	LocalVector<uint32_t> grid = splat_grid(frags);

	CHECK(oit_depth_to_slice(0.1f, 100.0f, 1000.0f, 8, 1.0f) == 2u);
	CHECK(oit_depth_to_slice(0.1f, 100.0f, 1000.0f, 8, 3.0f) == 3u);
	CHECK(oit_depth_to_slice(0.1f, 100.0f, 1000.0f, 8, 9.0f) == 5u);
	CHECK(grid[splat_froxel({ 2, 0, 3.0f, 0.0f })] == oit_pack_extinction(0.25f) + oit_pack_extinction(0.75f));
	CHECK(grid[splat_froxel({ 1, 1, 1.0f, 0.0f })] == oit_pack_extinction(0.5f));

	uint64_t total = 0;
	for (uint32_t v : grid) {
		total += v;
	}
	uint64_t expected = 0;
	for (const SplatFragment &f : frags) {
		if (f.view_z > 0.0f) {
			expected += oit_pack_extinction(f.alpha);
		}
	}
	CHECK(total == expected);

	LocalVector<SplatFragment> reversed;
	for (uint32_t i = frags.size(); i > 0; i--) {
		reversed.push_back(frags[i - 1]);
	}
	LocalVector<uint32_t> grid_reversed = splat_grid(reversed);
	for (uint32_t i = 0; i < grid.size(); i++) {
		CHECK(grid[i] == grid_reversed[i]);
	}
}

TEST_CASE("[OIT] splat: control, double-counting and screen-resolution raster disagree") {
	LocalVector<SplatFragment> frags = splat_samples();
	LocalVector<SplatFragment> twice(frags);
	for (const SplatFragment &f : frags) {
		twice.push_back(f);
	}
	LocalVector<uint32_t> grid = splat_grid(frags);
	LocalVector<uint32_t> grid_twice = splat_grid(twice);
	bool differs = false;
	for (uint32_t i = 0; i < grid.size(); i++) {
		differs = differs || (grid[i] != grid_twice[i]);
	}
	CHECK(differs);

	LocalVector<SplatFragment> tile;
	for (uint32_t i = 0; i < 36; i++) {
		tile.push_back({ 1, 1, 3.0f, 0.5f });
	}
	CHECK(splat_grid(tile)[splat_froxel({ 1, 1, 3.0f, 0.0f })] == 36u * oit_pack_extinction(0.5f));
}

static LocalVector<float> column_transmittance(const LocalVector<uint32_t> &p_column) {
	LocalVector<float> t;
	uint64_t sum = 0;
	for (uint32_t v : p_column) {
		sum += v;
		t.push_back(std::exp(-float(sum) / 65536.0f));
	}
	return t;
}

TEST_CASE("[OIT] lookup: a fragment reads the slice in front of its own") {
	LocalVector<uint32_t> lone;
	lone.resize(8);
	for (uint32_t i = 0; i < 8; i++) {
		lone[i] = 0;
	}
	lone[3] = oit_pack_extinction(0.5f);
	LocalVector<float> t = column_transmittance(lone);
	CHECK(oit_lookup_transmittance(t.ptr(), 3) == doctest::Approx(1.0f).epsilon(1e-4));
	CHECK(t[3] == doctest::Approx(0.5f).epsilon(1e-4));

	LocalVector<uint32_t> pair(lone);
	pair[3] = 0;
	pair[2] = oit_pack_extinction(0.5f);
	pair[5] = oit_pack_extinction(0.5f);
	t = column_transmittance(pair);
	CHECK(oit_lookup_transmittance(t.ptr(), 5) == doctest::Approx(0.5f).epsilon(1e-4));
	CHECK(oit_lookup_transmittance(t.ptr(), 2) == doctest::Approx(1.0f).epsilon(1e-4));
	CHECK(oit_lookup_transmittance(t.ptr(), 0) == 1.0f);
}

TEST_CASE("[OIT] views: packed columns are distinct across two views of a 4-wide grid") {
	for (uint32_t v = 0; v < 2; v++) {
		for (uint32_t x = 0; x < 4; x++) {
			for (uint32_t v2 = 0; v2 < 2; v2++) {
				for (uint32_t x2 = 0; x2 < 4; x2++) {
					CHECK((oit_packed_column(4, v, x) == oit_packed_column(4, v2, x2)) == (v == v2 && x == x2));
				}
			}
		}
	}
}

TEST_CASE("[OIT] views: a pixel center looks up the column it splatted into, both eyes") {
	for (uint32_t v = 0; v < 2; v++) {
		for (uint32_t x = 0; x < 320; x++) {
			CAPTURE(v);
			CAPTURE(x);
			CHECK(oit_sampled_column(320, 2, oit_packed_u(320, 2, v, (float(x) + 0.5f) / 320.0f)) == oit_packed_column(320, v, x));
		}
	}
	for (uint32_t x = 0; x < 320; x++) {
		CHECK(oit_sampled_column(320, 1, oit_packed_u(320, 1, 0, (float(x) + 0.5f) / 320.0f)) == x);
	}
}

TEST_CASE("[OIT] views: the clamp keeps each eye's edge inside its own slab") {
	CHECK(oit_sampled_column(4, 2, oit_packed_u(4, 2, 0, 1.0f)) == 3u);
	CHECK(oit_sampled_column(4, 2, oit_packed_u(4, 2, 1, 0.0f)) == 4u);
	// Control: unclamped, the right edge of view 0 reads view 1's first column.
	CHECK(oit_sampled_column(4, 2, (1.0f + 0.0f) / 2.0f) == 4u);
}

struct ViewSample {
	uint32_t dim_x;
	uint32_t view_count;
	uint32_t view;
	float u;
};

static witness::Generator<ViewSample> view_sample_gen = [](witness::RNG &r, const witness::Level &) -> ViewSample {
	ViewSample s;
	s.dim_x = r.uint_range(2, 640);
	s.view_count = r.uint_range(1, 2);
	s.view = r.uint_range(0, s.view_count - 1);
	s.u = float(r.float_range(-0.25, 1.25));
	return s;
};

TEST_CASE("[OIT][Witness] views: the clamped lookup never leaves the eye's slab") {
	std::function<bool(const ViewSample &)> predicate = [](const ViewSample &s) -> bool {
		uint32_t c = oit_sampled_column(s.dim_x, s.view_count, oit_packed_u(s.dim_x, s.view_count, s.view, s.u));
		return c >= s.view * s.dim_x && c < (s.view + 1) * s.dim_x;
	};
	witness::Trial trial = witness::resolve<ViewSample>("oit-views-in-slab", view_sample_gen, predicate);
	CHECK(trial.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Witness] views: falsification control, the unclamped lookup bleeds") {
	std::function<bool(const ViewSample &)> predicate = [](const ViewSample &s) -> bool {
		float unclamped = (s.u + float(s.view)) / float(s.view_count);
		uint32_t c = oit_sampled_column(s.dim_x, s.view_count, unclamped < 0.0f ? 0.0f : unclamped);
		return c >= s.view * s.dim_x && c < (s.view + 1) * s.dim_x;
	};
	witness::Trial trial = witness::resolve<ViewSample>("oit-views-bleed-control", view_sample_gen, predicate);
	CHECK(trial.outcome == witness::Outcome::FOUND);
}

struct ResolveEvent {
	float color;
	float alpha;
	uint32_t slice;
};

// Three half-alpha surfaces in slices 2, 3 and 5 of 8 over a background of 1, as in Oit/Resolve.lean.
static const ResolveEvent RESOLVE_STACK[3] = { { 0.8f, 0.5f, 5 }, { 0.2f, 0.5f, 2 }, { 0.6f, 0.5f, 3 } };
static const float RESOLVE_REFERENCE = 0.475f;

static float resolve_reference(const LocalVector<ResolveEvent> &p_sorted, float p_background) {
	float color = 0.0f;
	float t = 1.0f;
	for (const ResolveEvent &e : p_sorted) {
		color += e.color * e.alpha * t;
		t *= 1.0f - e.alpha;
	}
	return color + p_background * t;
}

static LocalVector<ResolveEvent> resolve_sorted(const LocalVector<ResolveEvent> &p_events) {
	LocalVector<ResolveEvent> sorted(p_events);
	for (uint32_t i = 1; i < sorted.size(); i++) {
		for (uint32_t j = i; j > 0 && sorted[j].slice < sorted[j - 1].slice; j--) {
			SWAP(sorted[j], sorted[j - 1]);
		}
	}
	return sorted;
}

static float resolve_exact_front(const LocalVector<ResolveEvent> &p_events, const ResolveEvent &p_e) {
	float t = 1.0f;
	for (const ResolveEvent &o : p_events) {
		if (o.slice < p_e.slice) {
			t *= 1.0f - o.alpha;
		}
	}
	return t;
}

static float resolve_froxel_front(const LocalVector<ResolveEvent> &p_events, const ResolveEvent &p_e) {
	LocalVector<uint32_t> column;
	column.resize(8);
	for (uint32_t i = 0; i < 8; i++) {
		column[i] = 0;
	}
	for (const ResolveEvent &o : p_events) {
		column[o.slice] += oit_pack_extinction(o.alpha);
	}
	LocalVector<float> t = column_transmittance(column);
	return oit_lookup_transmittance(t.ptr(), p_e.slice);
}

static OITAccum resolve_accumulate(const LocalVector<ResolveEvent> &p_events, bool p_froxel) {
	OITAccum accum;
	for (const ResolveEvent &e : p_events) {
		float t = p_froxel ? resolve_froxel_front(p_events, e) : resolve_exact_front(p_events, e);
		oit_accumulate(accum, e.color, e.alpha, t);
	}
	return accum;
}

// Control: the lookup scales alpha and a sorted back-to-front over-blend composites it.
static float resolve_over_blend(const LocalVector<ResolveEvent> &p_sorted, float p_background) {
	float acc = p_background;
	for (int i = int(p_sorted.size()) - 1; i >= 0; i--) {
		const ResolveEvent &e = p_sorted[i];
		float a = e.alpha * resolve_exact_front(p_sorted, e);
		acc = e.color * a + acc * (1.0f - a);
	}
	return acc;
}

TEST_CASE("[OIT] resolve: the weighted resolve reproduces the sorted over-blend in any order") {
	LocalVector<ResolveEvent> stack;
	for (const ResolveEvent &e : RESOLVE_STACK) {
		stack.push_back(e);
	}
	CHECK(resolve_reference(resolve_sorted(stack), 1.0f) == doctest::Approx(RESOLVE_REFERENCE).epsilon(1e-4));

	CHECK(oit_resolve(resolve_accumulate(stack, false), 1.0f) == doctest::Approx(RESOLVE_REFERENCE).epsilon(1e-4));
	LocalVector<ResolveEvent> reversed;
	for (int i = int(stack.size()) - 1; i >= 0; i--) {
		reversed.push_back(stack[i]);
	}
	CHECK(oit_resolve(resolve_accumulate(reversed, false), 1.0f) == doctest::Approx(RESOLVE_REFERENCE).epsilon(1e-4));
	CHECK(oit_resolve(resolve_accumulate(stack, true), 1.0f) == doctest::Approx(RESOLVE_REFERENCE).epsilon(1e-4));

	OITAccum exact = resolve_accumulate(stack, false);
	CHECK(exact.alpha == doctest::Approx(1.0f - std::exp(-exact.extinction)).epsilon(1e-4));

	LocalVector<ResolveEvent> lone;
	lone.push_back({ 0.2f, 0.5f, 3 });
	CHECK(oit_resolve(resolve_accumulate(lone, true), 1.0f) == doctest::Approx(0.6f).epsilon(1e-4));
}

TEST_CASE("[OIT] resolve: control, alpha scaled into a sorted over-blend attenuates twice") {
	LocalVector<ResolveEvent> stack;
	for (const ResolveEvent &e : RESOLVE_STACK) {
		stack.push_back(e);
	}
	float twice = resolve_over_blend(resolve_sorted(stack), 1.0f);
	CHECK(twice == doctest::Approx(0.540625f).epsilon(1e-4));
	CHECK(twice != doctest::Approx(RESOLVE_REFERENCE).epsilon(1e-4));

	// Two events in one slice: the lookup cannot order them, the total transmittance still holds.
	LocalVector<ResolveEvent> shared;
	shared.push_back({ 1.0f, 0.5f, 4 });
	shared.push_back({ 0.0f, 0.5f, 4 });
	OITAccum a = resolve_accumulate(shared, true);
	CHECK(std::exp(-a.extinction) == doctest::Approx(0.25f).epsilon(1e-4));
	LocalVector<ResolveEvent> swapped;
	swapped.push_back(shared[1]);
	swapped.push_back(shared[0]);
	CHECK(oit_resolve(a, 0.0f) == doctest::Approx(oit_resolve(resolve_accumulate(swapped, true), 0.0f)).epsilon(1e-4));
}

struct DepthPair {
	float near_plane;
	float far_plane;
	float linearization;
	uint32_t slice_count;
	float z0;
	float z1;
};

TEST_CASE("[OIT][Witness] depth_to_slice is monotone in view_z") {
	witness::Generator<DepthPair> gen = [](witness::RNG &r, const witness::Level &) -> DepthPair {
		DepthPair p;
		p.near_plane = float(r.float_range(0.01, 5.0));
		p.far_plane = p.near_plane + float(r.float_range(10.0, 1000.0));
		p.linearization = float(std::exp(r.float_range(std::log(0.05), std::log(20000.0))));
		p.slice_count = 128;
		p.z0 = float(r.float_range(p.near_plane, p.far_plane));
		p.z1 = p.z0 + float(r.float_range(0.01, p.far_plane - p.z0 + 0.02));
		return p;
	};
	std::function<bool(const DepthPair &)> predicate = [](const DepthPair &p) -> bool {
		uint32_t s0 = oit_depth_to_slice(p.near_plane, p.far_plane, p.linearization, p.slice_count, p.z0);
		uint32_t s1 = oit_depth_to_slice(p.near_plane, p.far_plane, p.linearization, p.slice_count, p.z1);
		return s1 >= s0;
	};
	witness::Trial trial = witness::resolve<DepthPair>("oit-depth-monotone", gen, predicate);
	CHECK(trial.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Witness] slice indices are always in range") {
	witness::Generator<DepthPair> gen = [](witness::RNG &r, const witness::Level &) -> DepthPair {
		DepthPair p;
		p.near_plane = float(r.float_range(0.01, 5.0));
		p.far_plane = p.near_plane + float(r.float_range(10.0, 1000.0));
		p.linearization = float(std::exp(r.float_range(std::log(0.05), std::log(20000.0))));
		p.slice_count = 1u << r.uint_range(6, 9);
		p.z0 = float(r.float_range(-100.0, p.far_plane * 2.0));
		p.z1 = p.z0;
		return p;
	};
	std::function<bool(const DepthPair &)> predicate = [](const DepthPair &p) -> bool {
		uint32_t s = oit_depth_to_slice(p.near_plane, p.far_plane, p.linearization, p.slice_count, p.z0);
		return s < p.slice_count;
	};
	witness::Trial trial = witness::resolve<DepthPair>("oit-slice-bounded", gen, predicate);
	CHECK(trial.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Witness] falsification control: constant slice function is caught") {
	witness::Generator<DepthPair> gen = [](witness::RNG &r, const witness::Level &) -> DepthPair {
		DepthPair p;
		p.near_plane = 0.1f;
		p.far_plane = 100.0f;
		p.linearization = 0.5f;
		p.slice_count = 128;
		p.z0 = float(r.float_range(0.1, 100.0));
		p.z1 = p.z0 + float(r.float_range(0.5, 90.0));
		return p;
	};
	// Predicate uses a KNOWN-BROKEN slice function (always returns 0) but
	// pretends the "correct" values are (0, s1_real). Monotonicity holds
	// iff s1 >= 0, which is trivially true for the broken function.
	// The USEFUL falsification check is the reverse: assert that when
	// z1 > z0 we get s1 > s0 STRICTLY at large enough gaps. A constant
	// function violates this for any input pair, so the ladder MUST
	// return FOUND.
	std::function<bool(const DepthPair &)> broken_predicate = [](const DepthPair &p) -> bool {
		uint32_t broken_s0 = 42;
		uint32_t broken_s1 = 42;
		if (p.z1 - p.z0 > 5.0f) {
			return broken_s1 > broken_s0;
		}
		return true;
	};
	witness::Trial trial = witness::resolve<DepthPair>("oit-falsification-control", gen, broken_predicate);
	CHECK(trial.outcome == witness::Outcome::FOUND);
}

// The hash ladder of Oit/Tables.lean: the same LCG, the same rows, the same FNV-1a fold.
static uint32_t tables_lcg(uint32_t p_x) {
	return p_x * 1664525u + 1013904223u;
}

static float tables_unit(uint32_t p_x) {
	return float(p_x >> 8) / 16777216.0f;
}

static uint32_t tables_fnv1a(uint32_t p_h, uint32_t p_w) {
	for (int i = 0; i < 4; i++) {
		p_h = (p_h ^ ((p_w >> (8 * i)) & 0xFFu)) * 16777619u;
	}
	return p_h;
}

static uint32_t tables_hash(uint32_t p_rows, bool p_perturb) {
	uint32_t x = 0x9E3779B9u;
	uint32_t h = 2166136261u;
	for (uint32_t i = 0; i < p_rows; i++) {
		uint32_t w1 = tables_lcg(x);
		uint32_t w2 = tables_lcg(w1);
		uint32_t w3 = tables_lcg(w2);
		uint32_t w4 = tables_lcg(w3);
		float k = tables_unit(w1) * 200.0f;
		k = k + 0.05f;
		float z = tables_unit(w2) * 600.0f;
		uint32_t slice = oit_depth_to_slice(0.1f, 500.0f, k, 128, z);
		uint32_t ext = oit_pack_extinction(tables_unit(w3));
		if (p_perturb && i == 7) {
			ext += 1;
		}
		float packed = oit_packed_u(320, 2, w4 % 2, tables_unit(w4));
		uint32_t bits;
		memcpy(&bits, &packed, sizeof(bits));
		h = tables_fnv1a(tables_fnv1a(tables_fnv1a(h, slice), ext), bits);
		x = w4;
	}
	return h;
}

TEST_CASE("[OIT] tables: the FNV-1a ladder matches the Lean spec") {
	CHECK(tables_hash(4096, false) == 0x9371EF76u);
}

TEST_CASE("[OIT] tables: control, one perturbed row changes the hash") {
	CHECK(tables_hash(4096, true) == 0x2AFAE94Du);
	CHECK(tables_hash(4096, true) != tables_hash(4096, false));
	CHECK(tables_hash(4095, false) != tables_hash(4096, false));
}

} // namespace TestOIT
