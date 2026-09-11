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
avoids "plausible" to sidestep any confusion with the Lean library.

## Why

- **No exceptions.** Godot, LLVM, and many embedded builds ship with
  `-fno-exceptions`. `witness-cpp` uses return values and a small
  trial-local context struct — no `throw`, no `try`, no `catch`. The
  CI proves this by building the test binary with `-fno-exceptions`
  `-fno-rtti`.
- **Doctest-friendly, not doctest-required.** The core
  `include/witness/ladder.h` has no test-framework dependency. If you
  want the `PROP_CHECK` macro that wraps a resolve in a doctest
  `SUBCASE`, include `witness/doctest.h`; if you use another framework,
  call `witness::resolve()` yourself.
- **Reproducible.** Every failing run prints the seed to re-run with.
  `PROPERTY_SEED=0x…` env var overrides the default random seed.

## Feature summary

- Iterative-deepening ladder with three rungs
  (`{walk_steps, fin_bound, num_inst}` = `{64,256,200}` →
  `{512,1024,800}` → `{4000,4096,2000}`).
- Deterministic seeded RNG (`std::mt19937_64`).
- Generator + Predicate contract.
- Shrinking: iterative, non-throwing, 128-step budget.
- Built-in generators: `gen_bool`, `gen_char`, `gen_int`, `gen_uint`,
  `gen_int64`, `gen_float`, `gen_double`, `gen_string`, `gen_vector<T>`,
  `gen_pair<A,B>`, `gen_optional<T>`, variadic `gen_tuple<...>`.
- Built-in combinators: `gen_oneof` (choose one of several generators),
  `gen_transform` (map a function over generator output), `gen_filter`
  (retry until a predicate holds, bounded).
- Built-in shrinkers: `shrink_bool`, `shrink_char`, `shrink_int`,
  `shrink_uint`, `shrink_int64`, `shrink_float`, `shrink_double`,
  `shrink_string`, `shrink_vector<T>`, `shrink_pair<A,B>`,
  `shrink_optional<T>`, variadic `shrink_tuple<...>`.
- `assume(cond)`: reject invalid inputs; rejected trials don't count
  toward `num_inst`.
- `classify(cond, label)`: distribution stats over holding trials,
  reported on `PROVABLY_NONE`.
- `resolve_with_ladder(...)`: run against a caller-supplied ladder
  instead of `DEFAULT_LADDER`.
- Doctest macros: `PROP_CHECK`, `PROP_CHECK_SHRINK`, `PROP_CHECK_SEED`.

## Usage

```cpp
#include "witness/doctest.h"

#include <algorithm>
#include <vector>

TEST_CASE("[witness] reverse . reverse is identity") {
    PROP_CHECK(
        "involution",
        [](witness::RNG &rng, const witness::Level &lvl) {
            std::vector<int> v(rng.uint_range(0, static_cast<uint32_t>(lvl.fin_bound / 32)));
            for (auto &x : v) x = rng.int_range(-100, 100);
            return v;
        },
        [](const std::vector<int> &v) {
            std::vector<int> r = v;
            std::reverse(r.begin(), r.end());
            std::reverse(r.begin(), r.end());
            return r == v;
        });
}
```

With shrinking:

```cpp
PROP_CHECK_SHRINK(
    "sorted output is monotonic",
    witness::gen_vector(witness::gen_int),
    [](std::vector<int> v) {
        std::sort(v.begin(), v.end());
        for (std::size_t i = 1; i < v.size(); ++i) if (v[i-1] > v[i]) return false;
        return true;
    },
    [](const std::vector<int> &v) { return witness::shrink_vector(v); },
    witness::AutoPrint{});
```

## Building the tests

```
cmake -B build -S .
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The test binary is built with `-fno-exceptions -fno-rtti` by default;
turn off `PLAUSIBLE_WITNESS_NO_EXCEPTIONS` to check that both configs
still pass. doctest is fetched at configure time via `FetchContent`.

## Integrating into another project

CMake:

```cmake
add_subdirectory(third_party/witness-cpp)
target_link_libraries(mytests PRIVATE witness::ladder)
```

Or `find_package(witness-cpp)` after `cmake --install`.

Header-only, so any build system that can add
`witness-cpp/include` to the include path also works.

## License

MIT. See `LICENSE`.
