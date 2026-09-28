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

	ClassDB::bind_method(D_METHOD("set_soft_band", "radians"), &JointLimitationKusudama3D::set_soft_band);
	ClassDB::bind_method(D_METHOD("get_soft_band"), &JointLimitationKusudama3D::get_soft_band);
	ClassDB::bind_method(D_METHOD("set_soft_temperature", "temperature"), &JointLimitationKusudama3D::set_soft_temperature);
	ClassDB::bind_method(D_METHOD("get_soft_temperature"), &JointLimitationKusudama3D::get_soft_temperature);

	ClassDB::bind_method(D_METHOD("set_prismatic_enabled", "enabled"), &JointLimitationKusudama3D::set_prismatic_enabled);
	ClassDB::bind_method(D_METHOD("is_prismatic_enabled"), &JointLimitationKusudama3D::is_prismatic_enabled);
	ClassDB::bind_method(D_METHOD("set_prismatic_min", "min"), &JointLimitationKusudama3D::set_prismatic_min);
	ClassDB::bind_method(D_METHOD("get_prismatic_min"), &JointLimitationKusudama3D::get_prismatic_min);
	ClassDB::bind_method(D_METHOD("set_prismatic_max", "max"), &JointLimitationKusudama3D::set_prismatic_max);
	ClassDB::bind_method(D_METHOD("get_prismatic_max"), &JointLimitationKusudama3D::get_prismatic_max);
	ClassDB::bind_method(D_METHOD("optimal_length", "head", "target", "bone_dir", "fixed_length"), &JointLimitationKusudama3D::optimal_length);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "cones", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_cones", "get_cones");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "twist_from", PROPERTY_HINT_RANGE, "-360,360,0.1,radians_as_degrees"), "set_twist_from", "get_twist_from");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "twist_to", PROPERTY_HINT_RANGE, "-360,360,0.1,radians_as_degrees"), "set_twist_to", "get_twist_to");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "soft_band", PROPERTY_HINT_RANGE, "0,90,0.1,radians_as_degrees"), "set_soft_band", "get_soft_band");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "soft_temperature", PROPERTY_HINT_RANGE, "0.001,1,0.001"), "set_soft_temperature", "get_soft_temperature");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "prismatic_enabled"), "set_prismatic_enabled", "is_prismatic_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "prismatic_min", PROPERTY_HINT_RANGE, "0,10,0.001,or_greater,suffix:m"), "set_prismatic_min", "get_prismatic_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "prismatic_max", PROPERTY_HINT_RANGE, "0,10,0.001,or_greater,suffix:m"), "set_prismatic_max", "get_prismatic_max");
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

static Vector3 any_perp(const Vector3 &p_axis) {
	Vector3 perp = (Math::abs(p_axis.z) < 0.9f) ? p_axis.cross(Vector3(0, 0, 1)) : p_axis.cross(Vector3(1, 0, 0));
	return perp.normalized();
}

// Ease the swing angle toward the cone radius from below; never exceeds it.
static Vector3 soft_cone(const Vector3 &p_point, const Vector3 &p_center, real_t p_radius, real_t p_band) {
	real_t th = p_point.angle_to(p_center);
	real_t band = MIN(p_band, p_radius);
	real_t th_sat;
	if (band <= 0.0) {
		th_sat = MIN(th, p_radius);
	} else {
		real_t inner = p_radius - band;
		th_sat = (th > inner) ? p_radius - band * Math::exp(-(th - inner) / band) : th;
	}
	Vector3 perp0 = p_point - p_center * p_point.dot(p_center);
	Vector3 perp = perp0.is_zero_approx() ? any_perp(p_center) : perp0.normalized();
	return (p_center * Math::cos(th_sat) + perp * Math::sin(th_sat)).normalized();
}

static Vector3 log_map(const Vector3 &p_a, const Vector3 &p_b) {
	real_t d = p_a.angle_to(p_b);
	if (d < 1e-9f) {
		return Vector3();
	}
	Vector3 tangent = p_b - p_a * p_a.dot(p_b);
	return tangent.normalized() * d;
}

static Vector3 exp_map(const Vector3 &p_a, const Vector3 &p_v) {
	real_t n = p_v.length();
	if (n < 1e-9f) {
		return p_a;
	}
	return (p_a * Math::cos(n) + p_v * (Math::sin(n) / n)).normalized();
}

Vector3 JointLimitationKusudama3D::_soft_project(const Vector3 &p_point) const {
	uint32_t n = cones.size();
	LocalVector<Vector3> candidates;
	LocalVector<real_t> distances;
	candidates.resize(n);
	distances.resize(n);
	real_t d_min = 1e30f;
	Vector3 anchor;
	for (uint32_t i = 0; i < n; i++) {
		Vector3 center = _get_cone_center_normalized(i);
		anchor += center;
		Vector3 q = soft_cone(p_point, center, cones[i].w, soft_band);
		real_t d = p_point.angle_to(q);
		candidates[i] = q;
		distances[i] = d;
		d_min = MIN(d_min, d);
	}
	if (d_min < 1e-5f) {
		return p_point;
	}
	// Blend in the tangent space of a fixed reference (the cone centroid), not the nearest
	// candidate, so the base point never switches and the result stays continuous.
	anchor = anchor.is_zero_approx() ? _get_cone_center_normalized(0) : anchor.normalized();
	real_t temperature = MAX(soft_temperature, (real_t)1e-4);
	Vector3 numerator;
	real_t denominator = 0.0;
	for (uint32_t i = 0; i < n; i++) {
		real_t w = Math::exp(-(distances[i] - d_min) / temperature);
		numerator += log_map(anchor, candidates[i]) * w;
		denominator += w;
	}
	if (denominator <= 1e-30f) {
		return p_point;
	}
	return exp_map(anchor, numerator / denominator);
}

Vector3 JointLimitationKusudama3D::_solve(const Vector3 &p_direction) const {
	Vector3 p = p_direction.normalized();
	if (cones.is_empty()) {
		return p;
	}
	// Always continuous: the softmin blend never jumps at the medial axis, so no
	// parameter reaches a jerk. soft_band adds C1 easing at the cone boundary.
	return _soft_project(p);
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

static void gizmo_push_line(PackedVector3Array &r_verts, PackedVector3Array &r_normals, PackedInt32Array &r_bones, PackedFloat32Array &r_weights, bool p_use_skin, int p_bone_index, const Transform3D &p_transform, const Vector3 &p_a, const Vector3 &p_b) {
	const Vector3 endpoints[2] = { p_a, p_b };
	for (int e = 0; e < 2; e++) {
		r_verts.push_back(p_use_skin ? p_transform.xform(endpoints[e]) : endpoints[e]);
		r_normals.push_back(Vector3(0, 1, 0));
		if (p_use_skin) {
			for (int k = 0; k < Mesh::ARRAY_WEIGHTS_SIZE; k++) {
				r_bones.push_back((k == 0) ? p_bone_index : 0);
				r_weights.push_back((k == 0) ? 1.0f : 0.0f);
			}
		}
	}
}

static Ref<Material> gizmo_line_material(const Color &p_color) {
	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	mat->set_albedo(p_color);
	mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	return mat;
}

static Ref<ArrayMesh> gizmo_lines_to_mesh(const PackedVector3Array &p_verts, const PackedVector3Array &p_normals, const PackedInt32Array &p_bones, const PackedFloat32Array &p_weights, bool p_use_skin) {
	Ref<ArrayMesh> mesh;
	if (p_verts.is_empty()) {
		return mesh;
	}
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = p_verts;
	arrays[Mesh::ARRAY_NORMAL] = p_normals;
	if (p_use_skin) {
		arrays[Mesh::ARRAY_BONES] = p_bones;
		arrays[Mesh::ARRAY_WEIGHTS] = p_weights;
	}
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
	return mesh;
}

void JointLimitationKusudama3D::get_soft_band_gizmo_mesh(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const {
	r_mesh.unref();
	r_material.unref();
	if (soft_band <= 0.0 || cones.is_empty()) {
		return;
	}
	real_t sphere_r = p_bone_length * (real_t)0.25;
	const int segments = 48;
	const bool use_skin = (p_bone_index >= 0);
	PackedVector3Array verts;
	PackedVector3Array normals;
	PackedInt32Array bones;
	PackedFloat32Array weights;
	for (uint32_t i = 0; i < cones.size(); i++) {
		real_t alpha = cones[i].w - soft_band;
		if (alpha <= 0.0) {
			continue;
		}
		Vector3 c = _get_cone_center_normalized(i);
		Vector3 u = (Math::abs(c.z) < 0.9f) ? c.cross(Vector3(0, 0, 1)).normalized() : c.cross(Vector3(1, 0, 0)).normalized();
		Vector3 v = c.cross(u).normalized();
		for (int s = 0; s < segments; s++) {
			real_t t0 = Math::TAU * real_t(s) / real_t(segments);
			real_t t1 = Math::TAU * real_t(s + 1) / real_t(segments);
			Vector3 d0 = (c * Math::cos(alpha) + (u * Math::cos(t0) + v * Math::sin(t0)) * Math::sin(alpha)) * sphere_r;
			Vector3 d1 = (c * Math::cos(alpha) + (u * Math::cos(t1) + v * Math::sin(t1)) * Math::sin(alpha)) * sphere_r;
			gizmo_push_line(verts, normals, bones, weights, use_skin, p_bone_index, p_transform, d0, d1);
		}
	}
	r_mesh = gizmo_lines_to_mesh(verts, normals, bones, weights, use_skin);
	if (r_mesh.is_null()) {
		return;
	}
	Color band_color = p_color;
	band_color.a = 0.9f;
	r_material = gizmo_line_material(band_color);
	r_mesh_to_skeleton_rest = use_skin ? Transform3D() : p_transform;
}

void JointLimitationKusudama3D::get_prismatic_gizmo_mesh(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const {
	r_mesh.unref();
	r_material.unref();
	if (!prismatic_enabled || prismatic_max <= prismatic_min) {
		return;
	}
	const bool use_skin = (p_bone_index >= 0);
	PackedVector3Array verts;
	PackedVector3Array normals;
	PackedInt32Array bones;
	PackedFloat32Array weights;
	// Travel runs along the bone forward axis (+Y); ticks mark the min and max stops.
	Vector3 low(0, prismatic_min, 0);
	Vector3 high(0, prismatic_max, 0);
	gizmo_push_line(verts, normals, bones, weights, use_skin, p_bone_index, p_transform, low, high);
	real_t tick = p_bone_length * (real_t)0.05;
	const Vector3 stops[2] = { low, high };
	for (int e = 0; e < 2; e++) {
		gizmo_push_line(verts, normals, bones, weights, use_skin, p_bone_index, p_transform, stops[e] - Vector3(tick, 0, 0), stops[e] + Vector3(tick, 0, 0));
		gizmo_push_line(verts, normals, bones, weights, use_skin, p_bone_index, p_transform, stops[e] - Vector3(0, 0, tick), stops[e] + Vector3(0, 0, tick));
	}
	r_mesh = gizmo_lines_to_mesh(verts, normals, bones, weights, use_skin);
	if (r_mesh.is_null()) {
		return;
	}
	Color axis_color = p_color;
	axis_color.a = 1.0f;
	r_material = gizmo_line_material(axis_color);
	r_mesh_to_skeleton_rest = use_skin ? Transform3D() : p_transform;
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
	ExtraMeshEntry soft;
	get_soft_band_gizmo_mesh(p_transform, p_bone_length, p_color, p_bone_index, soft.transform, soft.mesh, soft.material);
	if (soft.mesh.is_valid()) {
		r_extra_meshes.push_back(soft);
	}
	ExtraMeshEntry prismatic;
	get_prismatic_gizmo_mesh(p_transform, p_bone_length, p_color, p_bone_index, prismatic.transform, prismatic.mesh, prismatic.material);
	if (prismatic.mesh.is_valid()) {
		r_extra_meshes.push_back(prismatic);
	}
}
#endif // TOOLS_ENABLED

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

void JointLimitationKusudama3D::set_soft_band(real_t p_radians) {
	soft_band = MAX(p_radians, (real_t)0.0);
	emit_changed();
}

real_t JointLimitationKusudama3D::get_soft_band() const {
	return soft_band;
}

void JointLimitationKusudama3D::set_soft_temperature(real_t p_temperature) {
	soft_temperature = p_temperature;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_soft_temperature() const {
	return soft_temperature;
}

void JointLimitationKusudama3D::set_prismatic_enabled(bool p_enabled) {
	prismatic_enabled = p_enabled;
	emit_changed();
}

bool JointLimitationKusudama3D::is_prismatic_enabled() const {
	return prismatic_enabled;
}

void JointLimitationKusudama3D::set_prismatic_min(real_t p_min) {
	prismatic_min = p_min;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_prismatic_min() const {
	return prismatic_min;
}

void JointLimitationKusudama3D::set_prismatic_max(real_t p_max) {
	prismatic_max = p_max;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_prismatic_max() const {
	return prismatic_max;
}

real_t JointLimitationKusudama3D::optimal_length(const Vector3 &p_head, const Vector3 &p_target, const Vector3 &p_bone_dir, real_t p_fixed_length) const {
	if (!prismatic_enabled) {
		return p_fixed_length;
	}
	real_t dir_len_sq = p_bone_dir.length_squared();
	if (Math::is_zero_approx(dir_len_sq)) {
		return prismatic_min;
	}
	real_t projection = (p_target - p_head).dot(p_bone_dir) / dir_len_sq;
	return CLAMP(projection, prismatic_min, prismatic_max);
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
