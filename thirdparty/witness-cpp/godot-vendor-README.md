# witness-cpp

- Upstream: https://github.com/V-Sekai-fire/witness-cpp
- Version: git subtree, prefix `thirdparty/witness-cpp/`, tracked
  against branch `main/main`.
- License: MIT (see `LICENSE`).

Header-only C++17 property-testing ladder. Godot's tests binary
includes `witness/ladder.h` directly from
`thirdparty/witness-cpp/include/` via the shim at
`tests/property/test_witness.cpp` and does not build any of the
subtree's own CMake/doctest test suite.

Refresh with:

```
git subtree pull --prefix=thirdparty/witness-cpp \
    https://github.com/V-Sekai-fire/witness-cpp.git main/main --squash
```
