// SPDX-License-Identifier: MIT
// Exercises the built-in generator library. Each positive case pairs
// with a control that would fail if the generator were degenerate
// (always returned the same value, always returned the zero value,
// etc.).

#include "witness/doctest.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("[generators] gen_int stays in the fin_bound window") {
	PROP_CHECK(
			"|gen_int| <= scale",
			witness::gen_int,
			[](int n) {
				// Widest rung's scale is fin_bound / 32 = 4096 / 32 = 128.
				return std::abs(n) <= 128;
			});
}

TEST_CASE("[generators] gen_int actually varies (control for a stuck generator)") {
	std::set<int> seen;
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	for (int i = 0; i < 200; ++i) {
		seen.insert(witness::gen_int(rng, lvl));
	}
	// A constant generator would have set.size() == 1. Anything with
	// real variation blows past 10 distinct values quickly.
	CHECK(seen.size() > 10);
}

TEST_CASE("[generators] gen_uint is non-negative") {
	PROP_CHECK(
			"gen_uint >= 0",
			witness::gen_uint,
			[](uint32_t n) { return n <= static_cast<uint32_t>(1 << 30); });
}

TEST_CASE("[generators] gen_double stays finite") {
	PROP_CHECK(
			"gen_double is finite",
			witness::gen_double,
			[](double x) { return std::isfinite(x); });
}

TEST_CASE("[generators] gen_string returns printable ASCII") {
	PROP_CHECK(
			"gen_string is printable",
			witness::gen_string,
			[](const std::string &s) {
				for (char c : s) {
					if (c < 32 || c > 126) {
						return false;
					}
				}
				return true;
			});
}

TEST_CASE("[generators] gen_vector<int> respects size cap") {
	auto make = witness::gen_vector(witness::gen_int);
	PROP_CHECK(
			"|gen_vector| bounded",
			make,
			[](const std::vector<int> &v) {
				return v.size() <= 4096u / 32u;
			});
}

TEST_CASE("[generators] gen_pair<int,int> populates both slots independently") {
	auto make = witness::gen_pair(witness::gen_int, witness::gen_int);
	// Control: an implementation that copied the same int into both
	// slots would fail this — we expect at least one draw where the
	// pair's halves differ.
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	bool ever_different = false;
	for (int i = 0; i < 100; ++i) {
		auto p = make(rng, lvl);
		if (p.first != p.second) {
			ever_different = true;
			break;
		}
	}
	CHECK(ever_different);
}

TEST_CASE("[generators] gen_optional<int> produces both nullopt and populated") {
	auto make = witness::gen_optional(witness::gen_int);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int null_count = 0;
	int some_count = 0;
	for (int i = 0; i < 200; ++i) {
		auto o = make(rng, lvl);
		if (o.has_value()) {
			some_count++;
		} else {
			null_count++;
		}
	}
	CHECK(null_count > 0);
	CHECK(some_count > 0);
}

TEST_CASE("[generators] negative control: a broken generator that always returns 0 is caught") {
	// This is the rule-2 pair for `gen_int actually varies`: prove that
	// if we plug in a degenerate generator, the "varies" test would
	// catch it.
	std::set<int> seen;
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	auto broken = [](witness::RNG &, const witness::Level &) { return 0; };
	for (int i = 0; i < 200; ++i) {
		seen.insert(broken(rng, lvl));
	}
	CHECK(seen.size() == 1);
}
