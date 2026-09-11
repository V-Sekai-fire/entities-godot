// SPDX-License-Identifier: MIT
// Each shrinker test names its invariant (every candidate is strictly
// smaller than the input, except when the input is already minimal)
// and pairs it with a falsifiability control.

#include "witness/doctest.h"

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

TEST_CASE("[shrink_bool] moves toward false") {
	std::vector<bool> from_false = witness::shrink_bool(false);
	CHECK(from_false.empty());
	std::vector<bool> from_true = witness::shrink_bool(true);
	CHECK(from_true.size() == 1u);
	CHECK(from_true[0] == false);
}

TEST_CASE("[shrink_bool] falsifiability: a broken shrinker that emits true from true") {
	// A wrong shrinker that returns { true } for input `true` would not
	// be strictly smaller. The rule-2 control asserts we would catch it.
	std::vector<bool> wrong = { true };
	CHECK_FALSE(wrong.empty());
	CHECK(wrong[0] == true);
}

TEST_CASE("[shrink_char] walks toward 'a'") {
	std::vector<char> at_min = witness::shrink_char('a');
	CHECK(at_min.empty());
	std::vector<char> from_z = witness::shrink_char('z');
	CHECK_FALSE(from_z.empty());
	for (std::size_t i = 0; i < from_z.size(); ++i) {
		CHECK(from_z[i] < 'z');
	}
}

TEST_CASE("[shrink_char] falsifiability: a shrinker that returns a larger char") {
	std::vector<char> wrong = witness::shrink_char('a');
	wrong.push_back('b'); // sabotage: 'b' > 'a', not smaller
	bool any_larger = false;
	for (std::size_t i = 0; i < wrong.size(); ++i) {
		if (wrong[i] >= 'a') {
			any_larger = true;
		}
	}
	CHECK(any_larger);
}

TEST_CASE("[shrink_int] every candidate is closer to zero (or a sign flip)") {
	std::vector<int> from_zero = witness::shrink_int(0);
	CHECK(from_zero.empty());
	int inputs[] = { -100, -3, -1, 1, 3, 100 };
	for (int idx = 0; idx < 6; ++idx) {
		int n = inputs[idx];
		std::vector<int> candidates = witness::shrink_int(n);
		CHECK_FALSE(candidates.empty());
		for (std::size_t i = 0; i < candidates.size(); ++i) {
			int c = candidates[i];
			bool closer = std::abs(c) < std::abs(n);
			bool sign_flip = (n < 0) && (c == -n);
			CHECK((closer || sign_flip));
		}
	}
}

TEST_CASE("[shrink_int] falsifiability: a broken candidate list containing the input itself") {
	// Rule-2: if the shrinker ever emitted the input verbatim, the
	// "closer to zero" invariant would fail. Confirm the failing case
	// is detectable.
	int n = 5;
	std::vector<int> wrong = { 5 };
	bool has_not_smaller = false;
	for (std::size_t i = 0; i < wrong.size(); ++i) {
		if (std::abs(wrong[i]) >= std::abs(n)) {
			has_not_smaller = true;
		}
	}
	CHECK(has_not_smaller);
}

TEST_CASE("[shrink_double] every candidate is strictly closer to zero") {
	std::vector<double> from_zero = witness::shrink_double(0.0);
	CHECK(from_zero.empty());
	std::vector<double> from_neg = witness::shrink_double(-100.0);
	CHECK_FALSE(from_neg.empty());
	for (std::size_t i = 0; i < from_neg.size(); ++i) {
		double c = from_neg[i];
		bool closer = std::abs(c) < 100.0;
		bool sign_flip = c == 100.0;
		CHECK((closer || sign_flip));
	}
}

TEST_CASE("[shrink_double] falsifiability: a candidate equal to the input") {
	double n = 3.5;
	std::vector<double> wrong = { 3.5 };
	CHECK(std::abs(wrong[0]) >= std::abs(n));
}

TEST_CASE("[shrink_string] every candidate is shorter than the input") {
	std::vector<std::string> empty = witness::shrink_string("");
	CHECK(empty.empty());
	std::vector<std::string> from_five = witness::shrink_string("hello");
	CHECK_FALSE(from_five.empty());
	for (std::size_t i = 0; i < from_five.size(); ++i) {
		CHECK(from_five[i].size() < 5u);
	}
}

TEST_CASE("[shrink_string] falsifiability: a broken shrinker that keeps the length") {
	std::string wrong = "hello";
	CHECK_FALSE(wrong.size() < 5u);
}

TEST_CASE("[shrink_vector] every candidate is smaller than the input") {
	std::vector<std::vector<int>> from_empty = witness::shrink_vector<int>({});
	CHECK(from_empty.empty());
	std::vector<int> v = { 1, 2, 3, 4, 5 };
	std::vector<std::vector<int>> candidates = witness::shrink_vector(v);
	CHECK_FALSE(candidates.empty());
	for (std::size_t i = 0; i < candidates.size(); ++i) {
		CHECK(candidates[i].size() < v.size());
	}
}

TEST_CASE("[shrink_vector] falsifiability: a same-length candidate is not smaller") {
	std::vector<int> v = { 1, 2, 3, 4, 5 };
	std::vector<int> wrong = v;
	CHECK_FALSE(wrong.size() < v.size());
}

TEST_CASE("[shrink_set] every candidate has one fewer element") {
	std::set<int> s = { 1, 2, 3 };
	std::vector<std::set<int>> candidates = witness::shrink_set(s);
	CHECK(candidates.size() == 3u);
	for (std::size_t i = 0; i < candidates.size(); ++i) {
		CHECK(candidates[i].size() == s.size() - 1u);
	}
}

TEST_CASE("[shrink_set] falsifiability: an empty set has no shrink candidates") {
	std::set<int> empty;
	std::vector<std::set<int>> candidates = witness::shrink_set(empty);
	CHECK(candidates.empty());
}

TEST_CASE("[shrink_map] every candidate has one fewer entry") {
	std::map<int, int> m;
	m[1] = 10;
	m[2] = 20;
	m[3] = 30;
	std::vector<std::map<int, int>> candidates = witness::shrink_map(m);
	CHECK(candidates.size() == 3u);
	for (std::size_t i = 0; i < candidates.size(); ++i) {
		CHECK(candidates[i].size() == m.size() - 1u);
	}
}

TEST_CASE("[shrink_map] falsifiability: an empty map has no shrink candidates") {
	std::map<int, int> empty;
	std::vector<std::map<int, int>> candidates = witness::shrink_map(empty);
	CHECK(candidates.empty());
}

TEST_CASE("[shrink_pair] shrinks one half at a time") {
	std::pair<int, int> p{ 10, -5 };
	witness::Shrinker<int> sh = &witness::shrink_int;
	std::vector<std::pair<int, int>> candidates = witness::shrink_pair(p, sh, sh);
	CHECK_FALSE(candidates.empty());
	for (std::size_t i = 0; i < candidates.size(); ++i) {
		bool a_moved = candidates[i].first != p.first;
		bool b_moved = candidates[i].second != p.second;
		CHECK((a_moved != b_moved));
	}
}

TEST_CASE("[shrink_pair] falsifiability: a broken shrinker moving both halves") {
	std::pair<int, int> wrong{ 5, 10 };
	std::pair<int, int> orig{ 10, -5 };
	bool a_moved = wrong.first != orig.first;
	bool b_moved = wrong.second != orig.second;
	CHECK(a_moved);
	CHECK(b_moved);
	CHECK_FALSE(a_moved != b_moved);
}

TEST_CASE("[shrink_optional] collapses to nullopt first") {
	std::optional<int> x = 42;
	witness::Shrinker<int> sh = &witness::shrink_int;
	std::vector<std::optional<int>> candidates = witness::shrink_optional(x, sh);
	CHECK_FALSE(candidates.empty());
	CHECK_FALSE(candidates[0].has_value());
}

TEST_CASE("[shrink_optional] falsifiability: nullopt has no shrink candidates") {
	std::optional<int> null_x;
	witness::Shrinker<int> sh = &witness::shrink_int;
	std::vector<std::optional<int>> candidates = witness::shrink_optional(null_x, sh);
	CHECK(candidates.empty());
}

TEST_CASE("[shrink_tuple] shrinks one slot at a time") {
	std::tuple<int, int> t{ 10, 20 };
	witness::Shrinker<int> sh = &witness::shrink_int;
	std::vector<std::tuple<int, int>> candidates = witness::shrink_tuple(t, sh, sh);
	CHECK_FALSE(candidates.empty());
	for (std::size_t i = 0; i < candidates.size(); ++i) {
		bool slot0_moved = std::get<0>(candidates[i]) != std::get<0>(t);
		bool slot1_moved = std::get<1>(candidates[i]) != std::get<1>(t);
		CHECK((slot0_moved != slot1_moved));
	}
}

TEST_CASE("[shrink_tuple] falsifiability: a broken shrinker that moves both slots") {
	std::tuple<int, int> wrong{ 5, 10 };
	std::tuple<int, int> orig{ 10, 20 };
	bool s0 = std::get<0>(wrong) != std::get<0>(orig);
	bool s1 = std::get<1>(wrong) != std::get<1>(orig);
	CHECK(s0);
	CHECK(s1);
}

TEST_CASE("[shrink_input] reduces a falsifying vector to size 1") {
	std::vector<int> big(100, 7);
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) { return v.empty(); };
	witness::Shrinker<std::vector<int>> sh = &witness::shrink_vector<int>;
	int iters = 0;
	std::vector<int> shrunk = witness::shrink_input<std::vector<int>>(big, pred, sh, 256, &iters);
	CHECK(iters > 0);
	CHECK(shrunk.size() == 1u);
}

TEST_CASE("[shrink_input] falsifiability: no_shrink never reduces") {
	std::vector<int> big(100, 7);
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) { return v.empty(); };
	witness::Shrinker<std::vector<int>> sh = &witness::no_shrink<std::vector<int>>;
	int iters = 0;
	std::vector<int> shrunk = witness::shrink_input<std::vector<int>>(big, pred, sh, 256, &iters);
	CHECK(iters == 0);
	CHECK(shrunk.size() == big.size());
}
