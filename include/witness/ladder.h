// SPDX-License-Identifier: MIT
// witness-cpp: exception-free property-testing ladder for C++.
//
// Modeled on two upstream shapes:
//
//   1. github.com/fire/plausible-witness-dag — Lean/Lake library that
//      layers an iterative-deepening ladder + deterministic readback
//      over Plausible. The Level struct here mirrors its
//      { walkSteps, finBound, numInst } rungs and the DEFAULT_LADDER
//      values match the Lean defaults verbatim.
//   2. github.com/leanprover-community/plausible — the underlying
//      property-testing library. `num_inst` semantics and the
//      "counterexample surfaces a witness for the negated statement"
//      framing come from there. The Trial::Outcome triple
//      (FOUND / PROVABLY_NONE / BUDGET_HIT) reproduces plausible's own
//      Testable.checkIO result classification.
//
// Feature-complete (no exceptions used anywhere):
//
//   • Iterative-deepening ladder with three rungs.
//   • Deterministic seeded RNG.
//   • Generator + Predicate contract.
//   • Shrinking: iterative, non-throwing, with a 128-step budget.
//   • Built-in generators: gen_int, gen_uint, gen_int64, gen_float,
//     gen_double, gen_string, gen_vector<T>, gen_pair<A,B>,
//     gen_optional<T>.
//   • Built-in shrinkers: shrink_int, shrink_uint, shrink_int64,
//     shrink_float, shrink_double, shrink_string, shrink_vector<T>,
//     shrink_pair<A,B>, shrink_optional<T>.
//   • Reproducer seed: FOUND messages include
//     "re-run with property_seed=0xNNNN" so a failing run reproduces.
//     Default seed comes from std::random_device; override with
//     PROPERTY_SEED env var or the seed argument.
//   • assume(cond): mark this trial as invalid and resample — the
//     trial does NOT count toward num_inst.
//   • classify(cond, label): tag this trial with a label; the label's
//     hit count and % across all HOLD trials is reported on
//     PROVABLY_NONE for distribution insight.
//
// Header-only. C++17. Works with -fno-exceptions.

#ifndef WITNESS_LADDER_H
#define WITNESS_LADDER_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace witness {

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

enum class Verdict {
	HOLD, // property held on this input
	FALSIFY, // property falsified — captured as the counterexample
	SKIP, // input rejected by assume(); retry with a new sample
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
	int64_t int64_range(int64_t lo, int64_t hi) {
		return std::uniform_int_distribution<int64_t>(lo, hi)(gen);
	}
	double float_range(double lo, double hi) {
		return std::uniform_real_distribution<double>(lo, hi)(gen);
	}
	uint64_t next_u64() { return gen(); }
};

// -------------------------------------------------------------------
// Trial-local state: assume() + classify() reach it here. Kept
// thread_local so tests running in parallel over separate threads
// each see their own state; single-threaded tests (the common case)
// share one instance harmlessly.
// -------------------------------------------------------------------
struct Ctx {
	Verdict verdict = Verdict::HOLD;
	std::map<std::string, int64_t> class_hits;
	int64_t trials_counted = 0;
};

inline Ctx &ctx() {
	static thread_local Ctx c;
	return c;
}

// Rejects a generated input inside the predicate body; the trial is
// not counted and a fresh input is generated. Meant to be called from
// inside a predicate that returns bool: the predicate should then
// early-return `true` — the resolver reads the SKIP verdict from
// ctx() and treats the return value as no-op.
inline void assume(bool cond) {
	if (!cond) {
		ctx().verdict = Verdict::SKIP;
	}
}

// Tallies how many of the counted trials satisfy `cond`. The
// distribution is reported on PROVABLY_NONE in Trial::message.
inline void classify(bool cond, const std::string &label) {
	if (cond) {
		ctx().class_hits[label]++;
	}
}

// -------------------------------------------------------------------
// Print helpers. `print_value(os, x)` uses operator<< when available;
// falls back to "<opaque>" so predicates over non-streamable types
// still surface a message.
// -------------------------------------------------------------------
namespace detail {

template <class, class = void>
struct is_streamable : std::false_type {};

template <class T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream &>() << std::declval<const T &>())>> :
		std::true_type {};

template <class T>
inline void print_value(std::ostream &os, const T &x) {
	if constexpr (is_streamable<T>::value) {
		os << x;
	} else {
		os << "<opaque>";
	}
}

} // namespace detail

// -------------------------------------------------------------------
// Generators. Each has the signature `T (RNG &, const Level &)` so it
// can plug directly into resolve() without a wrapping lambda.
// -------------------------------------------------------------------
inline bool gen_bool(RNG &rng, const Level &) {
	return rng.int_range(0, 1) == 1;
}

inline char gen_char(RNG &rng, const Level &) {
	// Printable ASCII so falsifier printouts stay readable.
	return static_cast<char>(rng.int_range(32, 126));
}

inline int gen_int(RNG &rng, const Level &lvl) {
	int scale = std::max(1, lvl.fin_bound / 32);
	return rng.int_range(-scale, scale);
}

inline uint32_t gen_uint(RNG &rng, const Level &lvl) {
	uint32_t scale = std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32));
	return rng.uint_range(0, scale);
}

inline int64_t gen_int64(RNG &rng, const Level &lvl) {
	int64_t scale = std::max<int64_t>(1, lvl.fin_bound);
	return rng.int64_range(-scale, scale);
}

inline double gen_double(RNG &rng, const Level &lvl) {
	double scale = std::max(1.0, static_cast<double>(lvl.fin_bound));
	return rng.float_range(-scale, scale);
}

inline float gen_float(RNG &rng, const Level &lvl) {
	return static_cast<float>(gen_double(rng, lvl));
}

inline std::string gen_string(RNG &rng, const Level &lvl) {
	uint32_t len = rng.uint_range(0, std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32)));
	std::string s;
	s.reserve(len);
	for (uint32_t i = 0; i < len; ++i) {
		// Printable ASCII so the reproducer message stays readable.
		s.push_back(static_cast<char>(rng.int_range(32, 126)));
	}
	return s;
}

template <class ElemGen>
inline auto gen_vector(ElemGen elem_gen) {
	return [elem_gen](RNG &rng, const Level &lvl) {
		uint32_t n = rng.uint_range(0, std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32)));
		using Elem = decltype(elem_gen(rng, lvl));
		std::vector<Elem> v;
		v.reserve(n);
		for (uint32_t i = 0; i < n; ++i) {
			v.push_back(elem_gen(rng, lvl));
		}
		return v;
	};
}

template <class GenA, class GenB>
inline auto gen_pair(GenA a_gen, GenB b_gen) {
	return [a_gen, b_gen](RNG &rng, const Level &lvl) {
		return std::pair(a_gen(rng, lvl), b_gen(rng, lvl));
	};
}

template <class ElemGen>
inline auto gen_optional(ElemGen elem_gen) {
	return [elem_gen](RNG &rng, const Level &lvl) -> std::optional<decltype(elem_gen(rng, lvl))> {
		if (rng.int_range(0, 3) == 0) {
			return std::nullopt;
		}
		return elem_gen(rng, lvl);
	};
}

// Variadic tuple generator: `gen_tuple(gen_int, gen_string, gen_bool)`
// yields a std::tuple<int, std::string, bool>.
template <class... Gens>
inline auto gen_tuple(Gens... gens) {
	return [gens...](RNG &rng, const Level &lvl) {
		return std::make_tuple(gens(rng, lvl)...);
	};
}

// Pick one of several homogeneous generators uniformly at random.
// For heterogeneous generators, wrap each in a std::function of the
// common return type.
template <class Gen>
inline auto gen_oneof(std::vector<Gen> gens) {
	return [gens](RNG &rng, const Level &lvl) {
		int i = rng.int_range(0, static_cast<int>(gens.size()) - 1);
		return gens[static_cast<std::size_t>(i)](rng, lvl);
	};
}

// Map a function over a generator's output.
template <class Gen, class Fn>
inline auto gen_transform(Gen gen, Fn fn) {
	return [gen, fn](RNG &rng, const Level &lvl) {
		return fn(gen(rng, lvl));
	};
}

// Filter a generator: resample up to `retries` times until `pred`
// holds. On exhaustion the last draw is returned and assume() marks
// the trial as SKIP so the resolver drops it.
template <class Gen, class Pred>
inline auto gen_filter(Gen gen, Pred pred, int retries = 32) {
	return [gen, pred, retries](RNG &rng, const Level &lvl) {
		auto v = gen(rng, lvl);
		for (int i = 0; i < retries; ++i) {
			if (pred(v)) {
				return v;
			}
			v = gen(rng, lvl);
		}
		if (!pred(v)) {
			ctx().verdict = Verdict::SKIP;
		}
		return v;
	};
}

// -------------------------------------------------------------------
// Shrinkers. Each takes an input and returns strictly-smaller
// candidates; the caller's shrink loop picks the first that still
// falsifies.
// -------------------------------------------------------------------
inline std::vector<bool> shrink_bool(bool b) {
	// Only smaller value is false; false already at the minimum.
	if (b) {
		return { false };
	}
	return {};
}

inline std::vector<char> shrink_char(char c) {
	// 'a' is the canonical minimum for readable output; step toward it.
	std::vector<char> out;
	if (c != 'a') {
		out.push_back('a');
	}
	if (c > 'a') {
		out.push_back(static_cast<char>(c - 1));
	}
	return out;
}

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

inline std::vector<uint32_t> shrink_uint(uint32_t n) {
	std::vector<uint32_t> out;
	if (n != 0) {
		out.push_back(0);
	}
	uint32_t halved = n / 2;
	if (halved != n) {
		out.push_back(halved);
	}
	return out;
}

inline std::vector<int64_t> shrink_int64(int64_t n) {
	std::vector<int64_t> out;
	if (n != 0) {
		out.push_back(0);
	}
	if (n < 0) {
		out.push_back(-n);
	}
	int64_t halved = n / 2;
	if (halved != n) {
		out.push_back(halved);
	}
	return out;
}

inline std::vector<double> shrink_double(double x) {
	std::vector<double> out;
	if (x != 0.0) {
		out.push_back(0.0);
	}
	if (x < 0.0) {
		out.push_back(-x);
	}
	double halved = x / 2.0;
	if (halved != x) {
		out.push_back(halved);
	}
	return out;
}

inline std::vector<float> shrink_float(float x) {
	std::vector<float> out;
	if (x != 0.0f) {
		out.push_back(0.0f);
	}
	if (x < 0.0f) {
		out.push_back(-x);
	}
	float halved = x / 2.0f;
	if (halved != x) {
		out.push_back(halved);
	}
	return out;
}

inline std::vector<std::string> shrink_string(const std::string &s) {
	std::vector<std::string> out;
	if (s.empty()) {
		return out;
	}
	if (s.size() > 1) {
		out.emplace_back(s.begin(), s.begin() + s.size() / 2);
	}
	for (std::size_t i = 0; i < s.size(); ++i) {
		std::string smaller;
		smaller.reserve(s.size() - 1);
		smaller.append(s, 0, i);
		smaller.append(s, i + 1, std::string::npos);
		out.emplace_back(std::move(smaller));
	}
	return out;
}

template <class T>
inline std::vector<std::vector<T>> shrink_vector(const std::vector<T> &v) {
	std::vector<std::vector<T>> out;
	if (v.empty()) {
		return out;
	}
	if (v.size() > 1) {
		out.emplace_back(v.begin(), v.begin() + v.size() / 2);
	}
	for (std::size_t i = 0; i < v.size(); ++i) {
		std::vector<T> smaller;
		smaller.reserve(v.size() - 1);
		smaller.insert(smaller.end(), v.begin(), v.begin() + i);
		smaller.insert(smaller.end(), v.begin() + i + 1, v.end());
		out.emplace_back(std::move(smaller));
	}
	return out;
}

template <class A, class B, class ShrinkA, class ShrinkB>
inline auto shrink_pair(const std::pair<A, B> &p, ShrinkA a_shrink, ShrinkB b_shrink) {
	std::vector<std::pair<A, B>> out;
	for (auto &a : a_shrink(p.first)) {
		out.emplace_back(std::move(a), p.second);
	}
	for (auto &b : b_shrink(p.second)) {
		out.emplace_back(p.first, std::move(b));
	}
	return out;
}

// Tuple shrinker: shrinks one slot at a time. Callers pass one
// shrinker per slot in the same order.
namespace detail {

template <std::size_t I, class Tup, class ShrinkI>
inline void shrink_one_slot(const Tup &orig, ShrinkI &&shrinker, std::vector<Tup> &out) {
	auto candidates = shrinker(std::get<I>(orig));
	for (auto &c : candidates) {
		Tup copy = orig;
		std::get<I>(copy) = std::move(c);
		out.push_back(std::move(copy));
	}
}

template <class Tup, class Shrinks, std::size_t... Is>
inline std::vector<Tup> shrink_tuple_impl(const Tup &t, Shrinks &shrinks, std::index_sequence<Is...>) {
	std::vector<Tup> out;
	(shrink_one_slot<Is>(t, std::get<Is>(shrinks), out), ...);
	return out;
}

} // namespace detail

template <class... Ts, class... Shrinks>
inline std::vector<std::tuple<Ts...>> shrink_tuple(const std::tuple<Ts...> &t, Shrinks... shrinks) {
	auto pack = std::make_tuple(shrinks...);
	return detail::shrink_tuple_impl(t, pack, std::index_sequence_for<Ts...>{});
}

template <class T, class ShrinkT>
inline std::vector<std::optional<T>> shrink_optional(const std::optional<T> &x, ShrinkT t_shrink) {
	std::vector<std::optional<T>> out;
	if (x.has_value()) {
		out.emplace_back(std::nullopt);
		for (auto &t : t_shrink(*x)) {
			out.emplace_back(std::move(t));
		}
	}
	return out;
}

// -------------------------------------------------------------------
// Shrinking driver. Non-throwing; iterates smaller candidates until
// none falsifies or the budget is spent.
// -------------------------------------------------------------------
template <class T, class Pred, class Shrink>
inline T shrink_input(T input, Pred &&predicate, Shrink &&shrinker,
		int budget = 128, int *r_iterations = nullptr) {
	int iterations = 0;
	for (int i = 0; i < budget; ++i) {
		auto candidates = shrinker(input);
		bool improved = false;
		for (auto &c : candidates) {
			// Fresh trial context so assume/classify don't leak
			// state into the shrink measurement.
			Ctx saved = ctx();
			ctx() = Ctx{};
			bool held = static_cast<bool>(predicate(c));
			Verdict v = ctx().verdict;
			ctx() = saved;
			// A skipped shrink candidate is not smaller in the sense
			// that matters — try the next one.
			if (v == Verdict::SKIP) {
				continue;
			}
			if (!held) {
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

// No-op shrinker / printer defaults.
struct NoShrink {
	template <class T>
	std::vector<T> operator()(const T &) const {
		return {};
	}
};

struct NoPrint {
	template <class T>
	void operator()(std::ostream &, const T &) const {}
};

// Default printer: uses operator<< when available.
struct AutoPrint {
	template <class T>
	void operator()(std::ostream &os, const T &x) const {
		detail::print_value(os, x);
	}
};

namespace detail {

template <class T, class Printer>
inline std::string format_found(const char *query, const Level &lvl, int trial_idx,
		const T &input, int shrink_iters, Printer &&printer, uint64_t seed) {
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
	o << "; re-run with property_seed=0x" << std::hex << seed << std::dec;
	return o.str();
}

inline std::string format_provably_none(const char *query, uint64_t seed) {
	std::ostringstream o;
	o << query << " held across the ladder ("
	  << static_cast<int>(sizeof(DEFAULT_LADDER) / sizeof(Level)) << " rungs)"
	  << "; seed=0x" << std::hex << seed << std::dec;
	const auto &hits = ctx().class_hits;
	int64_t total = ctx().trials_counted;
	if (!hits.empty() && total > 0) {
		o << "; classifications:";
		bool first = true;
		for (auto &[label, n] : hits) {
			if (!first) {
				o << ",";
			}
			first = false;
			int pct = static_cast<int>((100 * n + total / 2) / total);
			o << " " << label << " " << pct << "% (" << n << "/" << total << ")";
		}
	}
	return o.str();
}

} // namespace detail

// Seed strategy: caller-supplied seed → PROPERTY_SEED env var → random.
inline uint64_t pick_seed(uint64_t caller_seed) {
	if (caller_seed != 0) {
		return caller_seed;
	}
	if (const char *env = std::getenv("PROPERTY_SEED")) {
		// Non-throwing parse — -fno-exceptions is in force. Accept
		// decimal and 0x-prefixed hex; on any parse failure fall
		// through to std::random_device.
		char *end = nullptr;
		uint64_t parsed = std::strtoull(env, &end, 0);
		if (end != env && *end == '\0' && parsed != 0) {
			return parsed;
		}
	}
	std::random_device rd;
	uint64_t s = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
	if (s == 0) {
		s = 0xC0FFEEULL;
	}
	return s;
}

// Wrap a bool-returning predicate as a Verdict-returning one, honoring
// assume()'s SKIP verdict. If the generator already set SKIP (via
// gen_filter's retry-exhaustion path), the predicate is not run.
template <class Pred, class T>
inline Verdict run_predicate(Pred &&predicate, const T &input) {
	if (ctx().verdict == Verdict::SKIP) {
		return Verdict::SKIP;
	}
	ctx().verdict = Verdict::HOLD;
	bool held = static_cast<bool>(predicate(input));
	if (ctx().verdict == Verdict::SKIP) {
		return Verdict::SKIP;
	}
	ctx().verdict = held ? Verdict::HOLD : Verdict::FALSIFY;
	return ctx().verdict;
}

// Full resolver against a caller-supplied ladder. `ladder` is any
// range of Level values — a std::vector<Level>, a std::array, or a
// pointer + count via std::span-like usage.
template <class Ladder, class Gen, class Pred, class Shrink, class Printer>
inline Trial resolve_with_ladder(const char *query, const Ladder &ladder,
		Gen &&make_input, Pred &&predicate,
		Shrink &&shrinker, Printer &&printer, uint64_t seed = 0) {
	uint64_t actual_seed = pick_seed(seed);
	RNG rng(actual_seed);
	ctx().class_hits.clear();
	ctx().trials_counted = 0;

	int last_idx = 0;
	for (const Level &lvl : ladder) {
		last_idx = lvl.idx;
		int trial_counter = 0;
		int assumed_skipped = 0;
		const int assume_cap = lvl.num_inst * 10;
		while (trial_counter < lvl.num_inst) {
			if (assumed_skipped >= assume_cap) {
				break;
			}
			ctx().verdict = Verdict::HOLD;
			auto input = make_input(rng, lvl);
			Verdict v = run_predicate(predicate, input);
			if (v == Verdict::SKIP) {
				assumed_skipped++;
				continue;
			}
			if (v == Verdict::FALSIFY) {
				int shrink_iters = 0;
				auto minimum = shrink_input(std::move(input), predicate, shrinker,
						128, &shrink_iters);
				return { Outcome::FOUND, lvl.idx, trial_counter,
					detail::format_found(query, lvl, trial_counter, minimum, shrink_iters,
							printer, actual_seed) };
			}
			ctx().trials_counted++;
			trial_counter++;
		}
	}
	return { Outcome::PROVABLY_NONE, last_idx, 0,
		detail::format_provably_none(query, actual_seed) };
}

// Default resolver: walks DEFAULT_LADDER.
template <class Gen, class Pred, class Shrink, class Printer>
inline Trial resolve(const char *query, Gen &&make_input, Pred &&predicate,
		Shrink &&shrinker, Printer &&printer, uint64_t seed = 0) {
	return resolve_with_ladder(query, DEFAULT_LADDER,
			std::forward<Gen>(make_input), std::forward<Pred>(predicate),
			std::forward<Shrink>(shrinker), std::forward<Printer>(printer), seed);
}

// No-shrinker convenience overload.
template <class Gen, class Pred>
inline Trial resolve(const char *query, Gen &&make_input, Pred &&predicate, uint64_t seed = 0) {
	return resolve(query, std::forward<Gen>(make_input), std::forward<Pred>(predicate),
			NoShrink{}, NoPrint{}, seed);
}

} // namespace witness

#endif // WITNESS_LADDER_H
