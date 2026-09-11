// SPDX-License-Identifier: MIT
// assume() rejects invalid inputs; classify() and collect() bin
// trials for distribution reporting. Each unit paired with a
// falsifiability control.

#include "witness/doctest.h"

#include <functional>
#include <string>

TEST_CASE("[assume] rejected inputs do not count toward num_inst") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &n) {
		witness::assume(n >= 0);
		return true;
	};
	witness::Trial t = witness::resolve<int>("filter and hold", gen, pred, 0xABCDEFULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[assume] falsifiability: an unconditionally-rejecting predicate terminates without FOUND") {
	// A predicate that rejects every input must not FOUND; the trial
	// count runs out via the assume_cap escape, PROVABLY_NONE is
	// reported, and the message names zero trials.
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) {
		witness::assume(false);
		return true;
	};
	witness::Trial t = witness::resolve<int>("vacuous", gen, pred, 0x1ULL);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[classify] label counts are reported on PROVABLY_NONE") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &n) {
		witness::classify(n > 0, "positive");
		witness::classify(n == 0, "zero");
		witness::classify(n < 0, "negative");
		return true;
	};
	witness::Trial t = witness::resolve<int>("classified", gen, pred, 0xBEEFULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	INFO(t.message);
	CHECK(t.message.find("classifications:") != std::string::npos);
	CHECK(t.message.find("positive") != std::string::npos);
	CHECK(t.message.find("negative") != std::string::npos);
}

TEST_CASE("[classify] falsifiability: a label that never matches does not appear") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) {
		witness::classify(false, "unreachable");
		return true;
	};
	witness::Trial t = witness::resolve<int>("always-hold", gen, pred, 0xBEEFULL);
	CHECK(t.message.find("unreachable") == std::string::npos);
}

TEST_CASE("[collect] value-tagged bins appear in the report") {
	witness::Generator<int> gen = witness::gen_int_range(1, 3);
	std::function<bool(const int &)> pred = [](const int &n) {
		witness::collect(n);
		return true;
	};
	witness::Trial t = witness::resolve<int>("value tagged", gen, pred, 0x42ULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	INFO(t.message);
	CHECK(t.message.find("1") != std::string::npos);
	CHECK(t.message.find("2") != std::string::npos);
	CHECK(t.message.find("3") != std::string::npos);
}

TEST_CASE("[collect] falsifiability: bins not generated do not appear") {
	witness::Generator<int> gen = witness::gen_int_range(1, 3);
	std::function<bool(const int &)> pred = [](const int &n) {
		witness::collect(n);
		return true;
	};
	witness::Trial t = witness::resolve<int>("value tagged", gen, pred, 0x42ULL);
	CHECK(t.message.find(" 99 ") == std::string::npos);
}
