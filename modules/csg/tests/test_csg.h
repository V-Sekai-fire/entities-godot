/**************************************************************************/
/*  test_csg.h                                                            */
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

#include "../csg_shape.h"

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "tests/test_macros.h"

namespace TestCSG {

TEST_CASE("[SceneTree][CSG] CSGPolygon3D") {
	SUBCASE("[SceneTree][CSG] CSGPolygon3D: using accurate path tangent for polygon rotation") {
		const float polygon_radius = 10.0f;

		const Vector3 expected_min_bounds = Vector3(-polygon_radius, -polygon_radius, 0);
		const Vector3 expected_max_bounds = Vector3(100 + polygon_radius, polygon_radius, 100);
		const AABB expected_aabb = AABB(expected_min_bounds, expected_max_bounds - expected_min_bounds);

		Ref<Curve3D> curve;
		curve.instantiate();
		curve->add_point(
				// p_position
				Vector3(0, 0, 0),
				// p_in
				Vector3(),
				// p_out
				Vector3(0, 0, 60));
		curve->add_point(
				// p_position
				Vector3(100, 0, 100),
				// p_in
				Vector3(0, 0, -60),
				// p_out
				Vector3());

		Path3D *path = memnew(Path3D);
		path->set_curve(curve);

		CSGPolygon3D *csg_polygon_3d = memnew(CSGPolygon3D);
		SceneTree::get_singleton()->get_root()->add_child(csg_polygon_3d);

		csg_polygon_3d->add_child(path);
		csg_polygon_3d->set_path_node(csg_polygon_3d->get_path_to(path));
		csg_polygon_3d->set_mode(CSGPolygon3D::Mode::MODE_PATH);

		PackedVector2Array polygon;
		polygon.append(Vector2(-polygon_radius, 0));
		polygon.append(Vector2(0, polygon_radius));
		polygon.append(Vector2(polygon_radius, 0));
		polygon.append(Vector2(0, -polygon_radius));
		csg_polygon_3d->set_polygon(polygon);

		csg_polygon_3d->set_path_rotation(CSGPolygon3D::PathRotation::PATH_ROTATION_PATH);
		csg_polygon_3d->set_path_rotation_accurate(true);

		// Minimize the number of extrusions.
		// This decreases the number of samples taken from the curve.
		// Having fewer samples increases the inaccuracy of the line between samples as an approximation of the tangent of the curve.
		// With correct polygon orientation, the bounding box for the given curve should be independent of the number of extrusions.
		csg_polygon_3d->set_path_interval_type(CSGPolygon3D::PathIntervalType::PATH_INTERVAL_DISTANCE);
		csg_polygon_3d->set_path_interval(1000.0f);

		// Call get_brush_faces to force the bounding box to update.
		csg_polygon_3d->get_brush_faces();

		CHECK(csg_polygon_3d->get_aabb().is_equal_approx(expected_aabb));

		// Perform the bounding box check again with a greater number of extrusions.
		csg_polygon_3d->set_path_interval(1.0f);
		csg_polygon_3d->get_brush_faces();

		CHECK(csg_polygon_3d->get_aabb().is_equal_approx(expected_aabb));

		csg_polygon_3d->remove_child(path);
		SceneTree::get_singleton()->get_root()->remove_child(csg_polygon_3d);

		memdelete(csg_polygon_3d);
		memdelete(path);
	}
}

// Two overlapping boxes, each skinned to its own bone, with its own UV2 and colour. After a union the surface is cut
// along the seam, and every vertex of it must still say which box it came from.
static Ref<ArrayMesh> _make_skinned_box(const Vector3 &p_center, float p_half, int p_bone) {
	PackedVector3Array vertices;
	for (int i = 0; i < 8; i++) {
		vertices.push_back(p_center + Vector3((i & 1) ? p_half : -p_half, (i & 2) ? p_half : -p_half, (i & 4) ? p_half : -p_half));
	}
	// Twelve triangles, wound outward.
	const int faces[36] = {
		0, 2, 3, 0, 3, 1, // -z
		4, 5, 7, 4, 7, 6, // +z
		0, 4, 6, 0, 6, 2, // -x
		1, 3, 7, 1, 7, 5, // +x
		0, 1, 5, 0, 5, 4, // -y
		2, 6, 7, 2, 7, 3, // +y
	};
	PackedInt32Array indices;
	for (int i = 0; i < 36; i++) {
		indices.push_back(faces[i]);
	}
	PackedVector2Array uvs, uv2s;
	PackedColorArray colors;
	PackedInt32Array bones;
	PackedFloat32Array weights;
	for (int i = 0; i < 8; i++) {
		uvs.push_back(Vector2(i & 1, (i >> 1) & 1));
		uv2s.push_back(Vector2(p_bone, 0.5));
		colors.push_back(Color(p_bone / 10.0f, 0.25, 0.75, 1.0)); // colours are stored 8-bit unorm, so keep them in 0..1
		for (int k = 0; k < 4; k++) {
			bones.push_back(k == 0 ? p_bone : 0);
			weights.push_back(k == 0 ? 1.0f : 0.0f);
		}
	}
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_TEX_UV2] = uv2s;
	arrays[Mesh::ARRAY_COLOR] = colors;
	arrays[Mesh::ARRAY_BONES] = bones;
	arrays[Mesh::ARRAY_WEIGHTS] = weights;
	arrays[Mesh::ARRAY_INDEX] = indices;
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return mesh;
}

TEST_CASE("[SceneTree][CSG] CSGMesh3D carries UV2, colour and skin weights through a union") {
	const Vector3 center_a = Vector3(0, 0, 0);
	const Vector3 center_b = Vector3(0.5, 0.3, 0.2);
	const float half = 0.5f;

	CSGCombiner3D *root = memnew(CSGCombiner3D);
	CSGMesh3D *box_a = memnew(CSGMesh3D);
	CSGMesh3D *box_b = memnew(CSGMesh3D);
	box_a->set_mesh(_make_skinned_box(center_a, half, 3));
	box_b->set_mesh(_make_skinned_box(center_b, half, 7));
	root->add_child(box_a);
	root->add_child(box_b);
	SceneTree::get_singleton()->get_root()->add_child(root);

	root->call("_update_shape");
	Ref<ArrayMesh> result = root->bake_static_mesh();
	REQUIRE(result.is_valid());
	REQUIRE(result->get_surface_count() >= 1);

	int checked_a = 0;
	int checked_b = 0;
	for (int s = 0; s < result->get_surface_count(); s++) {
		Array arrays = result->surface_get_arrays(s);
		PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
		PackedInt32Array bones = arrays[Mesh::ARRAY_BONES];
		PackedFloat32Array weights = arrays[Mesh::ARRAY_WEIGHTS];
		PackedVector2Array uv2s = arrays[Mesh::ARRAY_TEX_UV2];
		PackedColorArray colors = arrays[Mesh::ARRAY_COLOR];
		REQUIRE(bones.size() == vertices.size() * 4);
		REQUIRE(weights.size() == vertices.size() * 4);
		REQUIRE(uv2s.size() == vertices.size());
		REQUIRE(colors.size() == vertices.size());

		for (int v = 0; v < vertices.size(); v++) {
			float sum = 0.0f;
			for (int k = 0; k < 4; k++) {
				sum += weights[v * 4 + k];
			}
			CHECK(Math::is_equal_approx(sum, 1.0f));

			// Which box's surface is this vertex on? A vertex on both (the seam) is skipped.
			const Vector3 pa = vertices[v] - center_a;
			const Vector3 pb = vertices[v] - center_b;
			const bool on_a = Math::is_equal_approx(MAX(MAX(Math::abs(pa.x), Math::abs(pa.y)), Math::abs(pa.z)), (real_t)half, (real_t)1e-4);
			const bool on_b = Math::is_equal_approx(MAX(MAX(Math::abs(pb.x), Math::abs(pb.y)), Math::abs(pb.z)), (real_t)half, (real_t)1e-4);
			if (on_a == on_b) {
				continue;
			}
			const int want = on_a ? 3 : 7;
			CAPTURE(vertices[v]);
			CAPTURE(on_a);
			CAPTURE(bones[v * 4 + 1]);
			CAPTURE(weights[v * 4 + 1]);
			CHECK(bones[v * 4] == want);
			CHECK(weights[v * 4] > 0.99f);
			CHECK(Math::is_equal_approx(uv2s[v].x, (real_t)want, (real_t)1e-4));
			CHECK(Math::is_equal_approx(colors[v].r, want / 10.0f, 0.01f));
			if (on_a) {
				checked_a++;
			} else {
				checked_b++;
			}
		}
	}
	// Both boxes keep surface the other does not cover, so each must have been checked on many vertices.
	CHECK(checked_a > 8);
	CHECK(checked_b > 8);

	SceneTree::get_singleton()->get_root()->remove_child(root);
	memdelete(root);
}

} // namespace TestCSG
