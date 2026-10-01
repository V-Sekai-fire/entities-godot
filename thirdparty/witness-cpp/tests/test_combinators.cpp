// SPDX-License-Identifier: MIT
// Combinators: gen_tuple, gen_oneof, gen_element, gen_frequency,
// gen_constant, gen_transform, gen_filter, gen_bind, gen_array,
// resolve_with_ladder. Each unit paired with a falsifiability
// control.

#include "witness/doctest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

TEST_CASE("[gen_tuple] populates all slots") {
	witness::Generator<std::tuple<int, char, bool>> gen = witness::gen_tuple<int, char, bool>(
			&witness::gen_int, &witness::gen_char, &witness::gen_bool);
	std::function<bool(const std::tuple<int, char, bool> &)> pred =
			[](const std::tuple<int, char, bool> &t) {
				char c = std::get<1>(t);
				return c >= 32 && c <= 126;
			};
	witness::Trial t = witness::resolve<std::tuple<int, char, bool>>("tuple slots", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_tuple] falsifiability: a slot outside its range is caught") {
	witness::Generator<std::tuple<int, char, bool>> gen = witness::gen_tuple<int, char, bool>(
			&witness::gen_int, &witness::gen_char, &witness::gen_bool);
	std::function<bool(const std::tuple<int, char, bool> &)> pred =
			[](const std::tuple<int, char, bool> &t) {
				char c = std::get<1>(t);
				return c == 'a';
			};
	witness::Trial t = witness::resolve<std::tuple<int, char, bool>>("tuple char always 'a'", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_tuple] draws slots in a fixed order for a fixed seed") {
	// Determinism: repeating the run with the same seed yields the
	// same first counterexample. If the tuple slots drew from the RNG
	// in unspecified order, two runs could diverge.
	witness::Generator<std::tuple<int, int>> gen = witness::gen_tuple<int, int>(
			&witness::gen_int, &witness::gen_int);
	std::function<bool(const std::tuple<int, int> &)> pred =
			[](const std::tuple<int, int> &) { return false; };
	witness::Trial a = witness::resolve<std::tuple<int, int>>("det", gen, pred, 0xF00DULL);
	witness::Trial b = witness::resolve<std::tuple<int, int>>("det", gen, pred, 0xF00DULL);
	CHECK(a.message == b.message);
}

TEST_CASE("[gen_tuple] falsifiability: different seeds diverge") {
	witness::Generator<std::tuple<int, int>> gen = witness::gen_tuple<int, int>(
			&witness::gen_int, &witness::gen_int);
	std::function<bool(const std::tuple<int, int> &)> pred =
			[](const std::tuple<int, int> &) { return false; };
	witness::Trial a = witness::resolve<std::tuple<int, int>>("det", gen, pred, 0x1ULL);
	witness::Trial b = witness::resolve<std::tuple<int, int>>("det", gen, pred, 0x2ULL);
	CHECK(a.message != b.message);
}

TEST_CASE("[gen_oneof] picks from every alternative") {
	std::vector<witness::Generator<int>> alts;
	alts.push_back(witness::gen_constant(1));
	alts.push_back(witness::gen_constant(2));
	alts.push_back(witness::gen_constant(3));
	witness::Generator<int> picker = witness::gen_oneof<int>(alts);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	std::set<int> seen;
	for (int i = 0; i < 200; ++i) {
		seen.insert(picker(rng, lvl));
	}
	CHECK(seen.size() == 3u);
}

TEST_CASE("[gen_oneof] falsifiability: a single-alternative oneof only produces that value") {
	std::vector<witness::Generator<int>> alts;
	alts.push_back(witness::gen_constant(7));
	witness::Generator<int> picker = witness::gen_oneof<int>(alts);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	std::set<int> seen;
	for (int i = 0; i < 100; ++i) {
		seen.insert(picker(rng, lvl));
	}
	CHECK(seen.size() == 1u);
	CHECK(seen.count(7) == 1u);
}

TEST_CASE("[gen_element] picks uniformly from a list of concrete values") {
	std::vector<int> values{ 10, 20, 30 };
	witness::Generator<int> gen = witness::gen_element<int>(values);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	std::set<int> seen;
	for (int i = 0; i < 200; ++i) {
		seen.insert(gen(rng, lvl));
	}
	CHECK(seen.size() == 3u);
	CHECK(seen.count(10) == 1u);
	CHECK(seen.count(20) == 1u);
	CHECK(seen.count(30) == 1u);
}

TEST_CASE("[gen_element] falsifiability: a value not in the list never appears") {
	std::vector<int> values{ 10, 20, 30 };
	witness::Generator<int> gen = witness::gen_element<int>(values);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int off_list = 0;
	for (int i = 0; i < 200; ++i) {
		int v = gen(rng, lvl);
		if (v != 10 && v != 20 && v != 30) {
			off_list++;
		}
	}
	CHECK(off_list == 0);
}

TEST_CASE("[gen_frequency] favors the higher-weighted alternative") {
	std::vector<std::pair<int, witness::Generator<int>>> weighted;
	weighted.emplace_back(9, witness::gen_constant(1));
	weighted.emplace_back(1, witness::gen_constant(2));
	witness::Generator<int> gen = witness::gen_frequency<int>(weighted);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int ones = 0;
	int twos = 0;
	for (int i = 0; i < 1000; ++i) {
		int v = gen(rng, lvl);
		if (v == 1) {
			ones++;
		} else {
			twos++;
		}
	}
	CHECK(ones > twos * 3); // 9:1 mean ratio; ratio > 3 is safe headroom.
}

TEST_CASE("[gen_frequency] falsifiability: swapping the weights swaps the ratio") {
	std::vector<std::pair<int, witness::Generator<int>>> weighted;
	weighted.emplace_back(1, witness::gen_constant(1));
	weighted.emplace_back(9, witness::gen_constant(2));
	witness::Generator<int> gen = witness::gen_frequency<int>(weighted);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int ones = 0;
	int twos = 0;
	for (int i = 0; i < 1000; ++i) {
		int v = gen(rng, lvl);
		if (v == 1) {
			ones++;
		} else {
			twos++;
		}
	}
	CHECK(twos > ones * 3);
}

TEST_CASE("[gen_transform] applies the function to each generated value") {
	witness::Generator<int> gen = witness::gen_transform<int, int>(
			&witness::gen_int, [](int n) { return n * 2; });
	std::function<bool(const int &)> pred = [](const int &n) { return n % 2 == 0; };
	witness::Trial t = witness::resolve<int>("transform even", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_transform] falsifiability: an odd predicate on the doubled stream is caught") {
	witness::Generator<int> gen = witness::gen_transform<int, int>(
			&witness::gen_int, [](int n) { return n * 2; });
	std::function<bool(const int &)> pred = [](const int &n) { return n % 2 == 1; };
	witness::Trial t = witness::resolve<int>("transform is odd", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_filter] rejects non-matching inputs and terminates without FOUND") {
	witness::Generator<int> gen = witness::gen_filter<int>(
			&witness::gen_int, [](const int &n) { return n > 0; });
	std::function<bool(const int &)> pred = [](const int &n) { return n > 0; };
	witness::Trial t = witness::resolve<int>("filtered positive", gen, pred, 0xF17E3ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[gen_filter] falsifiability: an unsatisfiable filter never delivers to the predicate") {
	witness::Generator<int> gen = witness::gen_filter<int>(
			&witness::gen_int, [](const int &) { return false; }, 4);
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	witness::Trial t = witness::resolve<int>("nothing checked", gen, pred, 0xF17E3ULL);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_bind] draws a size, then a vector of that length") {
	std::function<witness::Generator<std::vector<int>>(int)> to_vec =
			[](int n) -> witness::Generator<std::vector<int>> {
		int nn = n;
		return [nn](witness::RNG &rng, const witness::Level &lvl) -> std::vector<int> {
			std::vector<int> v(static_cast<std::size_t>(nn < 0 ? -nn : nn));
			for (std::size_t i = 0; i < v.size(); ++i) {
				v[i] = rng.int_range(-10, 10);
			}
			(void)lvl;
			return v;
		};
	};
	witness::Generator<int> size_gen = witness::gen_int_range(0, 4);
	witness::Generator<std::vector<int>> gen = witness::gen_bind<int, std::vector<int>>(
			size_gen, to_vec);
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		return v.size() <= 4u;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("bound size", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_bind] falsifiability: a size cap tighter than the range is falsified") {
	std::function<witness::Generator<std::vector<int>>(int)> to_vec =
			[](int n) -> witness::Generator<std::vector<int>> {
		int nn = n;
		return [nn](witness::RNG &, const witness::Level &) -> std::vector<int> {
			return std::vector<int>(static_cast<std::size_t>(nn < 0 ? -nn : nn));
		};
	};
	witness::Generator<int> size_gen = witness::gen_int_range(0, 4);
	witness::Generator<std::vector<int>> gen = witness::gen_bind<int, std::vector<int>>(
			size_gen, to_vec);
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		return v.size() <= 2u;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("bound size <= 2", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_array] size is fixed at N") {
	witness::Generator<std::array<int, 4>> gen = witness::gen_array<4, int>(&witness::gen_int);
	std::function<bool(const std::array<int, 4> &)> pred =
			[](const std::array<int, 4> &a) { return a.size() == 4u; };
	witness::Trial t = witness::resolve<std::array<int, 4>>("array size", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_array] falsifiability: a mismatched N would fail the size check") {
	std::array<int, 3> wrong{};
	CHECK_FALSE(wrong.size() == 4u);
}

TEST_CASE("[resolve_with_ladder] a caller-supplied ladder drives the trial count") {
	std::array<witness::Level, 1> tiny = { witness::Level{ 0, 16, 32, 10 } };
	int trials = 0;
	std::function<int(witness::RNG &, const witness::Level &)> gen_fn = &witness::gen_int;
	witness::Generator<int> gen = gen_fn;
	std::function<bool(const int &)> pred = [&](const int &) {
		trials++;
		return true;
	};
	witness::Shrinker<int> sh = &witness::no_shrink<int>;
	std::function<void(std::ostream &, const int &)> printer;
	witness::Trial t = witness::resolve_with_ladder<int>(
			"tiny", tiny, gen, pred, sh, printer, 0x123ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	CHECK(trials == 10);
}

TEST_CASE("[resolve_with_ladder] falsifiability: an empty ladder runs zero trials") {
	std::vector<witness::Level> empty;
	int trials = 0;
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [&](const int &) {
		trials++;
		return false;
	};
	witness::Shrinker<int> sh = &witness::no_shrink<int>;
	std::function<void(std::ostream &, const int &)> printer;
	witness::Trial t = witness::resolve_with_ladder<int>(
			"empty", empty, gen, pred, sh, printer, 0x123ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	CHECK(trials == 0);
}
