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

// Godot-side shim for V-Sekai-fire/witness-cpp — pulled in as a git
// subtree at tests/property/witness-cpp. The shim does NOT include
// witness/doctest.h (that header pulls <doctest/doctest.h>, which is
// a different install path than Godot's thirdparty/doctest). Godot's
// tests/test_macros.h supplies TEST_CASE, SUBCASE, CHECK and INFO
// from the same doctest distribution.

#include "thirdparty/witness-cpp/include/witness/ladder.h"
#include "tests/test_macros.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace TestWitness {

static std::vector<int> gen_int_vec(::witness::RNG &rng, const ::witness::Level &lvl) {
	uint32_t n = rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32));
	std::vector<int> v(n);
	for (uint32_t i = 0; i < n; ++i) {
		v[i] = rng.int_range(-100, 100);
	}
	return v;
}

TEST_CASE("[Witness] reverse . reverse is identity on std::vector<int>") {
	::witness::Generator<std::vector<int>> gen = &gen_int_vec;
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		std::vector<int> r = v;
		std::reverse(r.begin(), r.end());
		std::reverse(r.begin(), r.end());
		return r == v;
	};
	::witness::Trial t = ::witness::resolve<std::vector<int>>("involution", gen, pred);
	INFO(t.message);
	CHECK(t.outcome != ::witness::Outcome::FOUND);
}

TEST_CASE("[Witness] falsifiability: a planted-false property is caught") {
	::witness::Generator<std::vector<int>> gen = &gen_int_vec;
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
		return v.empty();
	};
	::witness::Trial t = ::witness::resolve<std::vector<int>>("vectors always empty", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
	CHECK(t.level == 0);
}

TEST_CASE("[Witness] the reproducer seed lands in the FOUND message") {
	::witness::Generator<int> gen = &::witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	::witness::Trial t = ::witness::resolve<int>("always-false", gen, pred, 0xC0FFEEULL);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
	CHECK(t.message.find("property_seed=0xc0ffee") != std::string::npos);
}

TEST_CASE("[Witness] falsifiability: a different seed yields a different message") {
	::witness::Generator<int> gen = &::witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	::witness::Trial a = ::witness::resolve<int>("always-false", gen, pred, 0x1ULL);
	::witness::Trial b = ::witness::resolve<int>("always-false", gen, pred, 0x2ULL);
	CHECK(a.message != b.message);
}

TEST_CASE("[Witness] assume + classify flow through to the report") {
	::witness::Generator<int> gen = &::witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &n) {
		::witness::assume(n >= -1000000);
		::witness::classify(n > 0, "positive");
		return true;
	};
	::witness::Trial t = ::witness::resolve<int>("assume + classify", gen, pred, 0xBEEFULL);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
	INFO(t.message);
	CHECK(t.message.find("classifications:") != std::string::npos);
	CHECK(t.message.find("positive") != std::string::npos);
}

TEST_CASE("[Witness] falsifiability: a label that never matches never appears") {
	::witness::Generator<int> gen = &::witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) {
		::witness::classify(false, "unreachable");
		return true;
	};
	::witness::Trial t = ::witness::resolve<int>("always-hold", gen, pred, 0xBEEFULL);
	CHECK(t.message.find("unreachable") == std::string::npos);
}

TEST_CASE("[Witness] shrinker reduces a falsifying vector toward the minimum") {
	::witness::Generator<std::vector<int>> gen = [](::witness::RNG &rng, const ::witness::Level &lvl) {
		std::vector<int> v(rng.uint_range(20, static_cast<uint32_t>(lvl.fin_bound / 4) + 20));
		for (std::size_t i = 0; i < v.size(); ++i) {
			v[i] = rng.int_range(-100, 100);
		}
		return v;
	};
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) { return v.empty(); };
	::witness::Shrinker<std::vector<int>> sh = &::witness::shrink_vector<int>;
	std::function<void(std::ostream &, const std::vector<int> &)> printer =
			[](std::ostream &os, const std::vector<int> &v) { os << "size=" << v.size(); };
	::witness::Trial t = ::witness::resolve<std::vector<int>>("shrink toward minimum", gen, pred, sh, printer);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
	CHECK(t.message.find("shrunk ") != std::string::npos);
}

TEST_CASE("[Witness] falsifiability: no_shrink leaves the counterexample untouched") {
	::witness::Generator<std::vector<int>> gen = [](::witness::RNG &rng, const ::witness::Level &lvl) {
		std::vector<int> v(rng.uint_range(20, static_cast<uint32_t>(lvl.fin_bound / 4) + 20));
		for (std::size_t i = 0; i < v.size(); ++i) {
			v[i] = rng.int_range(-100, 100);
		}
		return v;
	};
	std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) { return v.empty(); };
	::witness::Trial t = ::witness::resolve<std::vector<int>>("no_shrink control", gen, pred);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
	CHECK(t.message.find("shrunk ") == std::string::npos);
}

} // namespace TestWitness

TEST_FORCE_LINK(test_witness)
