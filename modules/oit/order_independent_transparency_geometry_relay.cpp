/**************************************************************************/
/*  order_independent_transparency_geometry_relay.cpp                     */
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

#include "order_independent_transparency_geometry_relay.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"

OrderIndependentTransparencyGeometryRelay::OrderIndependentTransparencyGeometryRelay() {
	set_process(true);
}

OrderIndependentTransparencyGeometryRelay::~OrderIndependentTransparencyGeometryRelay() {
}

void OrderIndependentTransparencyGeometryRelay::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			if (!auto_repack) {
				break;
			}
			int child_count = get_child_count();
			if (child_count != last_child_count) {
				last_child_count = child_count;
				repack_now();
			}
		} break;
		default:
			break;
	}
}

void OrderIndependentTransparencyGeometryRelay::repack_now() {
	Array batches;
	emit_signal(SNAME("transparency_batches_ready"), batches);
}

void OrderIndependentTransparencyGeometryRelay::set_compositor_effect_path(const NodePath &p_path) {
	compositor_effect_path = p_path;
}

NodePath OrderIndependentTransparencyGeometryRelay::get_compositor_effect_path() const {
	return compositor_effect_path;
}

void OrderIndependentTransparencyGeometryRelay::set_auto_repack(bool p_enabled) {
	auto_repack = p_enabled;
}

bool OrderIndependentTransparencyGeometryRelay::get_auto_repack() const {
	return auto_repack;
}

void OrderIndependentTransparencyGeometryRelay::set_mesh_paths(const PackedStringArray &p_paths) {
	mesh_paths = p_paths;
}

PackedStringArray OrderIndependentTransparencyGeometryRelay::get_mesh_paths() const {
	return mesh_paths;
}

void OrderIndependentTransparencyGeometryRelay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_compositor_effect_path", "path"), &OrderIndependentTransparencyGeometryRelay::set_compositor_effect_path);
	ClassDB::bind_method(D_METHOD("get_compositor_effect_path"), &OrderIndependentTransparencyGeometryRelay::get_compositor_effect_path);

	ClassDB::bind_method(D_METHOD("set_auto_repack", "enabled"), &OrderIndependentTransparencyGeometryRelay::set_auto_repack);
	ClassDB::bind_method(D_METHOD("get_auto_repack"), &OrderIndependentTransparencyGeometryRelay::get_auto_repack);

	ClassDB::bind_method(D_METHOD("set_mesh_paths", "paths"), &OrderIndependentTransparencyGeometryRelay::set_mesh_paths);
	ClassDB::bind_method(D_METHOD("get_mesh_paths"), &OrderIndependentTransparencyGeometryRelay::get_mesh_paths);

	ClassDB::bind_method(D_METHOD("repack_now"), &OrderIndependentTransparencyGeometryRelay::repack_now);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "compositor_effect_path"), "set_compositor_effect_path", "get_compositor_effect_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_repack"), "set_auto_repack", "get_auto_repack");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "mesh_paths"), "set_mesh_paths", "get_mesh_paths");

	ADD_SIGNAL(MethodInfo("transparency_batches_ready",
			PropertyInfo(Variant::ARRAY, "batches")));
}
