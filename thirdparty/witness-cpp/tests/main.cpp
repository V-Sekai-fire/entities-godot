// SPDX-License-Identifier: MIT
// Provides doctest's main() for the witness-cpp test executable.
// Keeping it in its own TU avoids re-parsing the doctest impl in
// every test source file.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
