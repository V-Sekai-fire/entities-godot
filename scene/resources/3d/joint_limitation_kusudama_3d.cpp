/**************************************************************************/
/*  joint_limitation_kusudama_3d.cpp                                      */
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

#include "joint_limitation_kusudama_3d.h"

#include "core/object/class_db.h"

#ifdef TOOLS_ENABLED
#include "scene/resources/3d/kusudama_gizmo_shader.h"
#include "scene/resources/material.h"
#include "scene/resources/surface_tool.h"
#endif

void JointLimitationKusudama3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cones", "cones"), &JointLimitationKusudama3D::set_cones);
	ClassDB::bind_method(D_METHOD("get_cones"), &JointLimitationKusudama3D::get_cones);

	ClassDB::bind_method(D_METHOD("set_cone_count", "count"), &JointLimitationKusudama3D::set_cone_count);
	ClassDB::bind_method(D_METHOD("get_cone_count"), &JointLimitationKusudama3D::get_cone_count);
	ClassDB::bind_method(D_METHOD("set_cone_center", "index", "center"), &JointLimitationKusudama3D::set_cone_center);
	ClassDB::bind_method(D_METHOD("get_cone_center", "index"), &JointLimitationKusudama3D::get_cone_center);
	ClassDB::bind_method(D_METHOD("set_cone_radius", "index", "radius"), &JointLimitationKusudama3D::set_cone_radius);
	ClassDB::bind_method(D_METHOD("get_cone_radius", "index"), &JointLimitationKusudama3D::get_cone_radius);

	ClassDB::bind_method(D_METHOD("set_twist_from", "radians"), &JointLimitationKusudama3D::set_twist_from);
	ClassDB::bind_method(D_METHOD("get_twist_from"), &JointLimitationKusudama3D::get_twist_from);
	ClassDB::bind_method(D_METHOD("set_twist_to", "radians"), &JointLimitationKusudama3D::set_twist_to);
	ClassDB::bind_method(D_METHOD("get_twist_to"), &JointLimitationKusudama3D::get_twist_to);
	ClassDB::bind_method(D_METHOD("twist_angle_continuous", "rotation", "twist_axis", "previous_angle"), &JointLimitationKusudama3D::twist_angle_continuous);
	ClassDB::bind_method(D_METHOD("clamp_twist", "angle"), &JointLimitationKusudama3D::clamp_twist);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "cones", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_cones", "get_cones");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "twist_from", PROPERTY_HINT_RANGE, "-6.283185,6.283185,0.001,radians_as_degrees"), "set_twist_from", "get_twist_from");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "twist_to", PROPERTY_HINT_RANGE, "-6.283185,6.283185,0.001,radians_as_degrees"), "set_twist_to", "get_twist_to");
}

void JointLimitationKusudama3D::set_cones(const Vector<Vector4> &p_cones) {
	cones.clear();
	int n = MIN((int)p_cones.size(), MAX_KUSUDAMA_CONES);
	for (int i = 0; i < n; i++) {
		cones.push_back(p_cones[i]);
	}
	_invalidate_normalized_cache();
	emit_changed();
}

Vector<Vector4> JointLimitationKusudama3D::get_cones() const {
	Vector<Vector4> r;
	r.resize(cones.size());
	for (uint32_t i = 0; i < cones.size(); i++) {
		r.write[i] = cones[i];
	}
	return r;
}

void JointLimitationKusudama3D::set_cone_count(int p_count) {
	if (p_count < 0) {
		p_count = 0;
	}
	if (p_count > MAX_KUSUDAMA_CONES) {
		p_count = MAX_KUSUDAMA_CONES;
	}
	uint32_t old_size = cones.size();
	if (old_size == (uint32_t)p_count) {
		return;
	}
	cones.resize(p_count);
	// Initialize new cones with default values (+Y axis, 45 degree cone)
	for (uint32_t i = old_size; i < cones.size(); i++) {
		cones[i] = Vector4(0, 1, 0, Math::PI * 0.25);
	}
	_invalidate_normalized_cache();
	notify_property_list_changed();
	emit_changed();
}

int JointLimitationKusudama3D::get_cone_count() const {
	return cones.size();
}

void JointLimitationKusudama3D::set_cone_center(int p_index, const Vector3 &p_center) {
	ERR_FAIL_INDEX(p_index, (int)cones.size());
	// Store raw for serialization and direct input (like gravity_direction in SpringBoneSimulator).
	// Normalization is applied internally via cached values.
	Vector4 &cone = cones[p_index];
	cone.x = p_center.x;
	cone.y = p_center.y;
	cone.z = p_center.z;
	_invalidate_normalized_cache();
	emit_changed();
}

Vector3 JointLimitationKusudama3D::get_cone_center(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)cones.size(), Vector3::UP);
	const Vector4 &cone_data = cones[p_index];
	return Vector3(cone_data.x, cone_data.y, cone_data.z);
}

void JointLimitationKusudama3D::_invalidate_normalized_cache() const {
	_normalized_cone_centers_cache.clear();
}

Vector3 JointLimitationKusudama3D::_get_cone_center_normalized(int p_index) const {
	if (_normalized_cone_centers_cache.size() != cones.size()) {
		_normalized_cone_centers_cache.resize(cones.size());
		for (uint32_t i = 0; i < cones.size(); i++) {
			Vector3 raw(cones[i].x, cones[i].y, cones[i].z);
			if (raw.is_zero_approx()) {
				_normalized_cone_centers_cache[i] = Vector3::UP;
			} else {
				_normalized_cone_centers_cache[i] = raw.normalized();
			}
		}
	}
	ERR_FAIL_INDEX_V(p_index, (int)_normalized_cone_centers_cache.size(), Vector3::UP);
	return _normalized_cone_centers_cache[p_index];
}

void JointLimitationKusudama3D::set_cone_radius(int p_index, real_t p_radius) {
	ERR_FAIL_INDEX(p_index, (int)cones.size());
	cones[p_index].w = p_radius;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_cone_radius(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)cones.size(), 0.0);
	return cones[p_index].w;
}

bool JointLimitationKusudama3D::_set(const StringName &p_name, const Variant &p_value) {
	String prop_name = p_name;
	if (prop_name == "cone_count") {
		set_cone_count(p_value);
		return true;
	}
	if (prop_name.begins_with("cones/")) {
		int index = prop_name.get_slicec('/', 1).to_int();
		String what = prop_name.get_slicec('/', 2);
		if (what == "center") {
			set_cone_center(index, p_value);
			return true;
		}
		if (what == "radius") {
			set_cone_radius(index, p_value);
			return true;
		}
	}
	return false;
}

bool JointLimitationKusudama3D::_get(const StringName &p_name, Variant &r_ret) const {
	String prop_name = p_name;
	if (prop_name == "cone_count") {
		r_ret = get_cone_count();
		return true;
	}
	if (prop_name.begins_with("cones/")) {
		int index = prop_name.get_slicec('/', 1).to_int();
		String what = prop_name.get_slicec('/', 2);
		if (what == "center") {
			r_ret = get_cone_center(index);
			return true;
		}
		if (what == "radius") {
			r_ret = get_cone_radius(index);
			return true;
		}
	}
	return false;
}

void JointLimitationKusudama3D::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::INT, PNAME("cone_count"), PROPERTY_HINT_RANGE, "0,10,1", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_ARRAY, "Cones," + String(PNAME("cones")) + "/"));
	for (int i = 0; i < get_cone_count(); i++) {
		const String prefix = vformat("%s/%d/", PNAME("cones"), i);
		p_list->push_back(PropertyInfo(Variant::VECTOR3, prefix + PNAME("center"), PROPERTY_HINT_NONE, ""));
		p_list->push_back(PropertyInfo(Variant::FLOAT, prefix + PNAME("radius"), PROPERTY_HINT_RANGE, "1,180,0.1,radians_as_degrees"));
	}
}

static const int SWEEP_STEPS = 64;

static Vector3 project_on_cone_boundary(const Vector3 &p_point, const Vector3 &p_center, real_t p_radius) {
	Vector3 ortho = (p_point - p_point.project(p_center)).normalized();
	if (!ortho.is_finite()) {
		ortho = (Math::abs(p_center.z) < 0.9f) ? p_center.cross(Vector3(0, 0, 1)).normalized() : p_center.cross(Vector3(1, 0, 0)).normalized();
	}
	return (p_center * Math::cos(p_radius) + ortho * Math::sin(p_radius)).normalized();
}

Vector3 JointLimitationKusudama3D::_solve(const Vector3 &p_direction) const {
	Vector3 p = p_direction.normalized();
	uint32_t n = cones.size();
	if (n == 0) {
		return p;
	}

	// Swept-cone region: one sweep gives containment and the nearest boundary.
	real_t closest = 1e18f;
	Vector3 best = p;

	for (uint32_t i = 0; i < n; i++) {
		Vector3 c = _get_cone_center_normalized(i);
		real_t r = cones[i].w;
		if (p.dot(c) >= Math::cos(r)) {
			return p;
		}
		Vector3 b = project_on_cone_boundary(p, c, r);
		real_t d = p.distance_squared_to(b);
		if (d < closest) {
			closest = d;
			best = b;
		}
	}

	for (uint32_t i = 0; i + 1 < n; i++) {
		Vector3 c1 = _get_cone_center_normalized(i);
		Vector3 c2 = _get_cone_center_normalized(i + 1);
		real_t r1 = cones[i].w;
		real_t r2 = cones[i + 1].w;
		Vector3 axis = c1.cross(c2);
		if (axis.is_zero_approx()) {
			continue;
		}
		axis.normalize();
		real_t omega = c1.angle_to(c2);
		real_t seg_gap = omega / real_t(SWEEP_STEPS);
		for (int s = 1; s < SWEEP_STEPS; s++) {
			real_t theta = omega * real_t(s) / real_t(SWEEP_STEPS);
			Vector3 center = c1 * Math::cos(theta) + axis.cross(c1) * Math::sin(theta);
			real_t radius = r1 + (r2 - r1) * real_t(s) / real_t(SWEEP_STEPS);
			// Widen by the sampling gap: a legal direction between samples stays open.
			if (p.dot(center) >= Math::cos(radius + seg_gap)) {
				return p;
			}
			Vector3 b = project_on_cone_boundary(p, center, radius);
			real_t d = p.distance_squared_to(b);
			if (d < closest) {
				closest = d;
				best = b;
			}
		}
	}
	return best;
}

// Helper functions for kusudama solving

#ifdef TOOLS_ENABLED

int JointLimitationKusudama3D::get_cone_sequence_for_shader(PackedVector4Array &r_cone_sequence) const {
	r_cone_sequence.clear();
	uint32_t n = MIN(cones.size(), (uint32_t)MAX_KUSUDAMA_CONES);
	if (n == 0) {
		return 0;
	}
	// Layout: cone0, tangent0_1, tangent0_2, cone1, tangent1_1, tangent1_2, cone2, ...
	for (uint32_t i = 0; i < n; i++) {
		Vector3 center_i = _get_cone_center_normalized(i);
		real_t radius_i = cones[i].w;
		if (i == 0) {
			r_cone_sequence.push_back(Vector4(center_i.x, center_i.y, center_i.z, radius_i));
		}
		if (i + 1 < n) {
			Vector3 center_next = _get_cone_center_normalized(i + 1);
			real_t radius_next = cones[i + 1].w;
			Vector3 tan1, tan2;
			real_t trad;
			compute_tangent_circles(center_i, radius_i, center_next, radius_next, tan1, tan2, trad);
			r_cone_sequence.push_back(Vector4(tan1.x, tan1.y, tan1.z, (real_t)trad));
			r_cone_sequence.push_back(Vector4(tan2.x, tan2.y, tan2.z, (real_t)trad));
			r_cone_sequence.push_back(Vector4(center_next.x, center_next.y, center_next.z, radius_next));
		}
	}
	return n;
}

void JointLimitationKusudama3D::get_kusudama_fill_mesh_and_material(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const {
	r_mesh.unref();
	r_material.unref();
	PackedVector4Array cone_sequence;
	int cone_count = get_cone_sequence_for_shader(cone_sequence);
	if (cone_count <= 0) {
		return;
	}
	real_t sphere_r = p_bone_length * (real_t)0.25;
	const int rings = 16;
	const int radial_segments = 16;
	Vector<Vector3> points;
	Vector<Vector3> normals;
	Vector<int> indices;
	int thisrow = 0;
	int prevrow = 0;
	int point = 0;
	for (int j = 0; j <= (rings + 1); j++) {
		float v = (float)j / (float)(rings + 1);
		float w = Math::sin(Math::PI * v);
		float y = Math::cos(Math::PI * v);
		for (int i = 0; i <= radial_segments; i++) {
			float u = (float)i / (float)radial_segments;
			float x = Math::sin(u * Math::TAU);
			float z = Math::cos(u * Math::TAU);
			Vector3 p = Vector3(x * w, y, z * w);
			points.push_back(p.normalized());
			normals.push_back(p.normalized());
			point++;
			if (i > 0 && j > 0) {
				indices.push_back(prevrow + i - 1);
				indices.push_back(prevrow + i);
				indices.push_back(thisrow + i - 1);
				indices.push_back(prevrow + i);
				indices.push_back(thisrow + i);
				indices.push_back(thisrow + i - 1);
			}
		}
		prevrow = thisrow;
		thisrow = point;
	}
	if (indices.is_empty()) {
		return;
	}
	const bool use_skin = (p_bone_index >= 0);
	Ref<SurfaceTool> st;
	st.instantiate();
	st->begin(Mesh::PRIMITIVE_TRIANGLES);
	st->set_custom_format(0, SurfaceTool::CUSTOM_RGBA_HALF);
	if (use_skin) {
		PackedInt32Array bones;
		PackedFloat32Array weights;
		bones.resize(Mesh::ARRAY_WEIGHTS_SIZE);
		weights.resize(Mesh::ARRAY_WEIGHTS_SIZE);
		for (int k = 0; k < Mesh::ARRAY_WEIGHTS_SIZE; k++) {
			bones.write[k] = (k == 0) ? p_bone_index : 0;
			weights.write[k] = (k == 0) ? 1.0f : 0.0f;
		}
		st->set_bones(bones);
		st->set_weights(weights);
	}
	for (int idx = 0; idx < points.size(); idx++) {
		Vector3 n = normals[idx];
		Vector3 pos = points[idx];
		if (use_skin) {
			// Vertices must be in skeleton global rest space for Godot's skin (bind = inverse bone rest).
			pos = p_transform.xform(pos * sphere_r);
			n = p_transform.basis.xform(n).normalized();
		}
		Color c;
		c.r = n.x;
		c.g = n.y;
		c.b = n.z;
		c.a = 0.0f;
		st->set_custom(0, c);
		st->set_normal(n);
		st->add_vertex(pos);
	}
	for (int idx : indices) {
		st->add_index(idx);
	}
	r_mesh = st->commit();

	Ref<Shader> sh;
	sh.instantiate();
	sh->set_code(KUSUDAMA_GIZMO_SHADER);
	Ref<ShaderMaterial> mat;
	mat.instantiate();
	mat->set_shader(sh);
	Color boundary_color;
	boundary_color.set_ok_hsl(
			p_color.get_ok_hsl_h(),
			p_color.get_ok_hsl_s(),
			CLAMP((float)p_color.get_ok_hsl_l() - 0.25f, 0.0f, 1.0f),
			p_color.a);

	mat->set_shader_parameter("cone_count", cone_count);
	mat->set_shader_parameter("cone_sequence", cone_sequence);
	mat->set_shader_parameter("kusudama_color", p_color);
	mat->set_shader_parameter("boundary_outline_color", boundary_color);
	r_material = mat;

	if (use_skin) {
		r_mesh_to_skeleton_rest = Transform3D();
	} else {
		r_mesh_to_skeleton_rest = p_transform;
		r_mesh_to_skeleton_rest.basis.scale(Vector3(sphere_r, sphere_r, sphere_r));
	}
}

void JointLimitationKusudama3D::get_twist_gizmo_mesh(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const {
	r_mesh.unref();
	r_material.unref();
	real_t span = twist_to - twist_from;
	if (span <= 0.0) {
		return;
	}
	real_t radius = p_bone_length * (real_t)0.2;
	const int segments = 24;
	const bool use_skin = (p_bone_index >= 0);
	PackedVector3Array verts;
	PackedVector3Array normals;
	PackedInt32Array bones;
	PackedFloat32Array weights;
	// Pie wedge in the plane perpendicular to the forward (+Y) axis.
	Vector3 center(0, 0, 0);
	for (int i = 0; i < segments; i++) {
		real_t a0 = twist_from + span * real_t(i) / real_t(segments);
		real_t a1 = twist_from + span * real_t(i + 1) / real_t(segments);
		Vector3 p0(Math::sin(a0) * radius, 0, Math::cos(a0) * radius);
		Vector3 p1(Math::sin(a1) * radius, 0, Math::cos(a1) * radius);
		Vector3 tri[3] = { center, p0, p1 };
		for (int v = 0; v < 3; v++) {
			Vector3 pos = use_skin ? p_transform.xform(tri[v]) : tri[v];
			verts.push_back(pos);
			normals.push_back(Vector3(0, 1, 0));
			if (use_skin) {
				for (int k = 0; k < Mesh::ARRAY_WEIGHTS_SIZE; k++) {
					bones.push_back((k == 0) ? p_bone_index : 0);
					weights.push_back((k == 0) ? 1.0f : 0.0f);
				}
			}
		}
	}
	if (verts.is_empty()) {
		return;
	}
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = verts;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	if (use_skin) {
		arrays[Mesh::ARRAY_BONES] = bones;
		arrays[Mesh::ARRAY_WEIGHTS] = weights;
	}
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	r_mesh = mesh;

	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	Color twist_color = p_color;
	twist_color.a = 0.5f;
	mat->set_albedo(twist_color);
	mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	r_material = mat;

	if (use_skin) {
		r_mesh_to_skeleton_rest = Transform3D();
	} else {
		r_mesh_to_skeleton_rest = p_transform;
	}
}

void JointLimitationKusudama3D::append_extra_gizmo_meshes(const Transform3D &p_transform, float p_bone_length, const Color &p_color, Vector<ExtraMeshEntry> &r_extra_meshes, int p_bone_index) const {
	ExtraMeshEntry e;
	get_kusudama_fill_mesh_and_material(p_transform, p_bone_length, p_color, p_bone_index, e.transform, e.mesh, e.material);
	if (e.mesh.is_valid()) {
		r_extra_meshes.push_back(e);
	}
	ExtraMeshEntry twist;
	get_twist_gizmo_mesh(p_transform, p_bone_length, p_color, p_bone_index, twist.transform, twist.mesh, twist.material);
	if (twist.mesh.is_valid()) {
		r_extra_meshes.push_back(twist);
	}
}
#endif // TOOLS_ENABLED

// Helper function implementations
bool JointLimitationKusudama3D::is_point_in_cone(const Vector3 &p_point, const Vector3 &p_cone_center, real_t p_cone_radius) const {
	if (p_point.is_zero_approx()) {
		return false;
	}
	return p_point.normalized().angle_to(p_cone_center) <= p_cone_radius;
}

void JointLimitationKusudama3D::set_twist_from(real_t p_radians) {
	twist_from = p_radians;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_twist_from() const {
	return twist_from;
}

void JointLimitationKusudama3D::set_twist_to(real_t p_radians) {
	twist_to = p_radians;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_twist_to() const {
	return twist_to;
}

// log of p_q shifted by whole 720-degree phases nearest p_ln_neighbor (makima patch).
static Quaternion get_nearest_log(const Quaternion &p_q, const Quaternion &p_ln_neighbor) {
	const real_t PHASE = 2.0 * (real_t)Math::TAU;
	const real_t INV_PHASE = 1.0 / PHASE;
	Vector3 neighbor(p_ln_neighbor.x, p_ln_neighbor.y, p_ln_neighbor.z);
	Vector3 axis(p_q.x, p_q.y, p_q.z);
	real_t angle = 2.0 * Math::atan2(axis.length(), p_q.w);
	axis = axis.is_zero_approx() ? neighbor.normalized() : axis.normalized();
	real_t phases = Math::round((neighbor.dot(axis) - angle) * INV_PHASE);
	Vector3 nearest = axis * (angle + phases * PHASE);
	return Quaternion(nearest.x, nearest.y, nearest.z, 0);
}

real_t JointLimitationKusudama3D::twist_angle_continuous(const Quaternion &p_rotation, const Vector3 &p_twist_axis, real_t p_previous_angle) const {
	Vector3 axis = p_twist_axis.normalized();
	Vector3 rot_axis(p_rotation.x, p_rotation.y, p_rotation.z);
	Vector3 proj = axis * rot_axis.dot(axis);
	Quaternion twist(proj.x, proj.y, proj.z, p_rotation.w);
	if (twist.length_squared() < CMP_EPSILON) {
		twist = Quaternion();
	} else {
		twist = twist.normalized();
	}
	Quaternion ln_prev(axis.x * p_previous_angle, axis.y * p_previous_angle, axis.z * p_previous_angle, 0);
	Quaternion ln = get_nearest_log(twist, ln_prev);
	return Vector3(ln.x, ln.y, ln.z).dot(axis);
}

real_t JointLimitationKusudama3D::clamp_twist(real_t p_angle) const {
	return CLAMP(p_angle, twist_from, twist_to);
}

bool JointLimitationKusudama3D::is_point_in_tangent_path(const Vector3 &p_point, const Vector3 &p_center1, real_t p_radius1, const Vector3 &p_center2, real_t p_radius2) const {
	Vector3 dir = p_point.normalized();

	// Check if point is in the inter-cone path region using get_on_great_tangent_triangle
	// This function handles all the geometric checks including whether the point is inside tangent circles
	Vector3 path_point = get_on_great_tangent_triangle(dir, p_center1, p_radius1, p_center2, p_radius2);

	// If NaN, point is not in path region
	if (Math::is_nan(path_point.x)) {
		return false;
	}

	// If the returned point is approximately equal to the input point, point is in the path region.
	// Use a threshold with small margin to avoid flips when cosine is near the boundary (3+ cone stability).
	// get_on_great_tangent_triangle returns the input if in path, or a boundary point if inside a tangent circle.
	real_t cosine = path_point.dot(dir);
	const real_t in_path_cosine_min = 1.0 - 1e-4;
	return cosine >= in_path_cosine_min;
}

Vector3 JointLimitationKusudama3D::get_on_great_tangent_triangle(const Vector3 &p_point, const Vector3 &p_center1, real_t p_radius1, const Vector3 &p_center2, real_t p_radius2) const {
	Vector3 center1 = p_center1.normalized();
	Vector3 center2 = p_center2.normalized();
	Vector3 input = p_point.normalized();

	// Compute tangent circles
	Vector3 tan1, tan2;
	real_t tan_radius;
	compute_tangent_circles(center1, p_radius1, center2, p_radius2, tan1, tan2, tan_radius);

	real_t tan_radius_cos = Math::cos(tan_radius);

	// Determine which side of the arc we're on (tie-break arc_side_dot == 0 with second branch for stability).
	Vector3 arc_normal = center1.cross(center2);
	real_t arc_side_dot = input.dot(arc_normal);

	if (arc_side_dot <= 0.0) {
		// Use first tangent circle
		Vector3 cone1_cross_tangent1 = center1.cross(tan1);
		Vector3 tangent1_cross_cone2 = tan1.cross(center2);
		if (input.dot(cone1_cross_tangent1) > 0 && input.dot(tangent1_cross_cone2) > 0) {
			real_t to_next_cos = input.dot(tan1);
			if (to_next_cos > tan_radius_cos) {
				// Project onto tangent circle, but move slightly outside to ensure it's in the allowed region
				Vector3 plane_normal = tan1.cross(input);
				if (!plane_normal.is_finite() || plane_normal.is_zero_approx()) {
					plane_normal = Vector3::UP;
				}
				plane_normal.normalize();
				// Use slightly larger angle to move point outside the tangent circle (into allowed region)
				real_t adjusted_tan_radius = tan_radius + 5e-5;
				Quaternion rotate_about_by = Quaternion(plane_normal, adjusted_tan_radius);
				return rotate_about_by.xform(tan1).normalized();
			} else {
				return input;
			}
		}
	} else {
		// Use second tangent circle
		Vector3 tangent2_cross_cone1 = tan2.cross(center1);
		Vector3 cone2_cross_tangent2 = center2.cross(tan2);
		if (input.dot(tangent2_cross_cone1) > 0 && input.dot(cone2_cross_tangent2) > 0) {
			real_t to_next_cos = input.dot(tan2);
			if (to_next_cos > tan_radius_cos) {
				// Project onto tangent circle, but move slightly outside to ensure it's in the allowed region
				Vector3 plane_normal = tan2.cross(input);
				if (!plane_normal.is_finite() || plane_normal.is_zero_approx()) {
					plane_normal = Vector3::UP;
				}
				plane_normal.normalize();
				// Use slightly larger angle to move point outside the tangent circle (into allowed region)
				real_t adjusted_tan_radius = tan_radius + 5e-5;
				Quaternion rotate_about_by = Quaternion(plane_normal, adjusted_tan_radius);
				return rotate_about_by.xform(tan2).normalized();
			} else {
				return input;
			}
		}
	}

	return Vector3(NAN, NAN, NAN);
}

void JointLimitationKusudama3D::extend_ray(Vector3 &r_start, Vector3 &r_end, real_t p_amount) const {
	Vector3 mid_point = r_start.lerp(r_end, (real_t)0.5);
	r_start += mid_point.direction_to(r_start) * p_amount;
	r_end += mid_point.direction_to(r_end) * p_amount;
}

int JointLimitationKusudama3D::ray_sphere_intersection_full(const Vector3 &p_ray_start, const Vector3 &p_ray_end, const Vector3 &p_sphere_center, real_t p_radius, Vector3 *r_intersection1, Vector3 *r_intersection2) const {
	Vector3 ray_start_rel = p_ray_start - p_sphere_center;
	Vector3 ray_end_rel = p_ray_end - p_sphere_center;
	Vector3 ray_dir_normalized = ray_start_rel.direction_to(ray_end_rel);
	Vector3 ray_to_center = -ray_start_rel;
	real_t ray_dot_center = ray_dir_normalized.dot(ray_to_center);
	real_t radius_squared = p_radius * p_radius;
	real_t center_dist_squared = ray_to_center.length_squared();
	real_t ray_dot_squared = ray_dot_center * ray_dot_center;
	real_t discriminant = radius_squared - center_dist_squared + ray_dot_squared;

	if (discriminant < 0.0) {
		return 0; // No intersection
	}

	real_t sqrt_discriminant = Math::sqrt(discriminant);
	real_t t1 = ray_dot_center - sqrt_discriminant;
	real_t t2 = ray_dot_center + sqrt_discriminant;

	if (r_intersection1) {
		*r_intersection1 = p_ray_start + ray_dir_normalized * t1;
	}
	if (r_intersection2) {
		*r_intersection2 = p_ray_start + ray_dir_normalized * t2;
	}

	return discriminant > 0.0 ? 2 : 1; // Two intersections or one (tangent)
}

void JointLimitationKusudama3D::compute_tangent_circles(const Vector3 &p_center1, real_t p_radius1, const Vector3 &p_center2, real_t p_radius2, Vector3 &r_tangent1, Vector3 &r_tangent2, real_t &r_tangent_radius) const {
	Vector3 center1 = p_center1.normalized();
	Vector3 center2 = p_center2.normalized();

	// Compute tangent circle radius
	r_tangent_radius = (Math::PI - (p_radius1 + p_radius2)) / 2.0;

	// Find arc normal (axis perpendicular to both cone centers)
	Vector3 arc_normal = center1.cross(center2);
	real_t arc_normal_len = arc_normal.length();

	if (Math::is_zero_approx(arc_normal_len)) {
		// Cones are parallel or opposite - handle specially
		arc_normal = center1.get_any_perpendicular();
		if (arc_normal.is_zero_approx()) {
			arc_normal = Vector3::UP;
		}
		arc_normal.normalize();

		// For opposite cones, tangent circles are at 90 degrees from the cone centers
		Vector3 perp1 = center1.get_any_perpendicular().normalized();

		// Rotate around center1 by the tangent radius to get tangent centers
		Quaternion rot1 = Quaternion(center1, r_tangent_radius);
		Quaternion rot2 = Quaternion(center1, -r_tangent_radius);
		r_tangent1 = rot1.xform(perp1).normalized();
		r_tangent2 = rot2.xform(perp1).normalized();
		return;
	}
	arc_normal.normalize();

	// Use plane intersection method
	real_t boundary_plus_tangent_radius_a = p_radius1 + r_tangent_radius;
	real_t boundary_plus_tangent_radius_b = p_radius2 + r_tangent_radius;

	// The axis of this cone, scaled to minimize its distance to the tangent contact points
	Vector3 scaled_axis_a = center1 * Math::cos(boundary_plus_tangent_radius_a);
	// A point on the plane running through the tangent contact points
	Vector3 safe_arc_normal = arc_normal;
	if (Math::is_zero_approx(safe_arc_normal.length_squared())) {
		safe_arc_normal = Vector3::UP;
	}
	Quaternion temp_var = Quaternion(safe_arc_normal.normalized(), boundary_plus_tangent_radius_a);
	Vector3 plane_dir1_a = temp_var.xform(center1);
	// Another point on the same plane
	Vector3 safe_center1 = center1;
	if (Math::is_zero_approx(safe_center1.length_squared())) {
		safe_center1 = Vector3::BACK;
	}
	Quaternion temp_var2 = Quaternion(safe_center1.normalized(), Math::PI / 2);
	Vector3 plane_dir2_a = temp_var2.xform(plane_dir1_a);

	Vector3 scaled_axis_b = center2 * Math::cos(boundary_plus_tangent_radius_b);
	// A point on the plane running through the tangent contact points
	Quaternion temp_var3 = Quaternion(safe_arc_normal.normalized(), boundary_plus_tangent_radius_b);
	Vector3 plane_dir1_b = temp_var3.xform(center2);
	// Another point on the same plane
	Vector3 safe_center2 = center2;
	if (Math::is_zero_approx(safe_center2.length_squared())) {
		safe_center2 = Vector3::BACK;
	}
	Quaternion temp_var4 = Quaternion(safe_center2.normalized(), Math::PI / 2);
	Vector3 plane_dir2_b = temp_var4.xform(plane_dir1_b);

	// Ray from scaled center of next cone to half way point between the circumference of this cone and the next cone
	Vector3 ray1_b_start = plane_dir1_b;
	Vector3 ray1_b_end = scaled_axis_b;
	Vector3 ray2_b_start = plane_dir1_b;
	Vector3 ray2_b_end = plane_dir2_b;

	extend_ray(ray1_b_start, ray1_b_end, 99.0);
	extend_ray(ray2_b_start, ray2_b_end, 99.0);

	Plane plane_ta(scaled_axis_a, plane_dir1_a, plane_dir2_a);
	Vector3 intersection1;
	Vector3 intersection2;
	if (!plane_ta.intersects_ray(ray1_b_start, ray1_b_start.direction_to(ray1_b_end), &intersection1)) {
		intersection1 = Vector3(NAN, NAN, NAN);
	}
	if (!plane_ta.intersects_ray(ray2_b_start, ray2_b_start.direction_to(ray2_b_end), &intersection2)) {
		intersection2 = Vector3(NAN, NAN, NAN);
	}

	Vector3 intersection_ray_start = intersection1;
	Vector3 intersection_ray_end = intersection2;
	extend_ray(intersection_ray_start, intersection_ray_end, 99.0);

	Vector3 sphere_intersect1;
	Vector3 sphere_intersect2;
	ray_sphere_intersection_full(intersection_ray_start, intersection_ray_end, Vector3(), 1.0, &sphere_intersect1, &sphere_intersect2);

	r_tangent1 = sphere_intersect1.normalized();
	r_tangent2 = sphere_intersect2.normalized();

	// Handle degenerate tangent centers (NaN or zero)
	if (!r_tangent1.is_finite() || Math::is_zero_approx(r_tangent1.length_squared())) {
		r_tangent1 = center1.get_any_perpendicular();
		if (Math::is_zero_approx(r_tangent1.length_squared())) {
			r_tangent1 = Vector3::UP;
		}
		r_tangent1.normalize();
	}
	if (!r_tangent2.is_finite() || Math::is_zero_approx(r_tangent2.length_squared())) {
		Vector3 orthogonal_base = r_tangent1.is_finite() ? r_tangent1 : center1;
		r_tangent2 = orthogonal_base.get_any_perpendicular();
		if (Math::is_zero_approx(r_tangent2.length_squared())) {
			r_tangent2 = Vector3::RIGHT;
		}
		r_tangent2.normalize();
	}
}
