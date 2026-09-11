/**************************************************************************/
/*  test_property_ladder.cpp                                              */
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

// Smoke test + control for the property-ladder harness. Two paired cases:
//
//   1. Positive: a property that ALWAYS holds — reverse . reverse is the
//      identity on std::vector<int>. Must run all 3000 trials across the
//      ladder without a FOUND outcome.
//   2. Control (planted bug): a property that is DELIBERATELY false —
//      "a random vector is always empty". Must terminate with FOUND at
//      level 0 within a handful of trials. Proves the positive case is
//      not a no-op that reports PROVABLY_NONE regardless of the
//      predicate. Same rule 2 pattern used across the workspace.

#include "tests/test_macros.h"
#include "tests/property_ladder.h"

#include <algorithm>
#include <vector>

namespace TestPropertyLadder {

TEST_CASE("[PropertyLadder] reverse . reverse is identity on std::vector<int>") {
	PROP_CHECK(
			"involution",
			[](property::RNG &rng, const property::Level &lvl) {
				std::vector<int> v(rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32)));
				for (auto &x : v) {
					x = rng.int_range(-100, 100);
				}
				return v;
			},
			[](const std::vector<int> &v) {
				std::vector<int> r = v;
				std::reverse(r.begin(), r.end());
				std::reverse(r.begin(), r.end());
				return r == v; // true when the property holds
			});
}

TEST_CASE("[PropertyLadder] std::sort produces a non-decreasing sequence") {
	PROP_CHECK(
			"sorted is monotonic",
			[](property::RNG &rng, const property::Level &lvl) {
				std::vector<int> v(rng.uint_range(1, static_cast<uint32_t>(lvl.fin_bound / 32) + 1));
				for (auto &x : v) {
					x = rng.int_range(-1000, 1000);
				}
				return v;
			},
			[](std::vector<int> v) {
				std::sort(v.begin(), v.end());
				for (std::size_t i = 1; i < v.size(); ++i) {
					if (v[i - 1] > v[i]) {
						return false;
					}
				}
				return true;
			});
}

TEST_CASE("[PropertyLadder] negative control: a planted-false property is caught") {
	// A ladder that ALWAYS reports PROVABLY_NONE regardless of the
	// predicate is decoration — this control asserts the runner
	// terminates with FOUND on an obviously-false statement.
	::property::Trial trial = ::property::resolve(
			"vectors are always empty",
			[](::property::RNG &rng, const ::property::Level &lvl) {
				std::vector<int> v(rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32)));
				for (auto &x : v) {
					x = rng.int_range(-100, 100);
				}
				return v;
			},
			[](const std::vector<int> &v) {
				return v.empty(); // deliberately wrong
			});

	INFO(trial.message);
	CHECK(trial.outcome == ::property::Outcome::FOUND);
	CHECK(trial.level == 0);
}

} // namespace TestPropertyLadder

TEST_FORCE_LINK(test_property_ladder)
