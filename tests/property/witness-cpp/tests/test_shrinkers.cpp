// SPDX-License-Identifier: MIT
// Property-tests for the shrinker library. Each shrinker's contract:
// every emitted candidate must be strictly smaller than the input
// (except when the input is already at the minimum, where the
// candidate list is empty).

#include "witness/doctest.h"

#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("[shrinkers] shrink_int reaches zero") {
	CHECK(witness::shrink_int(0).empty());
	for (int n : { -100, -3, -1, 1, 3, 100 }) {
		auto candidates = witness::shrink_int(n);
		CHECK_FALSE(candidates.empty());
		for (int c : candidates) {
			bool closer = std::abs(c) < std::abs(n);
			bool sign_flip = (n < 0) && (c == -n);
			CHECK((closer || sign_flip));
		}
	}
}

TEST_CASE("[shrinkers] shrink_uint reaches zero") {
	CHECK(witness::shrink_uint(0).empty());
	for (uint32_t n : { 1u, 3u, 100u }) {
		auto candidates = witness::shrink_uint(n);
		CHECK_FALSE(candidates.empty());
		for (uint32_t c : candidates) {
			CHECK(c < n);
		}
	}
}

TEST_CASE("[shrinkers] shrink_double reaches zero") {
	CHECK(witness::shrink_double(0.0).empty());
	for (double x : { -100.0, -1.5, 1.5, 100.0 }) {
		auto candidates = witness::shrink_double(x);
		CHECK_FALSE(candidates.empty());
		for (double c : candidates) {
			bool closer = std::abs(c) < std::abs(x);
			bool sign_flip = (x < 0.0) && (c == -x);
			CHECK((closer || sign_flip));
		}
	}
}

TEST_CASE("[shrinkers] shrink_string reduces length") {
	CHECK(witness::shrink_string("").empty());
	auto candidates = witness::shrink_string("hello");
	CHECK_FALSE(candidates.empty());
	for (const auto &c : candidates) {
		CHECK(c.size() < std::string("hello").size());
	}
}

TEST_CASE("[shrinkers] shrink_vector<int> reduces size") {
	CHECK(witness::shrink_vector<int>({}).empty());
	std::vector<int> v = { 1, 2, 3, 4, 5 };
	auto candidates = witness::shrink_vector(v);
	CHECK_FALSE(candidates.empty());
	for (const auto &c : candidates) {
		CHECK(c.size() < v.size());
	}
}

TEST_CASE("[shrinkers] shrink_pair chains through its halves") {
	std::pair<int, int> p{ 10, -5 };
	auto shrink_a = [](int n) { return witness::shrink_int(n); };
	auto shrink_b = [](int n) { return witness::shrink_int(n); };
	auto candidates = witness::shrink_pair(p, shrink_a, shrink_b);
	CHECK_FALSE(candidates.empty());
	for (const auto &c : candidates) {
		// Exactly one half changed toward zero (or sign flipped for
		// negative halves).
		bool a_moved = (c.first != p.first);
		bool b_moved = (c.second != p.second);
		CHECK((a_moved != b_moved));
	}
}

TEST_CASE("[shrinkers] shrink_optional collapses to nullopt first") {
	std::optional<int> x = 42;
	auto candidates = witness::shrink_optional(x,
			[](int n) { return witness::shrink_int(n); });
	CHECK_FALSE(candidates.empty());
	CHECK_FALSE(candidates[0].has_value()); // nullopt comes first
}

TEST_CASE("[shrinkers] shrink_input actually minimizes") {
	// End-to-end: a big vector, a predicate that's false for any
	// non-empty vector — the shrinker must reduce to size 1 (any
	// smaller satisfies the predicate).
	std::vector<int> big(100, 7);
	int iters = 0;
	auto shrunk = witness::shrink_input(big,
			[](const std::vector<int> &v) { return v.empty(); },
			[](const std::vector<int> &v) { return witness::shrink_vector(v); },
			256,
			&iters);
	CHECK(iters > 0);
	CHECK(shrunk.size() == 1u);
}

TEST_CASE("[shrinkers] negative control: a no-op shrinker never reduces") {
	// Rule-2 pair: prove that shrink_input without a real shrinker
	// leaves the input alone. This is what NoShrink is for.
	std::vector<int> big(100, 7);
	int iters = 0;
	auto shrunk = witness::shrink_input(big,
			[](const std::vector<int> &v) { return v.empty(); },
			witness::NoShrink{},
			256,
			&iters);
	CHECK(iters == 0);
	CHECK(shrunk.size() == big.size());
}
