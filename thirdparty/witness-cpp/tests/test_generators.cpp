// SPDX-License-Identifier: MIT
// Every generator test names its unit (invariant the generator
// promises) and pairs it with a falsifiability control that would
// catch a broken implementation.

#include "witness/doctest.h"

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

TEST_CASE("[gen_bool] produces both values") {
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

TEST_CASE("[gen_bool] falsifiability: a stuck-at-true generator produces only trues") {
	std::function<bool(witness::RNG &, const witness::Level &)> stuck =
			[](witness::RNG &, const witness::Level &) { return true; };
	int f = 0;
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	for (int i = 0; i < 100; ++i) {
		if (!stuck(rng, lvl)) {
			f++;
		}
	}
	CHECK(f == 0);
}

TEST_CASE("[gen_int] stays inside the ladder's window") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &n) {
		return std::abs(n) <= 128;
	};
	witness::Trial t = witness::resolve<int>("gen_int windowed", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_int] falsifiability: a window narrower than the generator's output is caught") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &n) {
		return std::abs(n) == 0;
	};
	witness::Trial t = witness::resolve<int>("gen_int only produces zero", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_int_range] respects its bounds") {
	witness::Generator<int> gen = witness::gen_int_range(5, 7);
	std::function<bool(const int &)> pred = [](const int &n) { return n >= 5 && n <= 7; };
	witness::Trial t = witness::resolve<int>("range [5,7]", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_int_range] falsifiability: a stricter bound is falsified") {
	witness::Generator<int> gen = witness::gen_int_range(5, 7);
	std::function<bool(const int &)> pred = [](const int &n) { return n == 5; };
	witness::Trial t = witness::resolve<int>("range [5,7] always == 5", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_double] stays finite") {
	witness::Generator<double> gen = &witness::gen_double;
	std::function<bool(const double &)> pred = [](const double &x) { return std::isfinite(x); };
	witness::Trial t = witness::resolve<double>("gen_double is finite", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_double] falsifiability: pretending double is always integer") {
	witness::Generator<double> gen = &witness::gen_double;
	std::function<bool(const double &)> pred = [](const double &x) {
		return x == std::floor(x);
	};
	witness::Trial t = witness::resolve<double>("gen_double always integer", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_string] returns printable ASCII") {
	witness::Generator<std::string> gen = &witness::gen_string;
	std::function<bool(const std::string &)> pred = [](const std::string &s) {
		for (std::size_t i = 0; i < s.size(); ++i) {
			char c = s[i];
			if (c < 32 || c > 126) {
				return false;
			}
		}
		return true;
	};
	witness::Trial t = witness::resolve<std::string>("gen_string printable", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_string] falsifiability: strings are always length zero") {
	witness::Generator<std::string> gen = &witness::gen_string;
	std::function<bool(const std::string &)> pred = [](const std::string &s) { return s.empty(); };
	witness::Trial t = witness::resolve<std::string>("gen_string always empty", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_string_of] restricts characters to the alphabet") {
	witness::Generator<std::string> gen = witness::gen_string_of("abc");
	std::function<bool(const std::string &)> pred = [](const std::string &s) {
		for (std::size_t i = 0; i < s.size(); ++i) {
			if (s[i] != 'a' && s[i] != 'b' && s[i] != 'c') {
				return false;
			}
		}
		return true;
	};
	witness::Trial t = witness::resolve<std::string>("alphabet respected", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_string_of] falsifiability: an off-alphabet check falsifies") {
	witness::Generator<std::string> gen = witness::gen_string_of("abc");
	std::function<bool(const std::string &)> pred = [](const std::string &s) {
		for (std::size_t i = 0; i < s.size(); ++i) {
			if (s[i] == 'z') {
				return false;
			}
		}
		return s.size() < 3;
	};
	witness::Trial t = witness::resolve<std::string>("gen_string_of size cap 3", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_vector] respects its size cap") {
	witness::Generator<int> elem = &witness::gen_int;
	witness::Generator<std::vector<int>> gen = witness::gen_vector<int>(elem);
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		return v.size() <= 4096u / 32u;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("vector size cap", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_vector] falsifiability: a tighter size cap is falsified") {
	witness::Generator<int> elem = &witness::gen_int;
	witness::Generator<std::vector<int>> gen = witness::gen_vector<int>(elem);
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		return v.size() <= 2u;
	};
	witness::Trial t = witness::resolve<std::vector<int>>("vector <= 2", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_set] contains only distinct elements") {
	witness::Generator<int> elem = witness::gen_int_range(0, 10);
	witness::Generator<std::set<int>> gen = witness::gen_set<int>(elem);
	std::function<bool(const std::set<int> &)> pred = [](const std::set<int> &) {
		return true; // std::set enforces uniqueness at the type level.
	};
	witness::Trial t = witness::resolve<std::set<int>>("set is a set", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_set] falsifiability: a set-size lower bound is falsified") {
	witness::Generator<int> elem = witness::gen_int_range(0, 10);
	witness::Generator<std::set<int>> gen = witness::gen_set<int>(elem);
	std::function<bool(const std::set<int> &)> pred = [](const std::set<int> &s) {
		return s.size() >= 5;
	};
	witness::Trial t = witness::resolve<std::set<int>>("set size >= 5", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_map] keys are unique by std::map's own invariant") {
	witness::Generator<int> key = witness::gen_int_range(0, 20);
	witness::Generator<int> val = &witness::gen_int;
	witness::Generator<std::map<int, int>> gen = witness::gen_map<int, int>(key, val);
	std::function<bool(const std::map<int, int> &)> pred = [](const std::map<int, int> &) { return true; };
	witness::Trial t = witness::resolve<std::map<int, int>>("map keys unique", gen, pred);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[gen_map] falsifiability: a map-size lower bound is falsified") {
	witness::Generator<int> key = witness::gen_int_range(0, 20);
	witness::Generator<int> val = &witness::gen_int;
	witness::Generator<std::map<int, int>> gen = witness::gen_map<int, int>(key, val);
	std::function<bool(const std::map<int, int> &)> pred = [](const std::map<int, int> &m) {
		return m.size() >= 10;
	};
	witness::Trial t = witness::resolve<std::map<int, int>>("map size >= 10", gen, pred);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[gen_optional] populates both nullopt and value") {
	witness::Generator<int> elem = &witness::gen_int;
	witness::Generator<std::optional<int>> gen = witness::gen_optional<int>(elem);
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int nullc = 0;
	int somec = 0;
	for (int i = 0; i < 200; ++i) {
		std::optional<int> o = gen(rng, lvl);
		if (o.has_value()) {
			somec++;
		} else {
			nullc++;
		}
	}
	CHECK(nullc > 0);
	CHECK(somec > 0);
}

TEST_CASE("[gen_optional] falsifiability: a broken always-value generator has zero nullopts") {
	witness::Generator<std::optional<int>> broken = [](witness::RNG &r, const witness::Level &l) -> std::optional<int> {
		return witness::gen_int(r, l);
	};
	witness::RNG rng(0xC0FFEEULL);
	witness::Level lvl = witness::DEFAULT_LADDER[2];
	int nullc = 0;
	for (int i = 0; i < 200; ++i) {
		if (!broken(rng, lvl).has_value()) {
			nullc++;
		}
	}
	CHECK(nullc == 0);
}
