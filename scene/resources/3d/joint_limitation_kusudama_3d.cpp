/**************************************************************************/

#include "joint_limitation_kusudama_3d.h"

#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/variant/variant.h"

namespace {

#ifdef TOOLS_ENABLED

using SubdivideCallback = void (*)(const Vector3 &, const Vector3 &, const Vector3 &, int, void *);

struct SubdivideSegmentContext {
	LocalVector<Pair<Vector3, Vector3>> *ret;
};

struct SubdivideTriContext {
	LocalVector<Vector3> *r_triangles;
};

static void subdivide_segment_implementation(const Vector3 &a, const Vector3 &b, const Vector3 &c, int depth, void *p_ctx) {
	SubdivideSegmentContext *ctx = static_cast<SubdivideSegmentContext *>(p_ctx);
	if (depth <= 0) {
		ctx->ret->push_back(Pair<Vector3, Vector3>(a, b));
		ctx->ret->push_back(Pair<Vector3, Vector3>(b, c));
		ctx->ret->push_back(Pair<Vector3, Vector3>(c, a));
		return;
	}
	Vector3 ab = a.lerp(b, (real_t)0.5).normalized();
	Vector3 bc = b.lerp(c, (real_t)0.5).normalized();
	Vector3 ca = c.lerp(a, (real_t)0.5).normalized();
	subdivide_segment_implementation(a, ab, ca, depth - 1, p_ctx);
	subdivide_segment_implementation(b, bc, ab, depth - 1, p_ctx);
	subdivide_segment_implementation(c, ca, bc, depth - 1, p_ctx);
	subdivide_segment_implementation(ab, bc, ca, depth - 1, p_ctx);
}

static void subdivide_tri_implementation(const Vector3 &a, const Vector3 &b, const Vector3 &c, int depth, void *p_ctx) {
	SubdivideTriContext *ctx = static_cast<SubdivideTriContext *>(p_ctx);
	if (depth <= 0) {
		ctx->r_triangles->push_back(a);
		ctx->r_triangles->push_back(b);
		ctx->r_triangles->push_back(c);
		return;
	}
	Vector3 ab = a.lerp(b, (real_t)0.5).normalized();
	Vector3 bc = b.lerp(c, (real_t)0.5).normalized();
	Vector3 ca = c.lerp(a, (real_t)0.5).normalized();
	subdivide_tri_implementation(a, ab, ca, depth - 1, p_ctx);
	subdivide_tri_implementation(b, bc, ab, depth - 1, p_ctx);
	subdivide_tri_implementation(c, ca, bc, depth - 1, p_ctx);
	subdivide_tri_implementation(ab, bc, ca, depth - 1, p_ctx);
}

#endif // TOOLS_ENABLED

} // namespace

void JointLimitationKusudama3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cones", "cones"), &JointLimitationKusudama3D::set_cones);
	ClassDB::bind_method(D_METHOD("get_cones"), &JointLimitationKusudama3D::get_cones);

	ClassDB::bind_method(D_METHOD("set_cone_count", "count"), &JointLimitationKusudama3D::set_cone_count);
	ClassDB::bind_method(D_METHOD("get_cone_count"), &JointLimitationKusudama3D::get_cone_count);
	ClassDB::bind_method(D_METHOD("set_cone_center", "index", "center"), &JointLimitationKusudama3D::set_cone_center);
	ClassDB::bind_method(D_METHOD("get_cone_center", "index"), &JointLimitationKusudama3D::get_cone_center);
	ClassDB::bind_method(D_METHOD("set_cone_radius", "index", "radius"), &JointLimitationKusudama3D::set_cone_radius);
	ClassDB::bind_method(D_METHOD("get_cone_radius", "index"), &JointLimitationKusudama3D::get_cone_radius);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "cones", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_cones", "get_cones");
}

void JointLimitationKusudama3D::set_cones(const Vector<Vector4> &p_cones) {
	cones = p_cones;
	emit_changed();
}

Vector<Vector4> JointLimitationKusudama3D::get_cones() const {
	return cones;
}

void JointLimitationKusudama3D::set_cone_count(int p_count) {
	if (p_count < 0) {
		p_count = 0;
	}
	int old_size = cones.size();
	if (old_size == p_count) {
		return;
	}
	cones.resize(p_count);
	for (int i = old_size; i < cones.size(); i++) {
		cones.write[i] = Vector4(0, 1, 0, Math::PI * 0.25); // Default: +Y axis (normalized), 45 degree cone
	}
	notify_property_list_changed();
	emit_changed();
}

int JointLimitationKusudama3D::get_cone_count() const {
	return cones.size();
}

void JointLimitationKusudama3D::set_cone_center(int p_index, const Vector3 &p_center) {
	ERR_FAIL_INDEX(p_index, cones.size());
	Vector3 normalized_center = p_center;
	if (!normalized_center.is_zero_approx()) {
		normalized_center.normalize();
	} else {
		normalized_center = Vector3::UP; // Default fallback
	}
	Vector4 &cone = cones.write[p_index];
	cone.x = normalized_center.x;
	cone.y = normalized_center.y;
	cone.z = normalized_center.z;
	emit_changed();
}

Vector3 JointLimitationKusudama3D::get_cone_center(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, cones.size(), Vector3::UP);
	const Vector4 &cone_data = cones[p_index];
	return Vector3(cone_data.x, cone_data.y, cone_data.z);
}

void JointLimitationKusudama3D::set_cone_radius(int p_index, real_t p_radius) {
	ERR_FAIL_INDEX(p_index, cones.size());
	cones.write[p_index].w = p_radius;
	emit_changed();
}

real_t JointLimitationKusudama3D::get_cone_radius(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, cones.size(), 0.0);
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
	p_list->push_back(PropertyInfo(Variant::INT, PNAME("cone_count"), PROPERTY_HINT_RANGE, "0,16384,1,or_greater", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_ARRAY, "Cones," + String(PNAME("cones")) + "/"));
	for (int i = 0; i < get_cone_count(); i++) {
		const String prefix = vformat("%s/%d/", PNAME("cones"), i);
		p_list->push_back(PropertyInfo(Variant::VECTOR3, prefix + PNAME("center"), PROPERTY_HINT_NONE, ""));
		p_list->push_back(PropertyInfo(Variant::FLOAT, prefix + PNAME("radius"), PROPERTY_HINT_RANGE, "0,180,0.1,radians_as_degrees"));
	}
}

static const int SWEEP_STEPS = 64;

static Vector3 project_on_cone_boundary(const Vector3 &p_point, const Vector3 &p_center, real_t p_radius) {
	Vector3 center = p_center.normalized();
	Vector3 projected = p_point - center * p_point.dot(center);
	if (projected.is_zero_approx()) {
		projected = center.cross(Vector3::UP);
		if (projected.is_zero_approx()) {
			projected = center.cross(Vector3::RIGHT);
		}
	}
	projected.normalize();
	return (center * Math::cos(p_radius) + projected * Math::sin(p_radius)).normalized();
}

Vector3 JointLimitationKusudama3D::_solve(const Vector3 &p_direction) const {
	Vector3 result = p_direction.normalized();
	if (cones.is_empty()) {
		return result;
	}

	// Region is the union of the cones swept along the interpolated path; one
	// sweep gives both containment and the nearest boundary, so they agree.
	real_t closest_distance = INFINITY;
	Vector3 closest_point = result;

	for (int i = 0; i < cones.size(); i++) {
		const Vector4 &cone_data = cones[i];
		Vector3 center = Vector3(cone_data.x, cone_data.y, cone_data.z);
		real_t radius = cone_data.w;
		if (is_point_in_cone(result, center, radius)) {
			return result;
		}
		Vector3 boundary_point = project_on_cone_boundary(result, center, radius);
		real_t distance = result.distance_to(boundary_point);
		if (distance < closest_distance) {
			closest_distance = distance;
			closest_point = boundary_point;
		}
	}

	for (int i = 0; i + 1 < cones.size(); i++) {
		Vector3 center1 = Vector3(cones[i].x, cones[i].y, cones[i].z).normalized();
		Vector3 center2 = Vector3(cones[i + 1].x, cones[i + 1].y, cones[i + 1].z).normalized();
		real_t radius1 = cones[i].w;
		real_t radius2 = cones[i + 1].w;
		for (int s = 1; s < SWEEP_STEPS; s++) {
			real_t t = real_t(s) / real_t(SWEEP_STEPS);
			Vector3 center = center1.slerp(center2, t).normalized();
			real_t radius = radius1 + (radius2 - radius1) * t;
			if (is_point_in_cone(result, center, radius)) {
				return result;
			}
			Vector3 boundary_point = project_on_cone_boundary(result, center, radius);
			real_t distance = result.distance_to(boundary_point);
			if (distance < closest_distance) {
				closest_distance = distance;
				closest_point = boundary_point;
			}
		}
	}

	return closest_point;
}


#ifdef TOOLS_ENABLED
void JointLimitationKusudama3D::draw_shape(Ref<SurfaceTool> p_surface_tool, const Transform3D &p_transform, float p_bone_length, const Color &p_color, int p_bone_index, Ref<SurfaceTool> p_fill_surface_tool) const {
	real_t sphere_r = p_bone_length * (real_t)0.25;
	if (Math::is_zero_approx(sphere_r)) {
		return;
	}

	LocalVector<Segment> icosahedron_lines = get_icosahedron_sphere(3);
	LocalVector<Vector3> crossed_points;

	if (!cones.is_empty()) {
		icosahedron_lines = cull_lines_by_boundary(icosahedron_lines, crossed_points);
		crossed_points = sort_by_nearest_point(crossed_points);
	}

	p_surface_tool->set_color(p_color);
	if (!crossed_points.is_empty()) {
		for (uint32_t i = 0; i < crossed_points.size(); i++) {
			p_surface_tool->add_vertex(p_transform.xform(crossed_points[i] * sphere_r));
			p_surface_tool->add_vertex(p_transform.xform(crossed_points[(i + 1) % crossed_points.size()] * sphere_r));
		}
	}

	if (p_fill_surface_tool.is_valid() && !cones.is_empty()) {
		LocalVector<Vector3> triangles;
		get_icosahedron_triangles(3, triangles);
		Color fill_color = p_color;
		fill_color.a *= (real_t)0.4;

		for (uint32_t i = 0; i < triangles.size(); i += 3) {
			Vector3 a = triangles[i];
			Vector3 b = triangles[i + 1];
			Vector3 c = triangles[i + 2];
			Vector3 centroid = (a + b + c) / (real_t)3.0;
			centroid.normalize();
			Vector3 solved = _solve(centroid);
			if (solved.is_equal_approx(centroid)) {
				continue; // Skip allowed region; add only impossible region.
			}
			Vector3 na = a.normalized();
			Vector3 nb = b.normalized();
			Vector3 nc = c.normalized();
			p_fill_surface_tool->set_normal(p_transform.basis.xform(na));
			p_fill_surface_tool->set_color(fill_color);
			p_fill_surface_tool->add_vertex(p_transform.xform(a * sphere_r));
			p_fill_surface_tool->set_normal(p_transform.basis.xform(nb));
			p_fill_surface_tool->set_color(fill_color);
			p_fill_surface_tool->add_vertex(p_transform.xform(b * sphere_r));
			p_fill_surface_tool->set_normal(p_transform.basis.xform(nc));
			p_fill_surface_tool->set_color(fill_color);
			p_fill_surface_tool->add_vertex(p_transform.xform(c * sphere_r));
		}
	}
}

LocalVector<JointLimitationKusudama3D::Segment> JointLimitationKusudama3D::get_icosahedron_sphere(int p_subdiv) const {
	LocalVector<Segment> ret;

	if (p_subdiv < 0) {
		p_subdiv = 0;
	}

	const real_t phi = ((real_t)1.0 + Math::sqrt((real_t)5.0)) * (real_t)0.5;
	Vector3 v[12] = {
		Vector3(-1, phi, 0),
		Vector3(1, phi, 0),
		Vector3(-1, -phi, 0),
		Vector3(1, -phi, 0),
		Vector3(0, -1, phi),
		Vector3(0, 1, phi),
		Vector3(0, -1, -phi),
		Vector3(0, 1, -phi),
		Vector3(phi, 0, -1),
		Vector3(phi, 0, 1),
		Vector3(-phi, 0, -1),
		Vector3(-phi, 0, 1)
	};
	for (int i = 0; i < 12; i++) {
		v[i].normalize();
	}

	static const int faces[20][3] = {
		{ 0, 11, 5 },
		{ 0, 5, 1 },
		{ 0, 1, 7 },
		{ 0, 7, 10 },
		{ 0, 10, 11 },
		{ 1, 5, 9 },
		{ 5, 11, 4 },
		{ 11, 10, 2 },
		{ 10, 7, 6 },
		{ 7, 1, 8 },
		{ 3, 9, 4 },
		{ 3, 4, 2 },
		{ 3, 2, 6 },
		{ 3, 6, 8 },
		{ 3, 8, 9 },
		{ 4, 9, 5 },
		{ 2, 4, 11 },
		{ 6, 2, 10 },
		{ 8, 6, 7 },
		{ 9, 8, 1 }
	};

	SubdivideSegmentContext seg_ctx = { &ret };
	SubdivideCallback subdivide_callback = &subdivide_segment_implementation;
	for (int f = 0; f < 20; f++) {
		const Vector3 &a = v[faces[f][0]];
		const Vector3 &b = v[faces[f][1]];
		const Vector3 &c = v[faces[f][2]];
		subdivide_callback(a, b, c, p_subdiv, &seg_ctx);
	}

	for (uint32_t i = 0; i < ret.size(); i++) {
		if (ret[i].second < ret[i].first) {
			SWAP(ret[i].first, ret[i].second);
		}
	}
	ret.sort();
	uint32_t write = 0;
	for (uint32_t i = 0; i < ret.size(); i++) {
		if (write == 0 || ret[write - 1] != ret[i]) {
			ret[write++] = ret[i];
		}
	}
	ret.resize(write);

	return ret;
}

void JointLimitationKusudama3D::get_icosahedron_triangles(int p_subdiv, LocalVector<Vector3> &r_triangles) const {
	r_triangles.clear();
	if (p_subdiv < 0) {
		p_subdiv = 0;
	}

	const real_t phi = ((real_t)1.0 + Math::sqrt((real_t)5.0)) * (real_t)0.5;
	Vector3 v[12] = {
		Vector3(-1, phi, 0),
		Vector3(1, phi, 0),
		Vector3(-1, -phi, 0),
		Vector3(1, -phi, 0),
		Vector3(0, -1, phi),
		Vector3(0, 1, phi),
		Vector3(0, -1, -phi),
		Vector3(0, 1, -phi),
		Vector3(phi, 0, -1),
		Vector3(phi, 0, 1),
		Vector3(-phi, 0, -1),
		Vector3(-phi, 0, 1)
	};
	for (int i = 0; i < 12; i++) {
		v[i].normalize();
	}

	static const int faces[20][3] = {
		{ 0, 11, 5 },
		{ 0, 5, 1 },
		{ 0, 1, 7 },
		{ 0, 7, 10 },
		{ 0, 10, 11 },
		{ 1, 5, 9 },
		{ 5, 11, 4 },
		{ 11, 10, 2 },
		{ 10, 7, 6 },
		{ 7, 1, 8 },
		{ 3, 9, 4 },
		{ 3, 4, 2 },
		{ 3, 2, 6 },
		{ 3, 6, 8 },
		{ 3, 8, 9 },
		{ 4, 9, 5 },
		{ 2, 4, 11 },
		{ 6, 2, 10 },
		{ 8, 6, 7 },
		{ 9, 8, 1 }
	};

	SubdivideTriContext tri_ctx = { &r_triangles };
	SubdivideCallback subdivide_tri_callback = &subdivide_tri_implementation;
	for (int f = 0; f < 20; f++) {
		const Vector3 &a = v[faces[f][0]];
		const Vector3 &b = v[faces[f][1]];
		const Vector3 &c = v[faces[f][2]];
		subdivide_tri_callback(a, b, c, p_subdiv, &tri_ctx);
	}
}

LocalVector<JointLimitationKusudama3D::Segment> JointLimitationKusudama3D::cull_lines_by_boundary(const LocalVector<Segment> &p_segments, LocalVector<Vector3> &r_crossed_points) const {
	LocalVector<Segment> ret;
	for (const Segment &seg : p_segments) {
		Vector3 from_solved;
		bool from_is_in_boundary = is_in_boundary(seg.first, from_solved);
		Vector3 to_solved;
		bool to_is_in_boundary = is_in_boundary(seg.second, to_solved);
		if (from_is_in_boundary && to_is_in_boundary) {
			continue;
		} else if (!from_is_in_boundary && !to_is_in_boundary) {
			ret.push_back(seg);
		} else {
			Segment new_seg;
			if (from_is_in_boundary) {
				new_seg.first = seg.second;
				new_seg.second = to_solved;
				r_crossed_points.push_back(to_solved);
			} else {
				new_seg.first = from_solved;
				new_seg.second = seg.first;
				r_crossed_points.push_back(from_solved);
			}
			ret.push_back(new_seg);
		}
	}
	return ret;
}

bool JointLimitationKusudama3D::is_in_boundary(const Vector3 &p_point, Vector3 &r_solved) const {
	r_solved = _solve(p_point);
	return r_solved.is_equal_approx(p_point);
}

LocalVector<Vector3> JointLimitationKusudama3D::sort_by_nearest_point(const LocalVector<Vector3> &p_points) const {
	LocalVector<Vector3> ret;
	LocalVector<Vector3> points(p_points);
	if (!points.is_empty()) {
		ret.push_back(points[0]);
		points.remove_at(0);
		while (!points.is_empty()) {
			uint32_t current = ret.size() - 1;
			int nearest_index = -1;
			double nearest = INFINITY;
			for (uint32_t i = 0; i < points.size(); i++) {
				double dist = ret[current].distance_squared_to(points[i]);
				if (dist < nearest) {
					nearest = dist;
					nearest_index = i;
				}
			}
			if (nearest_index >= 0) {
				ret.push_back(points[nearest_index]);
				points.remove_at(nearest_index);
			}
		}
	}
	return ret;
}

#endif // TOOLS_ENABLED

bool JointLimitationKusudama3D::is_point_in_cone(const Vector3 &p_point, const Vector3 &p_cone_center, real_t p_cone_radius) const {
	if (p_point.is_zero_approx()) {
		return false;
	}
	return p_point.normalized().angle_to(p_cone_center) <= p_cone_radius;
}
