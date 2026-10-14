// SPDX-License-Identifier: MIT
// Core smoke suite: three properties, each paired with a rule-2
// negative control that would fire on a hostile predicate.

#include "witness/doctest.h"

#include <algorithm>
#include <functional>
#include <vector>

// A vector<int> generator scaled by the current rung.
static std::vector<int> gen_int_vec(witness::RNG &rng, const witness::Level &lvl) {
	uint32_t n = rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32));
	std::vector<int> v(n);
	for (uint32_t i = 0; i < n; ++i) {
		v[i] = rng.int_range(-100, 100);
	}
	return v;
}

TEST_CASE("[witness] reverse . reverse is identity on std::vector<int>") {
	witness::Generator<std::vector<int>> gen = &gen_int_vec;
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		std::vector<int> r = v;
		std::reverse(r.begin(), r.end());
		std::reverse(r.begin(), r.end());
		return r == v;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("involution", gen, pred);
	INFO(t.message);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[witness] falsifiability: a planted-false property is caught") {
	// Rule-2 control for the property above. Same generator, hostile
	// predicate ("all vectors are empty"). Must FOUND at rung 0.
	witness::Generator<std::vector<int>> gen = &gen_int_vec;
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		return v.empty();
	};
	witness::Trial t = witness::resolve<std::vector<int>>("vectors are always empty", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
	CHECK(t.level == 0);
}

TEST_CASE("[witness] std::sort produces a non-decreasing sequence") {
	witness::Generator<std::vector<int>> gen = &gen_int_vec;
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		std::vector<int> s = v;
		std::sort(s.begin(), s.end());
		for (std::size_t i = 1; i < s.size(); ++i) {
			if (s[i - 1] > s[i]) {
				return false;
			}
		}
		return true;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("sorted is monotonic", gen, pred);
	INFO(t.message);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[witness] falsifiability: a planted-false sort property is caught") {
	// Rule-2 control for the sort case: "sorted output is strictly
	// decreasing" is false for any input of size >= 2 with duplicates,
	// and false for most other inputs. Must FOUND.
	witness::Generator<std::vector<int>> gen = &gen_int_vec;
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		std::vector<int> s = v;
		std::sort(s.begin(), s.end());
		for (std::size_t i = 1; i < s.size(); ++i) {
			if (s[i - 1] >= s[i]) {
				return false;
			}
		}
		return true;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("sorted is strictly decreasing", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}
