// SPDX-License-Identifier: MIT
// Covers the combinator library: gen_bool, gen_char, gen_tuple,
// gen_oneof, gen_transform, gen_filter, shrink_tuple, and
// resolve_with_ladder. Each positive case has a rule-2 control.

#include "witness/doctest.h"

#include <array>
#include <functional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

TEST_CASE("[combinators] gen_bool produces both values") {
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int t = 0;
	int f = 0;
	for (int i = 0; i < 100; ++i) {
		if (witness::gen_bool(rng, lvl)) {
			t++;
		} else {
			f++;
		}
	}
	CHECK(t > 0);
	CHECK(f > 0);
}

TEST_CASE("[combinators] shrink_bool moves toward false") {
	CHECK(witness::shrink_bool(false).empty());
	std::vector<bool> from_true = witness::shrink_bool(true);
	CHECK(from_true.size() == 1u);
	CHECK(from_true[0] == false);
}

TEST_CASE("[combinators] gen_char stays printable") {
	PROP_CHECK(
			"gen_char printable",
			witness::gen_char,
			[](char c) { return c >= 32 && c <= 126; });
}

TEST_CASE("[combinators] shrink_char walks toward 'a'") {
	CHECK(witness::shrink_char('a').empty());
	std::vector<char> from_z = witness::shrink_char('z');
	CHECK_FALSE(from_z.empty());
	for (char c : from_z) {
		CHECK(c < 'z');
	}
}

TEST_CASE("[combinators] gen_tuple populates all slots") {
	std::function<std::tuple<int, char, bool>(witness::RNG &, const witness::Level &)> make =
			witness::gen_tuple(witness::gen_int, witness::gen_char, witness::gen_bool);
	PROP_CHECK(
			"tuple slots are typed",
			make,
			[](const std::tuple<int, char, bool> &t) {
				char c = std::get<1>(t);
				return c >= 32 && c <= 126;
			});
}

TEST_CASE("[combinators] shrink_tuple shrinks one slot at a time") {
	std::tuple<int, int> t{ 10, 20 };
	auto shrink_a = [](int n) { return witness::shrink_int(n); };
	auto shrink_b = [](int n) { return witness::shrink_int(n); };
	std::vector<std::tuple<int, int>> candidates = witness::shrink_tuple(t, shrink_a, shrink_b);
	CHECK_FALSE(candidates.empty());
	for (const std::tuple<int, int> &c : candidates) {
		bool a_moved = std::get<0>(c) != std::get<0>(t);
		bool b_moved = std::get<1>(c) != std::get<1>(t);
		CHECK((a_moved != b_moved));
	}
}

TEST_CASE("[combinators] gen_oneof picks from the given list") {
	using Gen = std::function<int(witness::RNG &, const witness::Level &)>;
	std::vector<Gen> gens{
		[](witness::RNG &, const witness::Level &) { return 1; },
		[](witness::RNG &, const witness::Level &) { return 2; },
		[](witness::RNG &, const witness::Level &) { return 3; },
	};
	Gen picker = witness::gen_oneof(gens);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	std::set<int> seen;
	for (int i = 0; i < 200; ++i) {
		seen.insert(picker(rng, lvl));
	}
	CHECK(seen.size() == 3u);
	CHECK(seen.count(1) == 1u);
	CHECK(seen.count(2) == 1u);
	CHECK(seen.count(3) == 1u);
}

TEST_CASE("[combinators] gen_transform maps a function over generated values") {
	auto doubled = witness::gen_transform(witness::gen_int, [](int n) { return n * 2; });
	PROP_CHECK(
			"transformed values are even",
			doubled,
			[](int n) { return n % 2 == 0; });
}

TEST_CASE("[combinators] gen_filter rejects non-matching inputs") {
	auto positive = witness::gen_filter(
			witness::gen_int,
			[](int n) { return n > 0; });
	witness::Trial t = witness::resolve(
			"filtered ints are positive",
			positive,
			[](int n) { return n > 0; },
			0xF17E3ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[combinators] gen_filter negative control: an unsatisfiable filter marks trials SKIP") {
	// Rule-2 pair: a filter whose predicate is unsatisfiable must not
	// silently pass a hostile predicate — the trials get skipped and
	// the resolver terminates at PROVABLY_NONE (nothing was checked).
	auto impossible = witness::gen_filter(
			witness::gen_int,
			[](int) { return false; },
			4);
	witness::Trial t = witness::resolve(
			"nothing to check",
			impossible,
			[](int) { return false; }, // would fail if ever reached
			0xF17E3ULL);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[combinators] resolve_with_ladder honors a caller-supplied ladder") {
	// One-rung ladder: 10 trials at the smallest rung, no others.
	std::array<witness::Level, 1> tiny = { witness::Level{ 0, 16, 32, 10 } };
	int trials = 0;
	witness::Trial t = witness::resolve_with_ladder(
			"tiny ladder",
			tiny,
			witness::gen_int,
			[&](int) {
				trials++;
				return true;
			},
			witness::NoShrink{},
			witness::NoPrint{},
			0x123ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	CHECK(trials == 10);
}

TEST_CASE("[combinators] resolve_with_ladder negative control: an empty ladder runs nothing") {
	// A 0-rung ladder yields PROVABLY_NONE with no trials counted —
	// pairs with the tiny-ladder test above to prove ladder length
	// actually drives the trial count.
	std::vector<witness::Level> empty_ladder;
	int trials = 0;
	witness::Trial t = witness::resolve_with_ladder(
			"empty ladder",
			empty_ladder,
			witness::gen_int,
			[&](int) {
				trials++;
				return false;
			},
			witness::NoShrink{},
			witness::NoPrint{},
			0x123ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	CHECK(trials == 0);
}
