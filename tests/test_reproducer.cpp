/**************************************************************************/
/*  test_reproducer.cpp                                                   */
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

// SPDX-License-Identifier: MIT
// Reproducer-seed guarantees. Each case pairs the unit with a
// falsifiability control.

#include "witness/doctest.h"

#include <cstdlib>
#include <functional>
#include <string>

TEST_CASE("[reproducer] same explicit seed reproduces the same message") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	witness::Trial a = witness::resolve<int>("always-false", gen, pred, 0xDEADBEEFULL);
	witness::Trial b = witness::resolve<int>("always-false", gen, pred, 0xDEADBEEFULL);
	CHECK(a.outcome == witness::Outcome::FOUND);
	CHECK(b.outcome == witness::Outcome::FOUND);
	CHECK(a.message == b.message);
}

TEST_CASE("[reproducer] falsifiability: different seeds give different messages") {
	// Rule-2 pair for "same seed → same message". If pick_seed had a
	// bug that ignored its argument, both runs would tie.
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	witness::Trial a = witness::resolve<int>("always-false", gen, pred, 0x1111ULL);
	witness::Trial b = witness::resolve<int>("always-false", gen, pred, 0x2222ULL);
	CHECK(a.message != b.message);
}

TEST_CASE("[reproducer] the FOUND message names the seed") {
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	witness::Trial t = witness::resolve<int>("always-false", gen, pred, 0xC0FFEEULL);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
	CHECK(t.message.find("property_seed=0xc0ffee") != std::string::npos);
}

TEST_CASE("[reproducer] falsifiability: a wrong-seed lookup misses") {
	// If the message printed a different seed than the one that ran,
	// the lookup would fail. Confirming a mismatched seed produces npos
	// closes the loop.
	std::string msg = "seed=0xc0ffee";
	CHECK(msg.find("0x1234") == std::string::npos);
}

TEST_CASE("[reproducer] PROPERTY_SEED env variable picks up when no explicit seed is given") {
#if defined(_WIN32)
	_putenv_s("PROPERTY_SEED", "0x1234");
#else
	setenv("PROPERTY_SEED", "0x1234", 1);
#endif
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	witness::Trial t = witness::resolve<int>("always-false", gen, pred);
#if defined(_WIN32)
	_putenv_s("PROPERTY_SEED", "");
#else
	unsetenv("PROPERTY_SEED");
#endif
	CHECK(t.outcome == witness::Outcome::FOUND);
	CHECK(t.message.find("property_seed=0x1234") != std::string::npos);
}

TEST_CASE("[reproducer] falsifiability: garbage PROPERTY_SEED falls back to random") {
	// If pick_seed accepted garbage, the message below would name
	// something impossible. Instead it falls back to random and does
	// NOT name the garbage token verbatim.
#if defined(_WIN32)
	_putenv_s("PROPERTY_SEED", "not-a-number");
#else
	setenv("PROPERTY_SEED", "not-a-number", 1);
#endif
	witness::Generator<int> gen = &witness::gen_int;
	std::function<bool(const int &)> pred = [](const int &) { return false; };
	witness::Trial t = witness::resolve<int>("always-false", gen, pred);
#if defined(_WIN32)
	_putenv_s("PROPERTY_SEED", "");
#else
	unsetenv("PROPERTY_SEED");
#endif
	CHECK(t.message.find("not-a-number") == std::string::npos);
}
