/**************************************************************************/
/*  doctest.h                                                             */
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

#pragma once

// SPDX-License-Identifier: MIT
// doctest bindings for witness-cpp.

#ifndef WITNESS_DOCTEST_H
#define WITNESS_DOCTEST_H

#include "witness/ladder.h"

#include <doctest/doctest.h>

// Every macro requires an explicit type argument for T because the
// library is type-erased through std::function; the type cannot be
// deduced from a Generator<T> passed as an argument.

#define PROP_CHECK(m_T, m_query, m_make_input, m_predicate) \
	SUBCASE(m_query) { \
		::witness::Trial _prop_trial = ::witness::resolve<m_T>( \
				m_query, m_make_input, m_predicate); \
		INFO(_prop_trial.message); \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND); \
	}

#define PROP_CHECK_SHRINK(m_T, m_query, m_make_input, m_predicate, m_shrinker, m_printer) \
	SUBCASE(m_query) { \
		::witness::Trial _prop_trial = ::witness::resolve<m_T>( \
				m_query, m_make_input, m_predicate, m_shrinker, m_printer); \
		INFO(_prop_trial.message); \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND); \
	}

#define PROP_CHECK_SEED(m_T, m_query, m_make_input, m_predicate, m_seed) \
	SUBCASE(m_query) { \
		::witness::Shrinker<m_T> _empty_shrink = &::witness::no_shrink<m_T>; \
		std::function<void(std::ostream &, const m_T &)> _empty_print; \
		::witness::Trial _prop_trial = ::witness::resolve<m_T>( \
				m_query, m_make_input, m_predicate, \
				_empty_shrink, _empty_print, m_seed); \
		INFO(_prop_trial.message); \
		CHECK(_prop_trial.outcome != ::witness::Outcome::FOUND); \
	}

#endif // WITNESS_DOCTEST_H
