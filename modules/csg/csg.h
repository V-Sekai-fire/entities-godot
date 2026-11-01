/**************************************************************************/
/*  csg.h                                                                 */
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

#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "scene/resources/material.h"

struct CSGBrush {
	struct Face {
		Vector3 vertices[3];
		Vector2 uvs[3];
		Vector2 uv2s[3];
		Color colors[3];
		AABB aabb;
		bool smooth = false;
		bool invert = false;
		int material = 0;
	};

	Vector<Face> faces;
	Vector<Ref<Material>> materials;

	// Optional per-corner attributes that survive a boolean. A cut makes new vertices, and Manifold fills their
	// properties by interpolating the corners of the triangle it cut, so each attribute has to be one that a
	// weighted mean keeps meaningful. UV2 and colour are. A bone index is not, so skin weights are carried as one
	// dense weight per bone in use (a channel), and the (index, weight) pairs of a vertex are rebuilt at the end.
	static constexpr int MAX_SKIN_CHANNELS = 64;
	bool has_uv2 = false;
	bool has_colors = false;
	Vector<int> skin_bones; // channel -> bone index of the source mesh
	Vector<float> skin_weights; // faces.size() * 3 * skin_bones.size(), face-major, corner, then channel

	inline void _regen_face_aabbs() {
		for (int i = 0; i < faces.size(); i++) {
			faces.write[i].aabb = AABB();
			faces.write[i].aabb.position = faces[i].vertices[0];
			faces.write[i].aabb.expand_to(faces[i].vertices[1]);
			faces.write[i].aabb.expand_to(faces[i].vertices[2]);
		}
	}

	// Create a brush from faces.
	void build_from_faces(const Vector<Vector3> &p_vertices, const Vector<Vector2> &p_uvs, const Vector<bool> &p_smooth, const Vector<Ref<Material>> &p_materials, const Vector<bool> &p_invert_faces);
	void copy_from(const CSGBrush &p_brush, const Transform3D &p_xform);

	// Attach per-corner attributes to a brush made by build_from_faces. Every array is unrolled the same way as
	// p_vertices there (3 entries per face), or empty. p_bones and p_weights hold p_influences entries per corner.
	void set_attributes(const Vector<Vector2> &p_uv2s, const Vector<Color> &p_colors, const Vector<int> &p_bones, const Vector<float> &p_weights, int p_influences);
};
