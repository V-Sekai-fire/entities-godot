/**************************************************************************/
/*  test_capsule_shape_3d.cpp                                             */
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

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_capsule_shape_3d)

#ifndef PHYSICS_3D_DISABLED

#include "scene/resources/3d/capsule_shape_3d.h"

namespace TestCapsuleShape3D {

TEST_CASE("[SceneTree][CapsuleShape3D] Setting the radius keeps the height until the radius passes half of it") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	capsule->set_height(7.1f);
	capsule->set_radius(1.3f);

	CHECK(capsule->get_radius() == doctest::Approx(1.3f));
	CHECK(capsule->get_height() == doctest::Approx(7.1f));

	capsule->set_radius(4.f);

	CHECK(capsule->get_height() == doctest::Approx(8.f));
	CHECK(capsule->get_mid_height() == doctest::Approx(0.f));
}

TEST_CASE("[SceneTree][CapsuleShape3D] Setting the height below twice the radius shrinks the radius") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	capsule->set_radius(1.f);
	capsule->set_height(0.5f);

	CHECK(capsule->get_radius() == doctest::Approx(0.25f));
	CHECK(capsule->get_height() == doctest::Approx(0.5f));
}

TEST_CASE("[SceneTree][CapsuleShape3D] An untapered capsule accepts a zero mid height") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	capsule->set_mid_height(0.f);

	CHECK(capsule->get_mid_height() == doctest::Approx(0.f));
	CHECK(capsule->get_height() == doctest::Approx(capsule->get_radius() * 2));
}

TEST_CASE("[SceneTree][CapsuleShape3D] Tapered capsules keep the mid height when a radius changes") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	capsule->set_tapered(true);
	capsule->set_bottom_radius(1.f);
	capsule->set_mid_height(2.f);

	CHECK(capsule->get_height() == doctest::Approx(3.5f));

	capsule->set_top_radius(0.8f);

	CHECK(capsule->get_mid_height() == doctest::Approx(2.f));
	CHECK(capsule->get_height() == doctest::Approx(3.8f));

	capsule->set_height(5.f);

	CHECK(capsule->get_mid_height() == doctest::Approx(3.2f));
	CHECK(capsule->get_top_radius() == doctest::Approx(0.8f));
	CHECK(capsule->get_bottom_radius() == doctest::Approx(1.f));
}

} // namespace TestCapsuleShape3D

#endif // PHYSICS_3D_DISABLED
