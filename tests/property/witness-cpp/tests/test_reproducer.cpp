// SPDX-License-Identifier: MIT
// Reproducer-seed guarantees:
//   1. The same explicit seed produces the same trial sequence.
//   2. The FOUND message names the seed so a failing run can be
//      re-run deterministically.
//   3. PROPERTY_SEED env var (set by the test harness itself, then
//      cleared) picks up as the fallback source.

#include "witness/doctest.h"

#include <cstdlib>
#include <string>

TEST_CASE("[reproducer] same explicit seed reproduces the same falsifier") {
	auto run = [](uint64_t seed) {
		return witness::resolve(
				"always-false",
				witness::gen_int,
				[](int) { return false; }, // falsifies on the first trial
				seed);
	};
	witness::Trial a = run(0xDEADBEEF);
	witness::Trial b = run(0xDEADBEEF);
	CHECK(a.outcome == witness::Outcome::FOUND);
	CHECK(b.outcome == witness::Outcome::FOUND);
	CHECK(a.message == b.message);
}

TEST_CASE("[reproducer] the FOUND message names the seed") {
	witness::Trial t = witness::resolve(
			"always-false",
			witness::gen_int,
			[](int) { return false; },
			0xC0FFEEULL);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
	CHECK(t.message.find("property_seed=0xc0ffee") != std::string::npos);
}

TEST_CASE("[reproducer] PROPERTY_SEED env picks up when no explicit seed is given") {
#if defined(_WIN32)
	_putenv_s("PROPERTY_SEED", "0x1234");
#else
	setenv("PROPERTY_SEED", "0x1234", 1);
#endif
	witness::Trial a = witness::resolve(
			"always-false",
			witness::gen_int,
			[](int) { return false; });
#if defined(_WIN32)
	_putenv_s("PROPERTY_SEED", "");
#else
	unsetenv("PROPERTY_SEED");
#endif
	CHECK(a.outcome == witness::Outcome::FOUND);
	CHECK(a.message.find("property_seed=0x1234") != std::string::npos);
}

TEST_CASE("[reproducer] negative control: different seeds give different messages") {
	// Rule-2 pair: prove that the seed actually influences output. If
	// pick_seed had a bug that ignored its argument, both trials would
	// produce identical messages and this control would fail.
	witness::Trial a = witness::resolve(
			"always-false", witness::gen_int,
			[](int) { return false; }, 0x1111);
	witness::Trial b = witness::resolve(
			"always-false", witness::gen_int,
			[](int) { return false; }, 0x2222);
	CHECK(a.message != b.message);
}
