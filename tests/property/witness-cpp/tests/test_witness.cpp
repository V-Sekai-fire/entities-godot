// SPDX-License-Identifier: MIT
// Core smoke suite for witness::resolve() — positive + planted-false
// (rule-2 negative control) pairs, matching the pattern used across
// V-Sekai-fire repos.

#include "witness/doctest.h"

#include <algorithm>
#include <vector>

TEST_CASE("[witness] reverse . reverse is identity on std::vector<int>") {
	PROP_CHECK(
			"involution",
			[](witness::RNG &rng, const witness::Level &lvl) {
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

TEST_CASE("[witness] std::sort produces a non-decreasing sequence") {
	PROP_CHECK(
			"sorted is monotonic",
			[](witness::RNG &rng, const witness::Level &lvl) {
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

TEST_CASE("[witness] negative control: a planted-false property is caught") {
	witness::Trial trial = witness::resolve(
			"vectors are always empty",
			[](witness::RNG &rng, const witness::Level &lvl) {
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
	CHECK(trial.outcome == witness::Outcome::FOUND);
	CHECK(trial.level == 0);
}
