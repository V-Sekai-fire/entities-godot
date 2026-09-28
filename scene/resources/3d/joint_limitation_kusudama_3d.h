/**************************************************************************/
/*  joint_limitation_kusudama_3d.h                                        */
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

#include "core/math/math_funcs.h"
#include "core/math/quaternion.h"
#include "core/templates/local_vector.h"
#include "scene/resources/3d/joint_limitation_3d.h"
#include "scene/resources/mesh.h"

class JointLimitationKusudama3D : public JointLimitation3D {
	GDCLASS(JointLimitationKusudama3D, JointLimitation3D);

	// Cones data: each cone is stored as Vector4(center_x, center_y, center_z, radius)
	// Center is stored raw for serialization and direct input (like gravity_direction in SpringBoneSimulator).
	LocalVector<Vector4> cones;
	// Cached normalized centers for internal use; invalidated when cones change.
	mutable LocalVector<Vector3> _normalized_cone_centers_cache;

	real_t twist_from = -Math::PI;
	real_t twist_to = Math::PI;

	// Soft-limit band (radians). The solve is continuous at any value; this eases the
	// cone boundary (C1). Soft-on by default so no configuration can produce a jerk.
	real_t soft_band = 0.06;
	real_t soft_temperature = 0.14;

	// Prismatic facet: a solved length DOF along the bone axis, off by default.
	bool prismatic_enabled = false;
	real_t prismatic_min = 0.0;
	real_t prismatic_max = 0.0;

	void _invalidate_normalized_cache() const;
	Vector3 _get_cone_center_normalized(int p_index) const;

#ifdef TOOLS_ENABLED
	typedef Pair<Vector3, Vector3> Segment;
#endif // TOOLS_ENABLED

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	virtual Vector3 _solve(const Vector3 &p_direction) const override;

private:
	// Softmin-blended soft cones; ports Kusudama.lean continuousProject.
	Vector3 _soft_project(const Vector3 &p_point) const;

	void extend_ray(Vector3 &r_start, Vector3 &r_end, real_t p_amount) const;
	int ray_sphere_intersection_full(const Vector3 &p_ray_start, const Vector3 &p_ray_end, const Vector3 &p_sphere_center, real_t p_radius, Vector3 *r_intersection1, Vector3 *r_intersection2) const;
	void compute_tangent_circles(const Vector3 &p_center1, real_t p_radius1, const Vector3 &p_center2, real_t p_radius2, Vector3 &r_tangent1, Vector3 &r_tangent2, real_t &r_tangent_radius) const;

public:
	void set_cones(const Vector<Vector4> &p_cones);
	Vector<Vector4> get_cones() const;

	void set_cone_count(int p_count);
	int get_cone_count() const;

	void set_cone_center(int p_index, const Vector3 &p_center);
	Vector3 get_cone_center(int p_index) const;

	void set_cone_radius(int p_index, real_t p_radius);
	real_t get_cone_radius(int p_index) const;

	void set_twist_from(real_t p_radians);
	real_t get_twist_from() const;
	void set_twist_to(real_t p_radians);
	real_t get_twist_to() const;

	void set_soft_band(real_t p_radians);
	real_t get_soft_band() const;
	void set_soft_temperature(real_t p_temperature);
	real_t get_soft_temperature() const;

	void set_prismatic_enabled(bool p_enabled);
	bool is_prismatic_enabled() const;
	void set_prismatic_min(real_t p_min);
	real_t get_prismatic_min() const;
	void set_prismatic_max(real_t p_max);
	real_t get_prismatic_max() const;
	// Clamped projection of the target onto the bone axis; ports PrismaticJoint.lean.
	real_t optimal_length(const Vector3 &p_head, const Vector3 &p_target, const Vector3 &p_bone_dir, real_t p_fixed_length) const;

	real_t twist_angle_continuous(const Quaternion &p_rotation, const Vector3 &p_twist_axis, real_t p_previous_angle) const;
	real_t clamp_twist(real_t p_angle) const;

	static const int MAX_KUSUDAMA_CONES = 3;

#ifdef TOOLS_ENABLED
	int get_cone_sequence_for_shader(PackedVector4Array &r_cone_sequence) const;
	// r_mesh_to_skeleton_rest: transform from mesh local to skeleton global rest space. Identity when skinned (p_bone_index >= 0); otherwise constraint pose with sphere scale.
	void get_kusudama_fill_mesh_and_material(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const;
	void get_twist_gizmo_mesh(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const;
	void get_soft_band_gizmo_mesh(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const;
	void get_prismatic_gizmo_mesh(const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Transform3D &r_mesh_to_skeleton_rest, Ref<ArrayMesh> &r_mesh, Ref<Material> &r_material) const;
	virtual void append_extra_gizmo_meshes(const Transform3D &p_transform, float p_bone_length, const Color &p_color, Vector<ExtraMeshEntry> &r_extra_meshes, int p_bone_index = -1) const override;

#endif // TOOLS_ENABLED
};
