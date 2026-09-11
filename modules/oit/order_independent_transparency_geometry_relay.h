/**************************************************************************/
/*  order_independent_transparency_geometry_relay.h                       */
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

#include "scene/3d/node_3d.h"

class OrderIndependentTransparencyGeometryRelay : public Node3D {
	GDCLASS(OrderIndependentTransparencyGeometryRelay, Node3D);

	NodePath compositor_effect_path;
	bool auto_repack = true;
	PackedStringArray mesh_paths;

	int last_child_count = 0;
	RID vertex_buffer_rid;
	RID instance_buffer_rid;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_compositor_effect_path(const NodePath &p_path);
	NodePath get_compositor_effect_path() const;

	void set_auto_repack(bool p_enabled);
	bool get_auto_repack() const;

	void set_mesh_paths(const PackedStringArray &p_paths);
	PackedStringArray get_mesh_paths() const;

	void repack_now();

	OrderIndependentTransparencyGeometryRelay();
	~OrderIndependentTransparencyGeometryRelay();
};
