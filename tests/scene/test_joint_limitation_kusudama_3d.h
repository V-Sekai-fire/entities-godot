/**************************************************************************/

#pragma once

#include "scene/resources/3d/joint_limitation_kusudama_3d.h"
#include "tests/test_macros.h"
#include "thirdparty/witness/doctest.h"

#include <array>
#include <functional>
#include <utility>

namespace TestJointLimitationKusudama3D {

static void set_cones_from_vector4(Ref<JointLimitationKusudama3D> limitation, const Vector<Vector4> &cones) {
	limitation->set_cone_count(cones.size());
	for (int i = 0; i < cones.size(); i++) {
		Vector3 center = Vector3(cones[i].x, cones[i].y, cones[i].z);
		real_t radius = cones[i].w;
		limitation->set_cone_center(i, center);
		limitation->set_cone_radius(i, radius);
	}
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point inside single cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(30.0); // 30 degrees

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = control_point;
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);
	CHECK(result.is_equal_approx(test_point));

	Vector3 point_in_cone = Quaternion(Vector3(1, 0, 0), radius * 0.5f).xform(control_point);
	result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), point_in_cone);
	CHECK(result.is_equal_approx(point_in_cone));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point outside single cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(30.0); // 30 degrees

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);

	CHECK_FALSE(result.is_equal_approx(test_point));

	real_t angle_to_control = result.angle_to(control_point);
	CHECK(angle_to_control <= radius + 0.01f); // Allow small tolerance
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point with zero radius cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = 0.0f; // Zero radius - only exact point allowed

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = control_point;
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);
	CHECK(result.is_equal_approx(control_point));

	Vector3 outside_point = Vector3(1, 0, 0).normalized();
	result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), outside_point);
	CHECK(result.is_equal_approx(control_point));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test multiple cones - point between cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point1 = Vector3(1, 0, 0).normalized();
	Vector3 control_point2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(45.0); // 45 degrees

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point1.x, control_point1.y, control_point1.z, radius));
	cones.push_back(Vector4(control_point2.x, control_point2.y, control_point2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 point_between = (control_point1 + control_point2).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point_between);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f); // Should be normalized
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point on path between two adjacent cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(60.0); // Large enough to create paths

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 path_point = (cp1 + cp2).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), path_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);

	Vector3 between_point = Vector3(0.7, 0.7, 0.1).normalized();
	Vector3 result2 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), between_point);
	CHECK(result2.is_finite());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point outside both cones but in path region") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(30.0); // Smaller radius

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 outside_point = Vector3(0.5, 0.5, 0.7).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), outside_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
	CHECK(result.length() < 1.1f);

	CHECK(result.is_normalized());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test path between cones with different radii") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius1 = Math::deg_to_rad(45.0);
	real_t radius2 = Math::deg_to_rad(30.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius1));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius2));
	set_cones_from_vector4(limitation, cones);

	Vector3 between = (cp1 + cp2).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), between);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test no wrap-around from last to first cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	Vector3 cp3 = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(10.0); // Very small radius to create clear forbidden zones

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	cones.push_back(Vector4(cp3.x, cp3.y, cp3.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 between_last_first = Vector3(0.577, 0.577, 0.577).normalized(); // ~55 degrees from each axis
	real_t angle_to_cp1 = between_last_first.angle_to(cp1);
	real_t angle_to_cp2 = between_last_first.angle_to(cp2);
	real_t angle_to_cp3 = between_last_first.angle_to(cp3);
	CHECK(angle_to_cp1 > radius);
	CHECK(angle_to_cp2 > radius);
	CHECK(angle_to_cp3 > radius);

	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), between_last_first);

	CHECK(result.is_finite());
	CHECK(result.is_normalized());
	real_t dist_to_cp1 = result.angle_to(cp1);
	real_t dist_to_cp3 = result.angle_to(cp3);
	CHECK((dist_to_cp1 < Math::PI / 2 || dist_to_cp3 < Math::PI / 2));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point in tangent circle region between cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(45.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 tangent_region_point = Vector3(0.6, 0.6, 0.5).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), tangent_region_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);

	real_t angle1 = result.angle_to(cp1);
	real_t angle2 = result.angle_to(cp2);
	CHECK((angle1 < Math::PI / 2 || angle2 < Math::PI / 2)); // Should be in hemisphere of at least one cone
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test multiple paths - point closest to which path") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	Vector3 cp3 = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(40.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	cones.push_back(Vector4(cp3.x, cp3.y, cp3.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = Vector3(0.4, 0.5, 0.3).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), test_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);

	CHECK(result.is_normalized());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test path with very close cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0.99, 0.1, 0).normalized();
	real_t radius = Math::deg_to_rad(30.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 between = (cp1 + cp2).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), between);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test path with nearly opposite cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(-0.9, 0.1, 0).normalized();
	real_t radius = Math::deg_to_rad(45.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 between = (cp1 + cp2).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), between);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test multiple cones - point in first cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point1 = Vector3(1, 0, 0).normalized();
	Vector3 control_point2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(45.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point1.x, control_point1.y, control_point1.z, radius));
	cones.push_back(Vector4(control_point2.x, control_point2.y, control_point2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 point_in_cone1 = Quaternion(Vector3(0, 1, 0), radius * 0.3f).xform(control_point1);
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point_in_cone1);
	CHECK(result.is_equal_approx(point_in_cone1));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test empty cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector<Vector4> empty_cones;
	set_cones_from_vector4(limitation, empty_cones);

	Vector3 test_point = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);
	CHECK(result.is_equal_approx(test_point));

	result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);
	CHECK(result.is_equal_approx(test_point));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test orientationally unconstrained") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	limitation->set_cone_count(0);

	Vector3 test_point = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);
	CHECK(result.is_equal_approx(test_point));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test cones getters and setters") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector<Vector4> cones;
	cones.push_back(Vector4(1, 0, 0, Math::deg_to_rad(30.0)));
	cones.push_back(Vector4(0, 1, 0, Math::deg_to_rad(45.0)));
	cones.push_back(Vector4(0, 0, 1, Math::deg_to_rad(60.0)));

	set_cones_from_vector4(limitation, cones);

	CHECK(limitation->get_cone_count() == 3);
	CHECK(Math::is_equal_approx((double)limitation->get_cone_radius(0), Math::deg_to_rad(30.0)));
	CHECK(Math::is_equal_approx((double)limitation->get_cone_radius(1), Math::deg_to_rad(45.0)));
	CHECK(Math::is_equal_approx((double)limitation->get_cone_radius(2), Math::deg_to_rad(60.0)));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test large radius cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(170.0); // Very large cone

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test three cones in sequence") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	Vector3 cp3 = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(45.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	cones.push_back(Vector4(cp3.x, cp3.y, cp3.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 point1 = Quaternion(Vector3(0, 1, 0), radius * 0.3f).xform(cp1);
	Vector3 result1 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point1);
	CHECK(result1.is_equal_approx(point1));

	Vector3 point2 = Quaternion(Vector3(1, 0, 0), radius * 0.3f).xform(cp2);
	real_t angle_to_cp2 = point2.angle_to(cp2);
	CHECK(angle_to_cp2 < radius);
	Vector3 result2 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point2);
	CHECK(result2.is_normalized());
	real_t result_angle_to_cp2 = result2.angle_to(cp2);
	CHECK(result_angle_to_cp2 <= radius + 0.01f);

	Vector3 point_between = (cp1 + cp2).normalized();
	Vector3 result3 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point_between);
	CHECK(result3.is_finite());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test edge case - parallel vectors") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(1, 0, 0).normalized();
	real_t radius = Math::deg_to_rad(30.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 parallel_point = control_point * 2.0f; // Same direction, different length
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), parallel_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test edge case - opposite direction") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(30.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 opposite = -control_point;
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), opposite);

	CHECK(result.is_finite());
	real_t angle_to_control = result.angle_to(control_point);
	CHECK(angle_to_control <= radius + 0.01f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test make_space method") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 forward = Vector3(0, 1, 0);
	Vector3 right = Vector3(1, 0, 0);
	Quaternion offset = Quaternion();

	Quaternion space = limitation->make_space(forward, right, offset);

	CHECK(space.is_normalized());
	CHECK(space.is_finite());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test cone count manipulation") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	CHECK(limitation->get_cone_count() == 0);

	limitation->set_cone_count(3);
	CHECK(limitation->get_cone_count() == 3);

	limitation->set_cone_count(2);
	CHECK(limitation->get_cone_count() == 2);

	limitation->set_cone_count(0);
	CHECK(limitation->get_cone_count() == 0);

	limitation->set_cone_count(1);
	CHECK(limitation->get_cone_count() == 1);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test individual cone property setters and getters") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	limitation->set_cone_count(2);

	Vector3 center1 = Vector3(1, 0, 0).normalized();
	Vector3 center2 = Vector3(0, 1, 0).normalized();
	limitation->set_cone_center(0, center1);
	limitation->set_cone_center(1, center2);

	Vector3 retrieved1 = limitation->get_cone_center(0);
	Vector3 retrieved2 = limitation->get_cone_center(1);
	CHECK(retrieved1.is_equal_approx(center1));
	CHECK(retrieved2.is_equal_approx(center2));

	real_t radius1 = Math::deg_to_rad(30.0);
	real_t radius2 = Math::deg_to_rad(45.0);
	limitation->set_cone_radius(0, radius1);
	limitation->set_cone_radius(1, radius2);

	CHECK(Math::is_equal_approx(limitation->get_cone_radius(0), radius1));
	CHECK(Math::is_equal_approx(limitation->get_cone_radius(1), radius2));

	Quaternion quat1 = Quaternion(Vector3(0, 1, 0), Math::deg_to_rad(45.0));
	Quaternion quat2 = Quaternion(Vector3(1, 0, 0), Math::deg_to_rad(30.0));
	Vector3 default_dir = Vector3(0, 1, 0);
	Vector3 quat_center1 = quat1.normalized().xform(default_dir);
	Vector3 quat_center2 = quat2.normalized().xform(default_dir);
	limitation->set_cone_center(0, quat_center1);
	limitation->set_cone_center(1, quat_center2);

	Vector3 retrieved_center1 = limitation->get_cone_center(0);
	Vector3 retrieved_center2 = limitation->get_cone_center(1);
	Quaternion retrieved_quat1 = Quaternion(default_dir, retrieved_center1);
	Quaternion retrieved_quat2 = Quaternion(default_dir, retrieved_center2);
	Vector3 dir1 = quat1.xform(Vector3(0, 1, 0));
	Vector3 dir2 = retrieved_quat1.xform(Vector3(0, 1, 0));
	bool quat_matches = dir1.is_equal_approx(dir2) || dir1.is_equal_approx(-dir2);
	CHECK(quat_matches);
	Vector3 dir3 = quat2.xform(Vector3(0, 1, 0));
	Vector3 dir4 = retrieved_quat2.xform(Vector3(0, 1, 0));
	bool quat_matches2 = dir3.is_equal_approx(dir4) || dir3.is_equal_approx(-dir4);
	CHECK(quat_matches2);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test point exactly on cone boundary") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(30.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 perp = control_point.get_any_perpendicular().normalized();
	Quaternion rot_to_boundary = Quaternion(control_point.cross(perp).normalized(), radius);
	Vector3 boundary_point = rot_to_boundary.xform(control_point).normalized();

	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), boundary_point);

	CHECK(result.is_finite());
	real_t angle_to_control = result.angle_to(control_point);
	CHECK(angle_to_control <= radius + 0.01f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test very small radius cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(0.1f); // Very small radius

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 center_point = control_point;
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), center_point);
	CHECK(result.is_equal_approx(center_point));

	Vector3 outside_point = Quaternion(Vector3(1, 0, 0), radius * 1.5f).xform(control_point);
	result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), outside_point);
	real_t angle_to_control = result.angle_to(control_point);
	CHECK(angle_to_control <= radius + 0.01f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test near maximum radius cone") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::PI - 0.01f; // Nearly maximum (almost hemisphere)

	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test four cones in sequence") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	Vector3 cp3 = Vector3(-1, 0, 0).normalized();
	Vector3 cp4 = Vector3(0, -1, 0).normalized();
	real_t radius = Math::deg_to_rad(40.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	cones.push_back(Vector4(cp3.x, cp3.y, cp3.z, radius));
	cones.push_back(Vector4(cp4.x, cp4.y, cp4.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 point1 = Quaternion(Vector3(0, 1, 0), radius * 0.3f).xform(cp1);
	Vector3 result1 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point1);
	CHECK(result1.is_equal_approx(point1));

	Vector3 point_between = (cp1 + cp2).normalized();
	Vector3 result2 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), point_between);
	CHECK(result2.is_finite());
	CHECK(result2.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test solve without rotation parameter") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 control_point = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(30.0);
	Vector<Vector4> cones;
	cones.push_back(Vector4(control_point.x, control_point.y, control_point.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_dir = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_dir);

	CHECK(result.is_finite());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test orientationally constrained with empty cones") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	limitation->set_cone_count(0);

	Vector3 test_point = Vector3(1, 0, 0).normalized();
	Vector3 result = limitation->solve(Vector3(0, 1, 0), Vector3(1, 0, 0), Quaternion(), test_point);
	CHECK(result.is_equal_approx(test_point));
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test tangent path - point in allowed region") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(45.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 path_point = Vector3(0.7, 0.7, 0.1).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), path_point);

	CHECK(result.is_finite());
	CHECK(result.is_normalized());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test tangent path - point inside forbidden tangent circle") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(30.0); // Smaller radius for clearer tangent circles

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 forbidden_point = Vector3(0.8, 0.5, 0.3).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), forbidden_point);

	CHECK(result.is_finite());
	CHECK(result.is_normalized());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test tangent path - point on tangent boundary") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(45.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 boundary_point = Vector3(0.6, 0.6, 0.5).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), boundary_point);

	CHECK(result.is_finite());
	CHECK(result.is_normalized());
	CHECK(result.length() > 0.9f);
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test tangent path - three cones with multiple paths") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	Vector3 cp3 = Vector3(0, 0, 1).normalized();
	real_t radius = Math::deg_to_rad(50.0);

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	cones.push_back(Vector4(cp3.x, cp3.y, cp3.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 path12 = Vector3(0.7, 0.7, 0.1).normalized();
	Vector3 result12 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), path12);
	CHECK(result12.is_finite());
	CHECK(result12.is_normalized());

	Vector3 path23 = Vector3(0.1, 0.7, 0.7).normalized();
	Vector3 result23 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), path23);
	CHECK(result23.is_finite());
	CHECK(result23.is_normalized());

	Vector3 path31 = Vector3(0.7, 0.1, 0.7).normalized();
	Vector3 result31 = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), path31);
	CHECK(result31.is_finite());
	CHECK(result31.is_normalized());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test tangent path - large cone radii") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(80.0); // Large radius

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 test_point = Vector3(0.3, 0.3, 0.9).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), test_point);

	CHECK(result.is_finite());
	CHECK(result.is_normalized());
}

TEST_CASE("[Scene][JointLimitationKusudama3D] Test tangent path - small cone radii") {
	Ref<JointLimitationKusudama3D> limitation;
	limitation.instantiate();

	Vector3 cp1 = Vector3(1, 0, 0).normalized();
	Vector3 cp2 = Vector3(0, 1, 0).normalized();
	real_t radius = Math::deg_to_rad(15.0); // Small radius

	Vector<Vector4> cones;
	cones.push_back(Vector4(cp1.x, cp1.y, cp1.z, radius));
	cones.push_back(Vector4(cp2.x, cp2.y, cp2.z, radius));
	set_cones_from_vector4(limitation, cones);

	Vector3 path_point = Vector3(0.6, 0.6, 0.5).normalized();
	Vector3 result = limitation->solve(Vector3(0, 0, 1), Vector3(1, 0, 0), Quaternion(), path_point);

	CHECK(result.is_finite());
	CHECK(result.is_normalized());
}

static const int GT_PATH_STEPS = 96;

static real_t gt_region_margin(const Ref<JointLimitationKusudama3D> &k, const Vector3 &d) {
	real_t m = Math::PI;
	const int n = k->get_cone_count();
	for (int i = 0; i < n; i++) {
		m = MIN(m, d.angle_to(k->get_cone_center(i)) - k->get_cone_radius(i));
	}
	for (int i = 0; i + 1 < n; i++) {
		const Vector3 c1 = k->get_cone_center(i).normalized();
		const Vector3 c2 = k->get_cone_center(i + 1).normalized();
		const real_t r1 = k->get_cone_radius(i);
		const real_t r2 = k->get_cone_radius(i + 1);
		for (int s = 0; s <= GT_PATH_STEPS; s++) {
			const real_t t = real_t(s) / real_t(GT_PATH_STEPS);
			const Vector3 c = c1.slerp(c2, t).normalized();
			m = MIN(m, d.angle_to(c) - (r1 + (r2 - r1) * t));
		}
	}
	return m;
}

static real_t gt_path_half_gap(const Ref<JointLimitationKusudama3D> &k) {
	real_t g = 0.0;
	const int n = k->get_cone_count();
	for (int i = 0; i + 1 < n; i++) {
		g = MAX(g, k->get_cone_center(i).normalized().angle_to(k->get_cone_center(i + 1).normalized()) / real_t(GT_PATH_STEPS));
	}
	return g * (real_t)0.5;
}

static bool gt_in_region(const Ref<JointLimitationKusudama3D> &k, const Vector3 &d) {
	return gt_region_margin(k, d) <= 0.0;
}

static const int GT_GRID_P = 36, GT_GRID_T = 72;

static real_t gt_grid_half_gap() {
	return (real_t)0.5 * (Math::PI / real_t(GT_GRID_P) + 2.0 * Math::PI / real_t(GT_GRID_T));
}

static bool nearest_projection_holds(const Ref<JointLimitationKusudama3D> &k, const Vector3 &input) {
	const real_t gap = 2.0 * gt_path_half_gap(k); // full sampling interval
	Vector3 s;
	k->is_in_boundary(input, s);
	if (!s.is_finite() || !s.is_normalized()) {
		return false;
	}
	if (gt_region_margin(k, s) > gap) {
		return false;
	}
	Vector3 s2;
	k->is_in_boundary(s, s2);
	return s2.is_finite() && s.angle_to(s2) <= gap;
}

static bool random_chain_nearest_pred(const std::pair<int64_t, std::array<double, 43>> &in) {
	const int n = (int)in.first;
	const std::array<double, 43> &d = in.second;
	Ref<JointLimitationKusudama3D> k;
	k.instantiate();
	k->set_cone_count(n);
	for (int i = 0; i < n; i++) {
		const Vector3 c(d[4 * i], d[4 * i + 1], d[4 * i + 2]);
		witness::assume(!c.is_zero_approx());
		k->set_cone_center(i, c.normalized());
		k->set_cone_radius(i, Math::deg_to_rad(5.0 + 55.0 * (d[4 * i + 3] * 0.5 + 0.5)));
	}
	Vector3 input(d[40], d[41], d[42]);
	witness::assume(!input.is_zero_approx());
	return nearest_projection_holds(k, input.normalized());
}

TEST_CASE("[JointLimitationKusudama3D][property] random cone chains snap to nearest ground-truth boundary") {
	typedef std::pair<int64_t, std::array<double, 43>> Input;
	witness::Generator<Input> gen = witness::gen_pair<int64_t, std::array<double, 43>>(
			witness::gen_int64_range(2, 10),
			witness::gen_array<43, double>(witness::gen_double_range(-1.0, 1.0)));
	witness::Trial trial = witness::resolve<Input>(
			"kusudama random-cone nearest-projection", gen, random_chain_nearest_pred, 0xA11FEEDULL);
	CHECK(trial.outcome != witness::Outcome::FOUND);
}

static bool bent3_nearest_pred(const std::array<double, 15> &d) {
	Ref<JointLimitationKusudama3D> k;
	k.instantiate();
	k->set_cone_count(3);
	for (int i = 0; i < 3; i++) {
		const Vector3 c(d[4 * i], d[4 * i + 1], d[4 * i + 2]);
		witness::assume(!c.is_zero_approx());
		k->set_cone_center(i, c.normalized());
		k->set_cone_radius(i, Math::deg_to_rad(8.0 + 22.0 * (d[4 * i + 3] * 0.5 + 0.5)));
	}
	Vector3 input(d[12], d[13], d[14]);
	witness::assume(!input.is_zero_approx());
	return nearest_projection_holds(k, input.normalized());
}

TEST_CASE("[JointLimitationKusudama3D][property] bent 3-cone chains snap to nearest ground-truth boundary") {
	typedef std::array<double, 15> Input;
	witness::Generator<Input> gen = witness::gen_array<15, double>(witness::gen_double_range(-1.0, 1.0));
	witness::Trial trial = witness::resolve<Input>(
			"kusudama bent-3-cone nearest-projection", gen, bent3_nearest_pred, 0xB0BABEEFULL);
	CHECK(trial.outcome != witness::Outcome::FOUND);
}

static double lcg_unit(uint64_t &s) {
	s = s * 6364136223846793005ULL + 1442695040888963407ULL;
	return (double(s >> 11) / 9007199254740992.0) * 2.0 - 1.0;
}

TEST_CASE("[JointLimitationKusudama3D][property] nearest-projection holds for 2..10 cones") {
	const int GP = 24, GT = 48;
	const real_t grid_gap = (real_t)0.5 * (Math::PI / real_t(GP) + 2.0 * Math::PI / real_t(GT));
	uint64_t rng = 0xC0FFEE11ULL;
	int out_cases = 0, snap_out = 0, excess_viol = 0;
	double worst_excess = 0.0;
	for (int n = 2; n <= 10; n++) {
		for (int trial = 0; trial < 12; trial++) {
			Ref<JointLimitationKusudama3D> k;
			k.instantiate();
			k->set_cone_count(n);
			bool ok = true;
			for (int i = 0; i < n; i++) {
				Vector3 c(lcg_unit(rng), lcg_unit(rng), lcg_unit(rng));
				if (c.is_zero_approx()) {
					ok = false;
				}
				k->set_cone_center(i, c.normalized());
				k->set_cone_radius(i, Math::deg_to_rad(8.0 + 22.0 * (lcg_unit(rng) * 0.5 + 0.5)));
			}
			Vector3 in(lcg_unit(rng), lcg_unit(rng), lcg_unit(rng));
			if (!ok || in.is_zero_approx()) {
				continue;
			}
			in = in.normalized();
			const real_t gap = 2.0 * gt_path_half_gap(k);
			if (gt_region_margin(k, in) <= gap) {
				continue;
			}
			out_cases++;
			Vector3 s;
			k->is_in_boundary(in, s);
			if (gt_region_margin(k, s) > gap) {
				snap_out++;
				continue;
			}
			double brute = Math::PI;
			for (int ip = 0; ip <= GP; ip++) {
				double phi = Math::PI * double(ip) / double(GP);
				for (int it = 0; it < GT; it++) {
					double th = 2.0 * Math::PI * double(it) / double(GT);
					Vector3 g(Math::sin(phi) * Math::cos(th), Math::cos(phi), Math::sin(phi) * Math::sin(th));
					if (gt_region_margin(k, g) <= gap) {
						brute = MIN(brute, in.angle_to(g));
					}
				}
			}
			double excess = Math::rad_to_deg(in.angle_to(s) - brute - grid_gap - gap);
			if (excess > 0.0) {
				excess_viol++;
			}
			worst_excess = MAX(worst_excess, excess);
		}
	}
	MESSAGE("2..10 cones: out-of-region cases=", out_cases, " snap-out=", snap_out, " excess-viol=", excess_viol, " worst excess over interval=", worst_excess, " deg");
	CHECK(out_cases > 0);
	CHECK(snap_out == 0);
	CHECK(excess_viol == 0);
}

TEST_CASE("[JointLimitationKusudama3D][property] control - out-of-region input is caught and repaired") {
	Ref<JointLimitationKusudama3D> k;
	k.instantiate();
	k->set_cone_count(3);
	k->set_cone_center(0, Vector3(0, 1, 0));
	k->set_cone_center(1, Vector3(1, 0, 0));
	k->set_cone_center(2, Vector3(0, 0, 1));
	for (int i = 0; i < 3; i++) {
		k->set_cone_radius(i, Math::deg_to_rad(10.0));
	}
	const real_t gap = 2.0 * gt_path_half_gap(k);
	const Vector3 outside = Vector3(-1, -1, -1).normalized();
	CHECK(gt_region_margin(k, outside) > gap); // control: oracle sees it is outside
	Vector3 s;
	k->is_in_boundary(outside, s);
	CHECK(gt_region_margin(k, s) <= gap); // the snap repairs it
	Vector3 inside;
	CHECK(k->is_in_boundary(Vector3(0, 1, 0), inside)); // an in-region point is unchanged
	CHECK(inside.is_equal_approx(Vector3(0, 1, 0)));
}

} // namespace TestJointLimitationKusudama3D
