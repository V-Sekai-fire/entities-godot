/**************************************************************************/
/*  property_ladder.h                                                     */
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

// Property tests without exceptions. In-tree, header-only, doctest-integrated.
//
// Modeled on two upstream shapes that already handle the concepts:
//
//   1. github.com/fire/plausible-witness-dag — Lean/Lake library that
//      layers an iterative-deepening ladder + deterministic readback
//      over Plausible. The Level struct here mirrors its
//      { walkSteps, finBound, numInst } rungs and the DEFAULT_LADDER
//      values match the Lean defaults verbatim.
//
//   2. github.com/leanprover-community/plausible — the underlying
//      property-testing library. Numbering (`num_inst`) and the
//      "counterexample surfaces a witness for the negated statement"
//      framing come from there. The Trial::Outcome triple
//      (FOUND / PROVABLY_NONE / BUDGET_HIT) reproduces plausible's own
//      Testable.checkIO result classification.
//
// Property tests classically use `throw` on a failed sample to unwind
// the trial and hand the input to a shrinker. Godot's tree is compiled
// with -fno-exceptions, so this harness reports failure via a plain
// return value; there is no exception-based unwind. Shrinking is v2 —
// with pure predicates it is straightforward to add iteratively over a
// caller-supplied shrink function, matching Plausible's Shrinkable
// instance shape.
//
// Usage:
//
//   #include "tests/test_macros.h"
//   #include "tests/property_ladder.h"
//
//   TEST_CASE("[Property] reverse . reverse is identity") {
//       PROP_CHECK("involution",
//           [](property::RNG &rng, const property::Level &lvl) {
//               std::vector<int> v(rng.uint_range(0, lvl.fin_bound / 32));
//               for (auto &x : v) x = rng.int_range(-100, 100);
//               return v;
//           },
//           [](const std::vector<int> &v) {
//               std::vector<int> r = v;
//               std::reverse(r.begin(), r.end());
//               std::reverse(r.begin(), r.end());
//               return r == v; // predicate: true when the property holds
//           });
//   }
//
// The macro PROP_CHECK runs the ladder and calls doctest's CHECK/INFO
// with the terminating Trial's diagnostic. No throw, no catch.

#ifndef PROPERTY_LADDER_H
#define PROPERTY_LADDER_H

#include "tests/test_macros.h"

#include <cstdint>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>

namespace property {

// One rung of the iterative-deepening ladder.
struct Level {
	int idx = 0;
	int walk_steps = 0; // caller-defined deterministic walk budget
	int fin_bound = 0; // candidate window for the generator
	int num_inst = 0; // random instances to try at this rung
};

// Default ladder mirrors plausible-witness-dag: cheap → wider.
inline constexpr Level DEFAULT_LADDER[3] = {
	{ 0, 64, 256, 200 },
	{ 1, 512, 1024, 800 },
	{ 2, 4000, 4096, 2000 },
};

enum class Outcome {
	FOUND, // predicate returned false — property falsified
	PROVABLY_NONE, // walked through all rungs, predicate held every trial
	BUDGET_HIT, // (reserved) trial exhausted its walk budget mid-check
};

// One trial's result — carries the rung, index and (when FOUND) a message.
struct Trial {
	Outcome outcome = Outcome::PROVABLY_NONE;
	int level = 0;
	int trial_idx = 0; // 0-based within the rung
	std::string message;
};

// Deterministic RNG. Same seed = same trial sequence.
struct RNG {
	std::mt19937_64 gen;

	explicit RNG(uint64_t seed = 0xC0FFEEULL) :
			gen(seed) {}

	int int_range(int lo, int hi) {
		return std::uniform_int_distribution<int>(lo, hi)(gen);
	}
	uint32_t uint_range(uint32_t lo, uint32_t hi) {
		return std::uniform_int_distribution<uint32_t>(lo, hi)(gen);
	}
	uint64_t next_u64() { return gen(); }
};

// Run one predicate through the ladder. Gen must be
// `T (RNG &, const Level &)` and Pred must be `bool (const T &)`.
// Predicate returns true when the property holds on this input.
//
// Returns the terminating Trial: FOUND with a message, or PROVABLY_NONE
// after every rung's num_inst trials held the predicate.
template <class Gen, class Pred>
inline Trial resolve(const char *query, Gen &&make_input, Pred &&predicate, uint64_t seed = 0xC0FFEEULL) {
	RNG rng(seed);
	for (const Level &lvl : DEFAULT_LADDER) {
		for (int t = 0; t < lvl.num_inst; ++t) {
			auto input = make_input(rng, lvl);
			if (!predicate(input)) {
				std::ostringstream o;
				o << query << " falsified at level " << lvl.idx
				  << " trial " << t
				  << " (walk_steps=" << lvl.walk_steps
				  << " fin_bound=" << lvl.fin_bound << ")";
				return { Outcome::FOUND, lvl.idx, t, o.str() };
			}
		}
	}
	std::ostringstream o;
	o << query << " held across the ladder (" << (int)(sizeof(DEFAULT_LADDER) / sizeof(Level)) << " rungs)";
	return { Outcome::PROVABLY_NONE, DEFAULT_LADDER[2].idx, 0, o.str() };
}

} // namespace property

// PROP_CHECK runs the ladder inside a doctest SUBCASE. INFO carries the
// terminating message; CHECK asserts the outcome is not FOUND.
#define PROP_CHECK(m_query, m_make_input, m_predicate) \
	SUBCASE(m_query) { \
		::property::Trial _prop_trial = ::property::resolve(m_query, m_make_input, m_predicate); \
		INFO(_prop_trial.message); \
		CHECK(_prop_trial.outcome != ::property::Outcome::FOUND); \
	}

#endif // PROPERTY_LADDER_H
