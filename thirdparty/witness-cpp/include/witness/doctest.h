// SPDX-License-Identifier: MIT
// doctest bindings for witness-cpp.

#ifndef WITNESS_DOCTEST_H
#define WITNESS_DOCTEST_H

#include "witness/ladder.h"

#include <doctest/doctest.h>

// Every macro requires an explicit type argument for T because the
// library is type-erased through std::function; the type cannot be
// deduced from a Generator<T> passed as an argument.

#define PROP_CHECK(m_T, m_query, m_make_input, m_predicate)                          \
	SUBCASE(m_query) {                                                               \
		::witness::Trial _prop_trial = ::witness::resolve<m_T>(                      \
				m_query, m_make_input, m_predicate);                                 \
		INFO(_prop_trial.message);                                                   \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND);                     \
	}

#define PROP_CHECK_SHRINK(m_T, m_query, m_make_input, m_predicate, m_shrinker, m_printer) \
	SUBCASE(m_query) {                                                                    \
		::witness::Trial _prop_trial = ::witness::resolve<m_T>(                           \
				m_query, m_make_input, m_predicate, m_shrinker, m_printer);               \
		INFO(_prop_trial.message);                                                        \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND);                          \
	}

#define PROP_CHECK_SEED(m_T, m_query, m_make_input, m_predicate, m_seed)                  \
	SUBCASE(m_query) {                                                                    \
		::witness::Shrinker<m_T> _empty_shrink = &::witness::no_shrink<m_T>;              \
		std::function<void(std::ostream &, const m_T &)> _empty_print;                    \
		::witness::Trial _prop_trial = ::witness::resolve<m_T>(                           \
				m_query, m_make_input, m_predicate,                                       \
				_empty_shrink, _empty_print, m_seed);                                     \
		INFO(_prop_trial.message);                                                        \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND);                          \
	}

#endif // WITNESS_DOCTEST_H
