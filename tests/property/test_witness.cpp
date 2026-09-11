/**************************************************************************/
/*  test_witness.cpp                                                      */
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

// Godot-side shim for github.com/V-Sekai-fire/witness-cpp — pulled in
// as a git subtree at tests/property/witness-cpp so the property harness
// can be developed and tested standalone (with its own CMake/doctest CI)
// while still linking into Godot's tests binary here.
//
// The shim intentionally does NOT include witness/doctest.h — that
// header includes <doctest/doctest.h>, which is a different install
// path than Godot's thirdparty/doctest. Godot's tests/test_macros.h
// already provides SUBCASE, CHECK and INFO from the same doctest
// distribution; the PROP_CHECK macros below are the two-liner that
// binds witness::resolve into a doctest SUBCASE.

#include "tests/property/witness-cpp/include/witness/ladder.h"
#include "tests/test_macros.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#define PROP_CHECK(m_query, m_make_input, m_predicate)                        \
	SUBCASE(m_query) {                                                        \
		::witness::Trial _prop_trial =                                        \
				::witness::resolve(m_query, m_make_input, m_predicate);       \
		INFO(_prop_trial.message);                                            \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND);              \
	}

namespace TestWitness {

TEST_CASE("[Witness] reverse . reverse is identity on std::vector<int>") {
	PROP_CHECK(
			"involution",
			[](::witness::RNG &rng, const ::witness::Level &lvl) {
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
				return r == v;
			});
}

TEST_CASE("[Witness] negative control: a planted-false property is caught") {
	// A ladder that ALWAYS reports PROVABLY_NONE regardless of the
	// predicate is decoration — this rule-2 control asserts the runner
	// terminates with FOUND on an obviously-false statement.
	::witness::Trial trial = ::witness::resolve(
			"vectors are always empty",
			[](::witness::RNG &rng, const ::witness::Level &lvl) {
				std::vector<int> v(rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32)));
				for (auto &x : v) {
					x = rng.int_range(-100, 100);
				}
				return v;
			},
			[](const std::vector<int> &v) {
				return v.empty();
			});

	INFO(trial.message);
	CHECK(trial.outcome == ::witness::Outcome::FOUND);
	CHECK(trial.level == 0);
}

TEST_CASE("[Witness] reproducer seed on failure names the seed to re-run") {
	::witness::Trial t = ::witness::resolve(
			"always-false", ::witness::gen_int,
			[](int) { return false; }, 0xC0FFEEULL);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
	CHECK(t.message.find("property_seed=0xc0ffee") != std::string::npos);
}

TEST_CASE("[Witness] assume() and classify() flow through to the report") {
	::witness::Trial t = ::witness::resolve(
			"assume + classify",
			::witness::gen_int,
			[](int n) {
				::witness::assume(n >= -1000000); // effectively always true
				::witness::classify(n > 0, "positive");
				return true;
			},
			0xBEEFULL);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
	INFO(t.message);
	CHECK(t.message.find("classifications:") != std::string::npos);
	CHECK(t.message.find("positive") != std::string::npos);
}

TEST_CASE("[Witness] shrinker reduces a falsifying vector toward the minimum") {
	auto shrinker = [](const std::vector<int> &v) {
		return ::witness::shrink_vector(v);
	};
	auto printer = [](std::ostream &os, const std::vector<int> &v) {
		os << "vector<int> size=" << v.size();
	};

	::witness::Trial trial = ::witness::resolve(
			"vectors are always empty (shrinking variant)",
			[](::witness::RNG &rng, const ::witness::Level &lvl) {
				std::vector<int> v(rng.uint_range(20, static_cast<uint32_t>(lvl.fin_bound / 4) + 20));
				for (auto &x : v) {
					x = rng.int_range(-100, 100);
				}
				return v;
			},
			[](const std::vector<int> &v) { return v.empty(); },
			shrinker,
			printer);

	INFO(trial.message);
	CHECK(trial.outcome == ::witness::Outcome::FOUND);
	CHECK(trial.message.find("shrunk ") != std::string::npos);
}

} // namespace TestWitness

TEST_FORCE_LINK(test_witness)
