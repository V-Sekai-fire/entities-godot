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
// return value: no exception unwind anywhere. Shrinking is layered on
// top of that by iterating over smaller candidates — a caller-supplied
// `shrink(input) -> std::vector<T>` producing strictly smaller
// candidates. Matches Plausible's `Shrinkable` instance shape: at each
// step the current failing input is replaced by the first candidate
// that still falsifies the predicate; the loop ends when no candidate
// falsifies or the shrink budget is spent. The shrunk minimum surfaces
// through `Trial::message`, formatted by an optional `to_string`
// callable so the failure log names the minimal counterexample.
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

// Shrink a falsifying input toward its minimum. `shrinker(x)` returns a
// list of strictly smaller candidates; the first one that still
// falsifies replaces `x`, and the loop repeats until no candidate
// falsifies or `budget` iterations pass. Non-throwing, deterministic
// given a pure predicate + pure shrinker.
template <class T, class Pred, class Shrink>
inline T shrink_input(T input, Pred &&predicate, Shrink &&shrinker, int budget = 128, int *r_iterations = nullptr) {
	int iterations = 0;
	for (int i = 0; i < budget; ++i) {
		auto candidates = shrinker(input);
		bool improved = false;
		for (auto &c : candidates) {
			if (!predicate(c)) {
				input = std::move(c);
				improved = true;
				iterations++;
				break;
			}
		}
		if (!improved) {
			break;
		}
	}
	if (r_iterations) {
		*r_iterations = iterations;
	}
	return input;
}

// A no-op shrinker for callers that do not supply one. Returns an empty
// candidate set, so `shrink_input` terminates immediately.
struct NoShrink {
	template <class T>
	std::vector<T> operator()(const T &) const {
		return {};
	}
};

// A no-op printer for callers that do not supply one. Emits nothing.
struct NoPrint {
	template <class T>
	void operator()(std::ostream &, const T &) const {}
};

namespace detail {

// Format a Trial's terminating message. `printer(os, input)` runs when
// present to inject the input's value into the log.
template <class T, class Printer>
inline std::string format_found(const char *query, const Level &lvl, int trial_idx, const T &input, int shrink_iters, Printer &&printer) {
	std::ostringstream o;
	o << query << " falsified at level " << lvl.idx
	  << " trial " << trial_idx
	  << " (walk_steps=" << lvl.walk_steps
	  << " fin_bound=" << lvl.fin_bound << ")";
	if (shrink_iters > 0) {
		o << "; shrunk " << shrink_iters << " step(s)";
	}
	std::ostringstream body;
	printer(body, input);
	if (!body.str().empty()) {
		o << "; minimum=" << body.str();
	}
	return o.str();
}

inline std::string format_provably_none(const char *query) {
	std::ostringstream o;
	o << query << " held across the ladder (" << (int)(sizeof(DEFAULT_LADDER) / sizeof(Level)) << " rungs)";
	return o.str();
}

} // namespace detail

// Full resolver: generator + predicate + shrinker + printer.
// Predicate returns true when the property holds on this input.
// Returns the terminating Trial: FOUND with the shrunk minimum in the
// message, or PROVABLY_NONE after every rung's num_inst trials held the
// predicate.
template <class Gen, class Pred, class Shrink, class Printer>
inline Trial resolve(const char *query, Gen &&make_input, Pred &&predicate,
		Shrink &&shrinker, Printer &&printer, uint64_t seed = 0xC0FFEEULL) {
	RNG rng(seed);
	for (const Level &lvl : DEFAULT_LADDER) {
		for (int t = 0; t < lvl.num_inst; ++t) {
			auto input = make_input(rng, lvl);
			if (!predicate(input)) {
				int shrink_iters = 0;
				auto minimum = shrink_input(std::move(input), predicate, shrinker, /*budget=*/128, &shrink_iters);
				return { Outcome::FOUND, lvl.idx, t,
					detail::format_found(query, lvl, t, minimum, shrink_iters, printer) };
			}
		}
	}
	return { Outcome::PROVABLY_NONE, DEFAULT_LADDER[2].idx, 0, detail::format_provably_none(query) };
}

// No-shrinker convenience overload.
template <class Gen, class Pred>
inline Trial resolve(const char *query, Gen &&make_input, Pred &&predicate, uint64_t seed = 0xC0FFEEULL) {
	return resolve(query, std::forward<Gen>(make_input), std::forward<Pred>(predicate),
			NoShrink{}, NoPrint{}, seed);
}

// Default shrinkers for common types. Each returns strictly smaller
// candidates; empty vector when no smaller candidate exists.

// Integers: try 0, sign-flipped, and halved. In that order — 0 is the
// smallest sensible witness; sign flip converts a large negative to a
// large positive (still large but often surfaces sign-dependent bugs);
// halving is the classic monotone reducer.
inline std::vector<int> shrink_int(int n) {
	std::vector<int> out;
	if (n != 0) {
		out.push_back(0);
	}
	if (n < 0) {
		out.push_back(-n);
	}
	int halved = n / 2;
	if (halved != n) {
		out.push_back(halved);
	}
	return out;
}

// std::vector<T>: try dropping each element and halving the length.
// Preserves relative order — element-drop is the standard shape-reducer.
template <class T>
inline std::vector<std::vector<T>> shrink_vector(const std::vector<T> &v) {
	std::vector<std::vector<T>> out;
	if (v.empty()) {
		return out;
	}
	// Half-length prefix.
	if (v.size() > 1) {
		out.emplace_back(v.begin(), v.begin() + v.size() / 2);
	}
	// One-element removed, from the front.
	for (std::size_t i = 0; i < v.size(); ++i) {
		std::vector<T> smaller;
		smaller.reserve(v.size() - 1);
		smaller.insert(smaller.end(), v.begin(), v.begin() + i);
		smaller.insert(smaller.end(), v.begin() + i + 1, v.end());
		out.emplace_back(std::move(smaller));
	}
	return out;
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

// PROP_CHECK_SHRINK runs the ladder with a caller-supplied shrinker
// and printer. On FOUND the printer emits the minimum counterexample
// into the doctest INFO log.
#define PROP_CHECK_SHRINK(m_query, m_make_input, m_predicate, m_shrinker, m_printer) \
	SUBCASE(m_query) { \
		::property::Trial _prop_trial = ::property::resolve(m_query, m_make_input, m_predicate, m_shrinker, m_printer); \
		INFO(_prop_trial.message); \
		CHECK(_prop_trial.outcome != ::property::Outcome::FOUND); \
	}

#endif // PROPERTY_LADDER_H
