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
// Header-only. C++17. Works with -fno-exceptions.

#ifndef WITNESS_LADDER_H
#define WITNESS_LADDER_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace witness {

struct Level {
	int idx = 0;
	int walk_steps = 0;
	int fin_bound = 0;
	int num_inst = 0;
};

inline constexpr Level DEFAULT_LADDER[3] = {
	{ 0, 64, 256, 200 },
	{ 1, 512, 1024, 800 },
	{ 2, 4000, 4096, 2000 },
};

enum class Outcome {
	FOUND,
	PROVABLY_NONE,
	BUDGET_HIT,
};

enum class Verdict {
	HOLD,
	FALSIFY,
	SKIP,
};

struct Trial {
	Outcome outcome = Outcome::PROVABLY_NONE;
	int level = 0;
	int trial_idx = 0;
	std::string message;
};

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

// Type-erased signatures. Naming these lets the whole library avoid
// `auto` return types: combinators produce Generator<T> objects that
// wrap capture-holding lambdas the caller could otherwise only hold
// through auto. Function pointers to the primitive gen_* functions
// convert to Generator<T> implicitly.
template <class T>
using Generator = std::function<T(RNG &, const Level &)>;

template <class T>
using Shrinker = std::function<std::vector<T>(const T &)>;

// Helper: the return type of a callable given (RNG&, const Level&).
template <class Gen>
using GenResult = std::invoke_result_t<Gen &, RNG &, const Level &>;

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

struct Ctx {
	Verdict verdict = Verdict::HOLD;
	std::map<std::string, int64_t> class_hits;
	int64_t trials_counted = 0;
};

inline Ctx &ctx() {
	static thread_local Ctx c;
	return c;
}

inline void assume(bool cond) {
	if (!cond) {
		ctx().verdict = Verdict::SKIP;
	}
}

inline void classify(bool cond, const std::string &label) {
	if (cond) {
		ctx().class_hits[label]++;
	}
}

// Value-tagged classifier: bins trials by the rendered form of `value`.
template <class T>
inline void collect(const T &value) {
	std::ostringstream o;
	detail::print_value(o, value);
	ctx().class_hits[o.str()]++;
}

// -------------------------------------------------------------------
// Primitive generators. Plain function pointers — they convert to
// Generator<T> at every call site with no wrapping cost.
// -------------------------------------------------------------------

inline bool gen_bool(RNG &rng, const Level &) {
	return rng.int_range(0, 1) == 1;
}

inline char gen_char(RNG &rng, const Level &) {
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

inline uint64_t gen_uint64(RNG &rng, const Level &lvl) {
	int64_t scale = std::max<int64_t>(1, lvl.fin_bound);
	return static_cast<uint64_t>(rng.int64_range(0, scale));
}

inline std::size_t gen_size(RNG &rng, const Level &lvl) {
	return static_cast<std::size_t>(gen_uint64(rng, lvl));
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
		s.push_back(static_cast<char>(rng.int_range(32, 126)));
	}
	return s;
}

// -------------------------------------------------------------------
// Ranged primitives. Return Generator<T> because they capture bounds.
// -------------------------------------------------------------------

inline Generator<int> gen_int_range(int lo, int hi) {
	return [lo, hi](RNG &rng, const Level &) -> int { return rng.int_range(lo, hi); };
}

inline Generator<uint32_t> gen_uint_range(uint32_t lo, uint32_t hi) {
	return [lo, hi](RNG &rng, const Level &) -> uint32_t { return rng.uint_range(lo, hi); };
}

inline Generator<int64_t> gen_int64_range(int64_t lo, int64_t hi) {
	return [lo, hi](RNG &rng, const Level &) -> int64_t { return rng.int64_range(lo, hi); };
}

inline Generator<double> gen_double_range(double lo, double hi) {
	return [lo, hi](RNG &rng, const Level &) -> double { return rng.float_range(lo, hi); };
}

// -------------------------------------------------------------------
// Container / composite generators.
// -------------------------------------------------------------------

template <class Elem>
inline Generator<std::vector<Elem>> gen_vector(Generator<Elem> elem_gen) {
	return [elem_gen](RNG &rng, const Level &lvl) -> std::vector<Elem> {
		uint32_t n = rng.uint_range(0, std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32)));
		std::vector<Elem> v;
		v.reserve(n);
		for (uint32_t i = 0; i < n; ++i) {
			v.push_back(elem_gen(rng, lvl));
		}
		return v;
	};
}

template <std::size_t N, class Elem>
inline Generator<std::array<Elem, N>> gen_array(Generator<Elem> elem_gen) {
	return [elem_gen](RNG &rng, const Level &lvl) -> std::array<Elem, N> {
		std::array<Elem, N> a{};
		for (std::size_t i = 0; i < N; ++i) {
			a[i] = elem_gen(rng, lvl);
		}
		return a;
	};
}

template <class Elem>
inline Generator<std::set<Elem>> gen_set(Generator<Elem> elem_gen) {
	return [elem_gen](RNG &rng, const Level &lvl) -> std::set<Elem> {
		uint32_t n = rng.uint_range(0, std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32)));
		std::set<Elem> s;
		for (uint32_t i = 0; i < n; ++i) {
			s.insert(elem_gen(rng, lvl));
		}
		return s;
	};
}

template <class Key, class Value>
inline Generator<std::map<Key, Value>> gen_map(Generator<Key> key_gen, Generator<Value> value_gen) {
	return [key_gen, value_gen](RNG &rng, const Level &lvl) -> std::map<Key, Value> {
		uint32_t n = rng.uint_range(0, std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32)));
		std::map<Key, Value> m;
		for (uint32_t i = 0; i < n; ++i) {
			Key k = key_gen(rng, lvl);
			Value v = value_gen(rng, lvl);
			m.emplace(std::move(k), std::move(v));
		}
		return m;
	};
}

template <class A, class B>
inline Generator<std::pair<A, B>> gen_pair(Generator<A> a_gen, Generator<B> b_gen) {
	return [a_gen, b_gen](RNG &rng, const Level &lvl) -> std::pair<A, B> {
		// Sequence the two draws so RNG consumption is stable: naming
		// them as intermediates forces evaluation order that constructor
		// arguments would leave unspecified.
		A a = a_gen(rng, lvl);
		B b = b_gen(rng, lvl);
		return std::pair<A, B>{ std::move(a), std::move(b) };
	};
}

template <class Elem>
inline Generator<std::optional<Elem>> gen_optional(Generator<Elem> elem_gen) {
	return [elem_gen](RNG &rng, const Level &lvl) -> std::optional<Elem> {
		if (rng.int_range(0, 3) == 0) {
			return std::nullopt;
		}
		return elem_gen(rng, lvl);
	};
}

// Variadic tuple. Brace initialization sequences the RNG draws left
// to right; std::make_tuple with function-call arguments would not.
template <class... Ts>
inline Generator<std::tuple<Ts...>> gen_tuple(Generator<Ts>... gens) {
	return [gens...](RNG &rng, const Level &lvl) -> std::tuple<Ts...> {
		return std::tuple<Ts...>{ gens(rng, lvl)... };
	};
}

// -------------------------------------------------------------------
// Combinators.
// -------------------------------------------------------------------

// Pick uniformly from a list of homogeneous generators.
template <class T>
inline Generator<T> gen_oneof(std::vector<Generator<T>> gens) {
	return [gens](RNG &rng, const Level &lvl) -> T {
		int i = rng.int_range(0, static_cast<int>(gens.size()) - 1);
		return gens[static_cast<std::size_t>(i)](rng, lvl);
	};
}

// Pick uniformly from a list of concrete values.
template <class T>
inline Generator<T> gen_element(std::vector<T> values) {
	return [values](RNG &rng, const Level &) -> T {
		int i = rng.int_range(0, static_cast<int>(values.size()) - 1);
		return values[static_cast<std::size_t>(i)];
	};
}

// Weighted choice — probability proportional to weight.
template <class T>
inline Generator<T> gen_frequency(std::vector<std::pair<int, Generator<T>>> weighted) {
	int total = 0;
	for (const std::pair<int, Generator<T>> &wg : weighted) {
		total += wg.first;
	}
	return [weighted, total](RNG &rng, const Level &lvl) -> T {
		int pick = rng.int_range(0, std::max(0, total - 1));
		int acc = 0;
		for (const std::pair<int, Generator<T>> &wg : weighted) {
			acc += wg.first;
			if (pick < acc) {
				return wg.second(rng, lvl);
			}
		}
		return weighted.back().second(rng, lvl);
	};
}

// Constant generator: always yields `v`.
template <class T>
inline Generator<T> gen_constant(T v) {
	return [v](RNG &, const Level &) -> T { return v; };
}

// String with a caller-supplied alphabet.
inline Generator<std::string> gen_string_of(std::string alphabet) {
	return [alphabet](RNG &rng, const Level &lvl) -> std::string {
		uint32_t len = rng.uint_range(0, std::max<uint32_t>(1, static_cast<uint32_t>(lvl.fin_bound / 32)));
		std::string s;
		s.reserve(len);
		if (alphabet.empty()) {
			return s;
		}
		for (uint32_t i = 0; i < len; ++i) {
			int idx = rng.int_range(0, static_cast<int>(alphabet.size()) - 1);
			s.push_back(alphabet[static_cast<std::size_t>(idx)]);
		}
		return s;
	};
}

// Map a function over a generator's output. The return type is the
// function's return type; caller names it explicitly.
template <class In, class Out>
inline Generator<Out> gen_transform(Generator<In> gen, std::function<Out(In)> fn) {
	return [gen, fn](RNG &rng, const Level &lvl) -> Out {
		return fn(gen(rng, lvl));
	};
}

// Filter: resample up to `retries` times until `pred` holds. On
// exhaustion the last draw is returned and the trial is marked SKIP.
template <class T>
inline Generator<T> gen_filter(Generator<T> gen, std::function<bool(const T &)> pred, int retries = 32) {
	return [gen, pred, retries](RNG &rng, const Level &lvl) -> T {
		T v = gen(rng, lvl);
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

// Dependent generation: draw from `gen`, then hand the value to `fn`
// which yields a second generator, then draw from that.
template <class In, class Out>
inline Generator<Out> gen_bind(Generator<In> gen, std::function<Generator<Out>(In)> fn) {
	return [gen, fn](RNG &rng, const Level &lvl) -> Out {
		In seed_value = gen(rng, lvl);
		Generator<Out> next_gen = fn(seed_value);
		return next_gen(rng, lvl);
	};
}

// -------------------------------------------------------------------
// Shrinkers.
// -------------------------------------------------------------------

inline std::vector<bool> shrink_bool(bool b) {
	if (b) {
		return { false };
	}
	return {};
}

inline std::vector<char> shrink_char(char c) {
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

inline std::vector<uint64_t> shrink_uint64(uint64_t n) {
	std::vector<uint64_t> out;
	if (n != 0) {
		out.push_back(0);
	}
	uint64_t halved = n / 2;
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

template <class T>
inline std::vector<std::set<T>> shrink_set(const std::set<T> &s) {
	std::vector<std::set<T>> out;
	if (s.empty()) {
		return out;
	}
	for (typename std::set<T>::const_iterator it = s.begin(); it != s.end(); ++it) {
		std::set<T> smaller = s;
		smaller.erase(*it);
		out.push_back(std::move(smaller));
	}
	return out;
}

template <class K, class V>
inline std::vector<std::map<K, V>> shrink_map(const std::map<K, V> &m) {
	std::vector<std::map<K, V>> out;
	if (m.empty()) {
		return out;
	}
	for (typename std::map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it) {
		std::map<K, V> smaller = m;
		smaller.erase(it->first);
		out.push_back(std::move(smaller));
	}
	return out;
}

template <class A, class B>
inline std::vector<std::pair<A, B>> shrink_pair(const std::pair<A, B> &p,
		Shrinker<A> a_shrink, Shrinker<B> b_shrink) {
	std::vector<std::pair<A, B>> out;
	std::vector<A> a_cands = a_shrink(p.first);
	for (std::size_t i = 0; i < a_cands.size(); ++i) {
		out.emplace_back(std::move(a_cands[i]), p.second);
	}
	std::vector<B> b_cands = b_shrink(p.second);
	for (std::size_t i = 0; i < b_cands.size(); ++i) {
		out.emplace_back(p.first, std::move(b_cands[i]));
	}
	return out;
}

template <class T>
inline std::vector<std::optional<T>> shrink_optional(const std::optional<T> &x, Shrinker<T> t_shrink) {
	std::vector<std::optional<T>> out;
	if (x.has_value()) {
		out.emplace_back(std::nullopt);
		std::vector<T> cands = t_shrink(*x);
		for (std::size_t i = 0; i < cands.size(); ++i) {
			out.emplace_back(std::move(cands[i]));
		}
	}
	return out;
}

namespace detail {

template <std::size_t I, class Tup, class ShrinkI>
inline void shrink_one_slot(const Tup &orig, ShrinkI &shrinker, std::vector<Tup> &out) {
	std::vector<std::tuple_element_t<I, Tup>> candidates = shrinker(std::get<I>(orig));
	for (std::size_t i = 0; i < candidates.size(); ++i) {
		Tup copy = orig;
		std::get<I>(copy) = std::move(candidates[i]);
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

template <class... Ts>
inline std::vector<std::tuple<Ts...>> shrink_tuple(const std::tuple<Ts...> &t, Shrinker<Ts>... shrinks) {
	std::tuple<Shrinker<Ts>...> pack{ shrinks... };
	return detail::shrink_tuple_impl(t, pack, std::index_sequence_for<Ts...>{});
}

// -------------------------------------------------------------------
// Shrinking driver. Non-throwing; iterates smaller candidates until
// none falsifies or the budget is spent.
// -------------------------------------------------------------------
template <class T>
inline T shrink_input(T input, std::function<bool(const T &)> predicate,
		Shrinker<T> shrinker, int budget = 128, int *r_iterations = nullptr) {
	int iterations = 0;
	for (int i = 0; i < budget; ++i) {
		std::vector<T> candidates = shrinker(input);
		bool improved = false;
		for (std::size_t k = 0; k < candidates.size(); ++k) {
			Ctx saved = ctx();
			ctx() = Ctx{};
			bool held = static_cast<bool>(predicate(candidates[k]));
			Verdict v = ctx().verdict;
			ctx() = saved;
			if (v == Verdict::SKIP) {
				continue;
			}
			if (!held) {
				input = std::move(candidates[k]);
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

// No-op shrinker: returns an empty candidate list for any input.
template <class T>
inline std::vector<T> no_shrink(const T &) {
	return {};
}

// -------------------------------------------------------------------
// Message formatting and seed selection.
// -------------------------------------------------------------------

namespace detail {

template <class T>
inline std::string format_found(const char *query, const Level &lvl, int trial_idx,
		const T &input, int shrink_iters,
		const std::function<void(std::ostream &, const T &)> &printer,
		uint64_t seed) {
	std::ostringstream o;
	o << query << " falsified at level " << lvl.idx
	  << " trial " << trial_idx
	  << " (walk_steps=" << lvl.walk_steps
	  << " fin_bound=" << lvl.fin_bound << ")";
	if (shrink_iters > 0) {
		o << "; shrunk " << shrink_iters << " step(s)";
	}
	std::ostringstream body;
	if (printer) {
		printer(body, input);
	}
	if (!body.str().empty()) {
		o << "; minimum=" << body.str();
	}
	o << "; re-run with property_seed=0x" << std::hex << seed << std::dec;
	return o.str();
}

inline std::string format_provably_none(const char *query, uint64_t seed) {
	std::ostringstream o;
	o << query << " held across the ladder";
	o << "; seed=0x" << std::hex << seed << std::dec;
	const std::map<std::string, int64_t> &hits = ctx().class_hits;
	int64_t total = ctx().trials_counted;
	if (!hits.empty() && total > 0) {
		o << "; classifications:";
		bool first = true;
		for (std::map<std::string, int64_t>::const_iterator it = hits.begin(); it != hits.end(); ++it) {
			if (!first) {
				o << ",";
			}
			first = false;
			int pct = static_cast<int>((100 * it->second + total / 2) / total);
			o << " " << it->first << " " << pct << "% (" << it->second << "/" << total << ")";
		}
	}
	return o.str();
}

} // namespace detail

inline uint64_t pick_seed(uint64_t caller_seed) {
	if (caller_seed != 0) {
		return caller_seed;
	}
	if (const char *env = std::getenv("PROPERTY_SEED")) {
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

template <class T>
inline Verdict run_predicate(const std::function<bool(const T &)> &predicate, const T &input) {
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

// -------------------------------------------------------------------
// Resolvers.
// -------------------------------------------------------------------

template <class T, class Ladder>
inline Trial resolve_with_ladder(const char *query, const Ladder &ladder,
		Generator<T> make_input,
		std::function<bool(const T &)> predicate,
		Shrinker<T> shrinker,
		std::function<void(std::ostream &, const T &)> printer,
		uint64_t seed = 0) {
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
			T input = make_input(rng, lvl);
			Verdict v = run_predicate<T>(predicate, input);
			if (v == Verdict::SKIP) {
				assumed_skipped++;
				continue;
			}
			if (v == Verdict::FALSIFY) {
				int shrink_iters = 0;
				T minimum = shrink_input<T>(std::move(input), predicate, shrinker,
						128, &shrink_iters);
				return { Outcome::FOUND, lvl.idx, trial_counter,
					detail::format_found<T>(query, lvl, trial_counter, minimum,
							shrink_iters, printer, actual_seed) };
			}
			ctx().trials_counted++;
			trial_counter++;
		}
	}
	return { Outcome::PROVABLY_NONE, last_idx, 0,
		detail::format_provably_none(query, actual_seed) };
}

template <class T>
inline Trial resolve(const char *query,
		Generator<T> make_input,
		std::function<bool(const T &)> predicate,
		Shrinker<T> shrinker,
		std::function<void(std::ostream &, const T &)> printer,
		uint64_t seed = 0) {
	return resolve_with_ladder<T>(query, DEFAULT_LADDER,
			std::move(make_input), std::move(predicate),
			std::move(shrinker), std::move(printer), seed);
}

// No-shrinker convenience: an empty Shrinker<T> and no printer.
template <class T>
inline Trial resolve(const char *query,
		Generator<T> make_input,
		std::function<bool(const T &)> predicate,
		uint64_t seed = 0) {
	Shrinker<T> empty_shrinker = &no_shrink<T>;
	std::function<void(std::ostream &, const T &)> empty_printer;
	return resolve<T>(query, std::move(make_input), std::move(predicate),
			std::move(empty_shrinker), std::move(empty_printer), seed);
}

} // namespace witness

#endif // WITNESS_LADDER_H
