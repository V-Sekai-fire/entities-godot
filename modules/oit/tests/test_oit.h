/**************************************************************************/
/*  test_oit.h                                                            */
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

#pragma once

#include "../oit_math.h"

#include "core/config/project_settings.h"
#include "core/variant/variant.h"
#include "tests/test_macros.h"

#include <thirdparty/witness-cpp/include/witness/ladder.h>

namespace TestOIT {

TEST_CASE("[OIT] Project settings register the AVBOIT surface") {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	REQUIRE(ps != nullptr);
	CHECK(ps->has_setting("rendering/oit/enabled"));
	CHECK(ps->has_setting("rendering/oit/slice_count"));
	CHECK(ps->has_setting("rendering/oit/tile_size"));
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
		p.linearization = float(r.float_range(0.05, 3.0));
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
		p.linearization = float(r.float_range(0.05, 3.0));
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

} // namespace TestOIT
