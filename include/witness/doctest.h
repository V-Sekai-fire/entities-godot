// SPDX-License-Identifier: MIT
// doctest bindings for witness-cpp.
//
// Optional: only include this header when you want the PROP_CHECK
// macro that runs a resolve() inside a doctest SUBCASE. The core
// witness.h has no doctest dependency, so downstream projects that
// use a different test framework can wrap resolve() themselves.

#ifndef WITNESS_DOCTEST_H
#define WITNESS_DOCTEST_H

#include "witness/ladder.h"

#include <doctest/doctest.h>

// Run a resolve inside a doctest SUBCASE. INFO carries the terminating
// message; CHECK asserts the outcome is not FOUND.
#define PROP_CHECK(m_query, m_make_input, m_predicate)                          \
	SUBCASE(m_query) {                                                          \
		::witness::Trial _prop_trial =                                        \
				::witness::resolve(m_query, m_make_input, m_predicate);       \
		INFO(_prop_trial.message);                                              \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND);              \
	}

// Same, with a caller-supplied shrinker and printer. On FOUND the
// printer emits the minimum counterexample into Trial::message and
// the doctest INFO log.
#define PROP_CHECK_SHRINK(m_query, m_make_input, m_predicate, m_shrinker, m_printer) \
	SUBCASE(m_query) {                                                               \
		::witness::Trial _prop_trial = ::witness::resolve(                       \
				m_query, m_make_input, m_predicate, m_shrinker, m_printer);          \
		INFO(_prop_trial.message);                                                   \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND);                   \
	}

#endif // WITNESS_DOCTEST_H
