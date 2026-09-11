# witness-cpp

Exception-free property-testing ladder for C++17. Header-only.
doctest-compatible.

Modeled on two upstream shapes:

- [`fire/plausible-witness-dag`](https://github.com/fire/plausible-witness-dag)
  — Lean/Lake library that layers an iterative-deepening ladder and
  deterministic readback over Plausible. The `Level` struct and the
  three-rung default ladder here mirror its Lean-side defaults verbatim.
- [`leanprover-community/plausible`](https://github.com/leanprover-community/plausible)
  — the underlying Lean property-testing library. `num_inst` semantics
  and the `FOUND / PROVABLY_NONE / BUDGET_HIT` outcome triple come
  from there.

`witness-cpp` is a fresh C++ implementation; it is not derived from,
affiliated with, or endorsed by either upstream. The name deliberately
avoids "plausible" to sidestep confusion with the Lean library.

## Why

- **No exceptions.** Godot, LLVM, and many embedded builds ship with
  `-fno-exceptions`. `witness-cpp` uses return values and a small
  trial-local context struct — no `throw`, no `try`, no `catch`. CI
  proves this by building the test binary with `-fno-exceptions`
  `-fno-rtti` across Linux, macOS, and (with exceptions on) Windows.
- **No `auto` in variable declarations or function return types.** The
  primitive generators are function pointers; every combinator returns
  a nameable `witness::Generator<T>` (a `std::function` alias) or a
  nameable `witness::Shrinker<T>`. The reader always sees the type
  being held.
- **Doctest-friendly, not doctest-required.** `witness/ladder.h` has
  no test-framework dependency. Include `witness/doctest.h` for the
  `PROP_CHECK` macros; use another framework by calling
  `witness::resolve<T>()` directly.
- **Reproducible.** Every failing run prints the seed to re-run with.
  `PROPERTY_SEED=0x…` env var overrides the default random seed.

## Feature surface

**Primitive generators** (function pointers): `gen_bool`, `gen_char`,
`gen_int`, `gen_uint`, `gen_int64`, `gen_uint64`, `gen_size`,
`gen_float`, `gen_double`, `gen_string`.

**Ranged variants** (`Generator<T>`-returning): `gen_int_range`,
`gen_uint_range`, `gen_int64_range`, `gen_double_range`,
`gen_string_of` (charset-controlled).

**Container generators**: `gen_vector<T>`, `gen_array<N, T>` (fixed
size), `gen_set<T>`, `gen_map<K, V>`, `gen_pair<A, B>`,
`gen_optional<T>`, variadic `gen_tuple<Ts...>`.

**Combinators**: `gen_oneof<T>` (uniform choice among a homogeneous
list), `gen_element<T>` (choose from a concrete list of values),
`gen_frequency<T>` (weighted choice), `gen_constant<T>`,
`gen_transform<In, Out>` (map a function), `gen_filter<T>` (retry
until a predicate holds, then SKIP the trial), `gen_bind<In, Out>`
(dependent generation, the flatmap analogue).

**Shrinkers**: `shrink_bool`, `shrink_char`, `shrink_int`,
`shrink_uint`, `shrink_int64`, `shrink_uint64`, `shrink_float`,
`shrink_double`, `shrink_string`, `shrink_vector<T>`,
`shrink_set<T>`, `shrink_map<K, V>`, `shrink_pair<A, B>`,
`shrink_optional<T>`, variadic `shrink_tuple<Ts...>`, `no_shrink<T>`.

**Runtime**: `assume(cond)` (reject invalid inputs; skipped trials
don't count toward `num_inst`), `classify(cond, label)` (boolean
distribution stats), `collect(value)` (value-tagged distribution
stats). Both distributions are reported on `PROVABLY_NONE` in the
Trial message.

**Resolvers**: `resolve<T>(query, gen, pred, [shrinker, printer,]
seed=0)` walks `DEFAULT_LADDER`; `resolve_with_ladder<T>(query, ladder,
gen, pred, shrinker, printer, seed=0)` walks a caller-supplied ladder
(any range of `Level` values — array, vector, span-shaped).

**Doctest macros**: `PROP_CHECK(T, query, gen, pred)`,
`PROP_CHECK_SHRINK(T, query, gen, pred, shrinker, printer)`,
`PROP_CHECK_SEED(T, query, gen, pred, seed)`. Every macro takes the
element type `T` explicitly — with type erasure through
`std::function`, template deduction cannot recover it from the
argument.

## Usage

```cpp
#include "witness/doctest.h"

#include <algorithm>
#include <functional>
#include <vector>

static std::vector<int> make_ints(witness::RNG &rng, const witness::Level &lvl) {
    uint32_t n = rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32));
    std::vector<int> v(n);
    for (uint32_t i = 0; i < n; ++i) v[i] = rng.int_range(-100, 100);
    return v;
}

TEST_CASE("[witness] reverse . reverse is identity") {
    witness::Generator<std::vector<int>> gen = &make_ints;
    std::function<bool(const std::vector<int> &)> pred = [](const std::vector<int> &v) {
        std::vector<int> r = v;
        std::reverse(r.begin(), r.end());
        std::reverse(r.begin(), r.end());
        return r == v;
    };
    witness::Trial t = witness::resolve<std::vector<int>>("involution", gen, pred);
    CHECK(t.outcome != witness::Outcome::FOUND);
}
```

With shrinking:

```cpp
witness::Shrinker<std::vector<int>> sh = &witness::shrink_vector<int>;
std::function<void(std::ostream &, const std::vector<int> &)> printer =
    [](std::ostream &os, const std::vector<int> &v) { os << "size=" << v.size(); };
witness::Trial t = witness::resolve<std::vector<int>>(
    "sort monotonic", gen, pred, sh, printer);
```

## Building the tests

```
cmake -B build -S .
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The test binary is built with `-fno-exceptions -fno-rtti` by default;
turn off `WITNESS_NO_EXCEPTIONS` to check that the exceptions-on
config also passes. doctest is fetched at configure time via
`FetchContent`.

## Integrating into another project

CMake:

```cmake
add_subdirectory(third_party/witness-cpp)
target_link_libraries(mytests PRIVATE witness::ladder)
```

Or `find_package(witness-cpp)` after `cmake --install`. Header-only,
so any build system that can add `witness-cpp/include` to the include
path also works.

## Test suite

80 test cases, 165 assertions, all passing under `-fno-exceptions`
`-fno-rtti`. Every property-test and every unit test paired with a
falsifiability control asserting the broken input fails
(CLAUDE.md rule 2).

## License

MIT. See `LICENSE` and `CITATION.cff`.
