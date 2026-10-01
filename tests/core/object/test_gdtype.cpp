/**************************************************************************/
/*  test_gdtype.cpp                                                       */
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

TEST_FORCE_LINK(test_gdtype)

#include "core/object/class_db.h"
#include "core/object/gdtype.h"
#include "tests/test_tools.h"

namespace TestGDType {

// Any method will do, only the name of the bind matters.
static MethodBind *create_test_method_bind(const StringName &p_name) {
	MethodBind *method = create_method_bind(&Object::notify_property_list_changed);
	method->set_name(p_name);
	return method;
}

TEST_CASE("[GDType] Binding a method under a name inherited from the super type fails and reports the name") {
	// Like the type of a GDExtension class: a mutable type deriving from `Object`, which binds `get_property_list`.
	GDType type(&Object::get_gdtype_static(), "GDTypeTestClass");
	type.initialize();
	REQUIRE(type.members().has("get_property_list"));

	ErrorDetector ed;
	ERR_PRINT_OFF;
	// Ownership is handed over, so the rejected bind is deleted.
	const bool bound = type.bind_method(create_test_method_bind("get_property_list"), true);
	ERR_PRINT_ON;

	CHECK_FALSE(bound);
	CHECK_FALSE(type.members(true).has("get_property_list"));
	REQUIRE(ed.has_error);
	CHECK_MESSAGE(ed.last_error_message.contains("get_property_list"),
			vformat("The error should name the rejected method, got: \"%s\"", ed.last_error_message));
}

TEST_CASE("[GDType] Binding a method under a name the type already bound fails and reports the name") {
	GDType type(&Object::get_gdtype_static(), "GDTypeTestClass");
	type.initialize();

	MethodBind *method = create_test_method_bind("test_method");
	REQUIRE(type.bind_method(method, true));

	ErrorDetector ed;
	ERR_PRINT_OFF;
	const bool bound = type.bind_method(create_test_method_bind("test_method"), true);
	ERR_PRINT_ON;

	CHECK_FALSE(bound);
	const GDType::Member *member = type.members(true).getptr("test_method");
	REQUIRE(member);
	CHECK_MESSAGE(member->payload.method == method, "The method bound first should be kept.");
	REQUIRE(ed.has_error);
	CHECK_MESSAGE(ed.last_error_message.contains("test_method"),
			vformat("The error should name the rejected method, got: \"%s\"", ed.last_error_message));
}

} // namespace TestGDType
