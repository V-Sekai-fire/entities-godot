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
#include "tests/test_tools.h"

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

TEST_CASE("[SceneTree][CapsuleShape3D] An untapered capsule accepts zero sizes and rejects negative ones") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	ErrorDetector ed;

	capsule->set_height(0.f);

	CHECK_FALSE(ed.has_error);
	CHECK(capsule->get_height() == doctest::Approx(0.f));
	CHECK(capsule->get_radius() == doctest::Approx(0.f));

	capsule->set_height(2.f);
	capsule->set_radius(0.f);

	CHECK_FALSE(ed.has_error);
	CHECK(capsule->get_height() == doctest::Approx(2.f));

	ERR_PRINT_OFF;
	capsule->set_radius(-1.f);
	capsule->set_height(-1.f);
	capsule->set_mid_height(-1.f);
	ERR_PRINT_ON;

	CHECK(ed.has_error);
	CHECK(capsule->get_radius() == doctest::Approx(0.f));
	CHECK(capsule->get_height() == doctest::Approx(2.f));
}

TEST_CASE("[SceneTree][CapsuleShape3D] A tapered capsule accepts a zero mid height but not a zero radius") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	capsule->set_tapered(true);
	capsule->set_bottom_radius(1.f);
	ErrorDetector ed;

	capsule->set_mid_height(0.f);
	capsule->set_height(1.5f);

	CHECK_FALSE(ed.has_error);
	CHECK(capsule->get_mid_height() == doctest::Approx(0.f));

	ERR_PRINT_OFF;
	capsule->set_top_radius(0.f);
	ERR_PRINT_ON;

	CHECK(ed.has_error);
	CHECK(capsule->get_top_radius() == doctest::Approx(0.5f));
}

#ifdef TOOLS_ENABLED
TEST_CASE("[SceneTree][CapsuleShape3D] Undoing an inspector edit restores the capsule") {
	Ref<CapsuleShape3D> capsule = memnew(CapsuleShape3D);
	UndoRedo *undo_redo = memnew(UndoRedo);

	SUBCASE("[SceneTree][CapsuleShape3D] A radius of a tapered capsule") {
		capsule->set_tapered(true);
		capsule->set_top_radius(0.3f);
		capsule->set_bottom_radius(0.9f);

		set_property_like_inspector(undo_redo, capsule.ptr(), "top_radius", 0.5f);
		undo_redo->undo();

		CHECK(capsule->get_top_radius() == doctest::Approx(0.3f));
		CHECK(capsule->get_bottom_radius() == doctest::Approx(0.9f));
	}

	SUBCASE("[SceneTree][CapsuleShape3D] A radius that raised the height") {
		set_property_like_inspector(undo_redo, capsule.ptr(), "radius", 1.5f);
		REQUIRE(capsule->get_height() == doctest::Approx(3.f));
		undo_redo->undo();

		CHECK(capsule->get_radius() == doctest::Approx(0.5f));
		CHECK(capsule->get_height() == doctest::Approx(2.f));
	}

	SUBCASE("[SceneTree][CapsuleShape3D] A height that shrank the radius") {
		set_property_like_inspector(undo_redo, capsule.ptr(), "height", 0.5f);
		REQUIRE(capsule->get_radius() == doctest::Approx(0.25f));
		undo_redo->undo();

		CHECK(capsule->get_radius() == doctest::Approx(0.5f));
		CHECK(capsule->get_height() == doctest::Approx(2.f));
	}

	memdelete(undo_redo);
}
#endif // TOOLS_ENABLED

} // namespace TestCapsuleShape3D

#endif // PHYSICS_3D_DISABLED
