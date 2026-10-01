/**************************************************************************/
/*  test_cylinder_shape_3d.cpp                                            */
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

TEST_FORCE_LINK(test_cylinder_shape_3d)

#ifndef PHYSICS_3D_DISABLED

#include "scene/resources/3d/cylinder_shape_3d.h"
#include "tests/test_tools.h"

namespace TestCylinderShape3D {

TEST_CASE("[SceneTree][CylinderShape3D] Radius and height are set independently") {
	Ref<CylinderShape3D> cylinder = memnew(CylinderShape3D);
	cylinder->set_height(0.5f);
	cylinder->set_radius(3.f);

	CHECK(cylinder->get_radius() == doctest::Approx(3.f));
	CHECK(cylinder->get_height() == doctest::Approx(0.5f));
}

TEST_CASE("[SceneTree][CylinderShape3D] Tapered cylinders keep the height when a radius changes") {
	Ref<CylinderShape3D> cylinder = memnew(CylinderShape3D);
	cylinder->set_tapered(true);
	cylinder->set_top_radius(0.2f);
	cylinder->set_bottom_radius(1.f);

	CHECK(cylinder->get_top_radius() == doctest::Approx(0.2f));
	CHECK(cylinder->get_bottom_radius() == doctest::Approx(1.f));
	CHECK(cylinder->get_height() == doctest::Approx(2.f));
}

TEST_CASE("[SceneTree][CylinderShape3D] Zero sizes are accepted, except the height of a tapered cylinder") {
	Ref<CylinderShape3D> cylinder = memnew(CylinderShape3D);
	ErrorDetector ed;

	cylinder->set_radius(0.f);
	cylinder->set_height(0.f);

	CHECK_FALSE(ed.has_error);
	CHECK(cylinder->get_radius() == doctest::Approx(0.f));
	CHECK(cylinder->get_height() == doctest::Approx(0.f));

	cylinder->set_height(2.f);
	cylinder->set_tapered(true);
	cylinder->set_bottom_radius(1.f);
	cylinder->set_top_radius(0.f);

	CHECK_FALSE(ed.has_error);
	CHECK(cylinder->get_top_radius() == doctest::Approx(0.f));

	ERR_PRINT_OFF;
	cylinder->set_height(0.f);
	cylinder->set_radius(-1.f);
	ERR_PRINT_ON;

	CHECK(ed.has_error);
	CHECK(cylinder->get_height() == doctest::Approx(2.f));
	CHECK(cylinder->get_bottom_radius() == doctest::Approx(1.f));
}

#ifdef TOOLS_ENABLED
TEST_CASE("[SceneTree][CylinderShape3D] Undoing an inspector edit of a radius keeps the taper") {
	Ref<CylinderShape3D> cylinder = memnew(CylinderShape3D);
	cylinder->set_tapered(true);
	cylinder->set_top_radius(0.3f);
	cylinder->set_bottom_radius(0.9f);
	UndoRedo *undo_redo = memnew(UndoRedo);

	set_property_like_inspector(undo_redo, cylinder.ptr(), "top_radius", 0.5f);
	undo_redo->undo();

	CHECK(cylinder->get_top_radius() == doctest::Approx(0.3f));
	CHECK(cylinder->get_bottom_radius() == doctest::Approx(0.9f));
	memdelete(undo_redo);
}
#endif // TOOLS_ENABLED

} // namespace TestCylinderShape3D

#endif // PHYSICS_3D_DISABLED
