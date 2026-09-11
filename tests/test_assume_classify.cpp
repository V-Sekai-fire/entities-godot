// SPDX-License-Identifier: MIT
// assume() rejects invalid inputs; classify() collects distribution
// stats reported on PROVABLY_NONE.

#include "witness/doctest.h"

#include <string>

TEST_CASE("[assume] rejected inputs don't count toward num_inst") {
	// A predicate that assume()-rejects half its inputs, then holds on
	// the rest. The resolve must still walk the full ladder without
	// FOUND. Historically a bug would have counted the SKIP toward
	// num_inst and terminated early — this test catches that.
	witness::Trial t = witness::resolve(
			"filter and hold",
			witness::gen_int,
			[](int n) {
				witness::assume(n >= 0); // reject negatives
				return true;
			},
			0xABCDEFULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[assume] a predicate that assumes-away everything terminates cleanly") {
	// Sanity: if every input is rejected, the resolver bails out of
	// the rung once assumed_skipped exceeds num_inst * 10 rather than
	// spinning forever. Must terminate; must not FOUND.
	witness::Trial t = witness::resolve(
			"vacuous predicate",
			witness::gen_int,
			[](int) {
				witness::assume(false);
				return true;
			},
			0x1ULL);
	CHECK(t.outcome != witness::Outcome::FOUND);
}

TEST_CASE("[classify] label counts are reported on PROVABLY_NONE") {
	witness::Trial t = witness::resolve(
			"always-hold with classification",
			witness::gen_int,
			[](int n) {
				witness::classify(n > 0, "positive");
				witness::classify(n == 0, "zero");
				witness::classify(n < 0, "negative");
				return true;
			},
			0xBEEFULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	INFO(t.message);
	CHECK(t.message.find("classifications:") != std::string::npos);
	// With gen_int centered on zero the distribution should mention
	// both signs and zero over 3000 trials.
	CHECK(t.message.find("positive") != std::string::npos);
	CHECK(t.message.find("negative") != std::string::npos);
}

TEST_CASE("[classify] negative control: a label that never matches never appears") {
	// Rule-2 pair for [classify]: if the classify condition is
	// always false, the label must not appear in the report.
	witness::Trial t = witness::resolve(
			"always-hold, always-false classify",
			witness::gen_int,
			[](int) {
				witness::classify(false, "unreachable");
				return true;
			},
			0xBEEFULL);
	CHECK(t.outcome == witness::Outcome::PROVABLY_NONE);
	CHECK(t.message.find("unreachable") == std::string::npos);
}
