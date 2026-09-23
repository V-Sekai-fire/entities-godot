/**************************************************************************/
/*  cassie_sketch_graph.cpp                                               */
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

#include "cassie_sketch_graph.h"

#include "core/math/aabb.h"
#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/templates/local_vector.h"
#include "core/templates/sort_array.h"

void CassieSketchGraphNode::add_edge_id(int p_edge_id) {
	edge_ids.push_back(p_edge_id);
}

void CassieSketchGraphNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &CassieSketchGraphNode::get_id);
	ClassDB::bind_method(D_METHOD("get_position"), &CassieSketchGraphNode::get_position);
	ClassDB::bind_method(D_METHOD("get_normal"), &CassieSketchGraphNode::get_normal);
	ClassDB::bind_method(D_METHOD("get_edge_ids"), &CassieSketchGraphNode::get_edge_ids);
	ClassDB::bind_method(D_METHOD("get_degree"), &CassieSketchGraphNode::get_degree);
	ClassDB::bind_method(D_METHOD("get_is_sharp"), &CassieSketchGraphNode::get_is_sharp);
}

int CassieSketchGraphEdge::get_opposite(int node_id) const {
	return node_id == node_a_id ? node_b_id : node_a_id;
}

// Segment.GetPointAt: p_u is lerped between the end parameters and the
// polyline is read at that parameter.
Vector3 CassieSketchGraphEdge::get_point_at(real_t p_u) const {
	const int n = points.size();
	if (n == 1 || params.size() != n) {
		return n > 0 ? points[0] : Vector3();
	}
	const real_t t = Math::lerp(real_t(params[0]), real_t(params[n - 1]), CLAMP(p_u, real_t(0), real_t(1)));
	int i = 0;
	while (i < n - 2 && real_t(params[i + 1]) < t) {
		i++;
	}
	const real_t span = real_t(params[i + 1]) - real_t(params[i]);
	const real_t f = span > real_t(0) ? CLAMP((t - real_t(params[i])) / span, real_t(0), real_t(1)) : real_t(0);
	return points[i].lerp(points[i + 1], f);
}

Vector3 CassieSketchGraphEdge::get_tangent_away_from(int node_id) const {
	const int n = points.size();
	if (n < 2) {
		return Vector3();
	}
	if (node_id == node_a_id) {
		return (points[1] - points[0]).normalized();
	}
	return (points[n - 2] - points[n - 1]).normalized();
}

void CassieSketchGraphEdge::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &CassieSketchGraphEdge::get_id);
	ClassDB::bind_method(D_METHOD("get_points"), &CassieSketchGraphEdge::get_points);
	ClassDB::bind_method(D_METHOD("get_normals"), &CassieSketchGraphEdge::get_normals);
	ClassDB::bind_method(D_METHOD("get_node_a_id"), &CassieSketchGraphEdge::get_node_a_id);
	ClassDB::bind_method(D_METHOD("get_node_b_id"), &CassieSketchGraphEdge::get_node_b_id);
	ClassDB::bind_method(D_METHOD("get_opposite", "node_id"),
			&CassieSketchGraphEdge::get_opposite);
	ClassDB::bind_method(D_METHOD("get_source_polyline_idx"),
			&CassieSketchGraphEdge::get_source_polyline_idx);
	ClassDB::bind_method(D_METHOD("get_boundary"), &CassieSketchGraphEdge::get_boundary);
}

// ── Graph implementation ────────────────────────────────────────────────────

void CassieSketchGraph::clear() {
	nodes.clear();
	edges.clear();
	next_node_id = 0;
	next_edge_id = 0;
	live_cycles.clear();
	cycles_by_edge.clear();
	updated_edges.clear();
	cycles_to_check.clear();
	next_cycle_id = 0;
}

int CassieSketchGraph::_find_or_create_node(const Vector3 &p_pos,
		const Vector3 &p_normal) {
	// Linear scan over existing nodes within merge_epsilon. For the
	// modest node counts the editing demo produces (low hundreds) this
	// is fine; the GDScript scaffold has a spatial hash but the C++
	// path stays simple until profiling motivates one.
	for (const KeyValue<int, Ref<CassieSketchGraphNode>> &e : nodes) {
		if (e.value.is_valid() &&
				p_pos.distance_to(e.value->get_position()) <= merge_epsilon) {
			return e.key;
		}
	}
	const int id = next_node_id++;
	Ref<CassieSketchGraphNode> node;
	node.instantiate();
	node->set_id(id);
	node->set_position(p_pos);
	Vector3 n = p_normal;
	if (n.length_squared() > real_t(1e-20)) {
		n = n.normalized();
	} else {
		n = Vector3(0, 1, 0);
	}
	node->set_normal(n);
	nodes.insert(id, node);
	return id;
}

void CassieSketchGraph::_update_node_normal(int p_node_id) {
	HashMap<int, Ref<CassieSketchGraphNode>>::Iterator it = nodes.find(p_node_id);
	if (!it) {
		return;
	}
	Ref<CassieSketchGraphNode> node = it->value;
	const PackedInt32Array eids = node->get_edge_ids();
	// Average per-edge stored normal if any edge supplies one (drawn-with-
	// normals path).
	Vector3 acc;
	for (int i = 0; i < eids.size(); ++i) {
		HashMap<int, Ref<CassieSketchGraphEdge>>::Iterator eit = edges.find(eids[i]);
		if (!eit || eit->value.is_null()) {
			continue;
		}
		const PackedVector3Array nrms = eit->value->get_normals();
		if (nrms.is_empty()) {
			continue;
		}
		acc += eit->value->get_node_a_id() == p_node_id
				? nrms[0]
				: nrms[nrms.size() - 1];
	}
	if (acc.length_squared() > real_t(1e-10)) {
		node->set_normal(acc.normalized());
		return;
	}
	// Fallback for the raw-input path (no per-edge normals): best-fit plane
	// normal from the incident edge tangents. The tangent plane at a
	// curve-network vertex spans the plane that contains all incident
	// edges' departing tangents; its normal is what the cycle walk
	// needs for a coherent angular sort. Yu 2021 §5.1 ("we obtain this
	// normal estimate at each intersection by assuming that the
	// intersecting curves lie on a smooth surface, whose tangent plane
	// is spanned by the curve tangents"). Build the 3×3 covariance of
	// tangents, take the eigenvector with the smallest eigenvalue.
	LocalVector<Vector3> tangents;
	for (int i = 0; i < eids.size(); ++i) {
		HashMap<int, Ref<CassieSketchGraphEdge>>::Iterator eit = edges.find(eids[i]);
		if (!eit || eit->value.is_null()) {
			continue;
		}
		const Vector3 t = eit->value->get_tangent_away_from(p_node_id);
		if (t.length_squared() > real_t(1e-10)) {
			tangents.push_back(t);
		}
	}
	if (tangents.size() < 2) {
		return;
	}
	// 3×3 covariance Σ tᵢ tᵢᵀ — laid out as 6 distinct entries.
	real_t cxx = 0, cyy = 0, czz = 0, cxy = 0, cxz = 0, cyz = 0;
	for (uint32_t i = 0; i < tangents.size(); ++i) {
		const Vector3 t = tangents[i];
		cxx += t.x * t.x;
		cyy += t.y * t.y;
		czz += t.z * t.z;
		cxy += t.x * t.y;
		cxz += t.x * t.z;
		cyz += t.y * t.z;
	}
	// Smallest-eigenvector via inverse-power iteration (a few sweeps).
	// Seed: cross of first two non-parallel tangents (good initial guess
	// of the plane normal direction). If degenerate, fall back to (0,1,0).
	Vector3 seed(0, 1, 0);
	for (uint32_t i = 1; i < tangents.size(); ++i) {
		const Vector3 c = tangents[0].cross(tangents[i]);
		if (c.length_squared() > real_t(1e-10)) {
			seed = c.normalized();
			break;
		}
	}
	// Project tangents out of seed: v ← seed − (covariance · seed).
	// For tangent plane, normal n satisfies Σ (tᵢ·n)² minimal — i.e. n
	// is the smallest-eigenvalue eigenvector of the covariance. One
	// step of explicit power-iteration on (I·trace − C) converges fast.
	Vector3 n = seed;
	const real_t trace = cxx + cyy + czz;
	for (int iter = 0; iter < 4; ++iter) {
		// Apply (trace·I − C) to n.
		const Vector3 cn(
				cxx * n.x + cxy * n.y + cxz * n.z,
				cxy * n.x + cyy * n.y + cyz * n.z,
				cxz * n.x + cyz * n.y + czz * n.z);
		Vector3 r = n * trace - cn;
		const real_t r2 = r.length_squared();
		if (r2 < real_t(1e-20)) {
			break;
		}
		n = r / Math::sqrt(r2);
	}
	if (n.length_squared() > real_t(1e-10)) {
		node->set_normal(n.normalized());
	}
}

void CassieSketchGraph::_update_node_sharpness(int p_node_id) {
	HashMap<int, Ref<CassieSketchGraphNode>>::Iterator it = nodes.find(p_node_id);
	if (!it) {
		return;
	}
	Ref<CassieSketchGraphNode> node = it->value;
	const int deg = node->get_degree();
	if (deg != 2) {
		node->set_is_sharp(deg > 2);
		return;
	}
	const PackedInt32Array eids = node->get_edge_ids();
	HashMap<int, Ref<CassieSketchGraphEdge>>::Iterator e0it = edges.find(eids[0]);
	HashMap<int, Ref<CassieSketchGraphEdge>>::Iterator e1it = edges.find(eids[1]);
	if (!e0it || !e1it) {
		node->set_is_sharp(false);
		return;
	}
	const Vector3 t0 = e0it->value->get_tangent_away_from(p_node_id);
	const Vector3 t1 = e1it->value->get_tangent_away_from(p_node_id);
	const real_t cosang = CLAMP(t0.dot(t1), real_t(-1.0), real_t(1.0));
	const real_t angle = Math::acos(cosang);
	// The two tangents both point AWAY from the node, so they're at the
	// supplementary of the curve's turning angle. A truly sharp corner
	// (small turning angle along the curve) gives `angle` close to π, a
	// smooth continuation gives `angle` close to π too. The simpler
	// reading: "sharp" iff the angular gap deviates from π by more than
	// the threshold (i.e., one tangent is not nearly opposite the other).
	node->set_is_sharp(Math::abs(real_t(Math::PI) - angle) > sharp_angle_threshold);
}

// Segment-segment closest pair in 3D. Returns the parameters s, t in [0, 1]
// on segments (a0→a1) and (b0→b1) of the closest points, plus the squared
// distance between them. Standard clamped-projection routine.
static void _segment_segment_closest(const Vector3 &a0, const Vector3 &a1,
		const Vector3 &b0, const Vector3 &b1, real_t &out_s, real_t &out_t,
		real_t &out_dist2) {
	const Vector3 d1 = a1 - a0;
	const Vector3 d2 = b1 - b0;
	const Vector3 r = a0 - b0;
	const real_t a = d1.dot(d1);
	const real_t e = d2.dot(d2);
	const real_t f = d2.dot(r);
	real_t s = 0;
	real_t t = 0;
	const real_t kEps = real_t(1e-20);
	if (a <= kEps && e <= kEps) {
		s = 0;
		t = 0;
	} else if (a <= kEps) {
		s = 0;
		t = CLAMP(f / e, real_t(0), real_t(1));
	} else {
		const real_t c = d1.dot(r);
		if (e <= kEps) {
			t = 0;
			s = CLAMP(-c / a, real_t(0), real_t(1));
		} else {
			const real_t b = d1.dot(d2);
			const real_t denom = a * e - b * b;
			if (denom > kEps) {
				s = CLAMP((b * f - c * e) / denom, real_t(0), real_t(1));
			} else {
				s = 0;
			}
			t = (b * s + f) / e;
			if (t < 0) {
				t = 0;
				s = CLAMP(-c / a, real_t(0), real_t(1));
			} else if (t > 1) {
				t = 1;
				s = CLAMP((b - c) / a, real_t(0), real_t(1));
			}
		}
	}
	out_s = s;
	out_t = t;
	const Vector3 cp_a = a0 + d1 * s;
	const Vector3 cp_b = b0 + d2 * t;
	out_dist2 = cp_a.distance_squared_to(cp_b);
}

int CassieSketchGraph::add_stroke(const PackedVector3Array &p_points,
		const PackedVector3Array &p_normals) {
	if (p_points.size() < 2) {
		return -1;
	}
	const Vector3 pa = p_points[0];
	const Vector3 pb = p_points[p_points.size() - 1];
	const Vector3 na = p_normals.is_empty() ? Vector3(0, 1, 0) : p_normals[0];
	const Vector3 nb = p_normals.is_empty() ? Vector3(0, 1, 0)
											: p_normals[p_normals.size() - 1];

	const int node_a = _find_or_create_node(pa, na);
	const int node_b = _find_or_create_node(pb, nb);

	const int eid = next_edge_id++;
	Ref<CassieSketchGraphEdge> edge;
	edge.instantiate();
	edge->set_id(eid);
	edge->set_points(p_points);
	edge->set_params(_uniform_params(p_points.size()));
	edge->set_normals(p_normals);
	edge->set_node_a_id(node_a);
	edge->set_node_b_id(node_b);
	edges.insert(eid, edge);

	nodes[node_a]->add_edge_id(eid);
	nodes[node_b]->add_edge_id(eid);

	_update_node_sharpness(node_a);
	_update_node_sharpness(node_b);
	_update_node_normal(node_a);
	_update_node_normal(node_b);
	return eid;
}

void CassieSketchGraph::_remove_edge(int p_edge_id) {
	HashMap<int, Ref<CassieSketchGraphEdge>>::Iterator it = edges.find(p_edge_id);
	if (!it) {
		return;
	}
	const int ends[2] = { it->value->get_node_a_id(), it->value->get_node_b_id() };
	for (int e = 0; e < 2; ++e) {
		HashMap<int, Ref<CassieSketchGraphNode>>::Iterator nit = nodes.find(ends[e]);
		if (!nit) {
			continue;
		}
		PackedInt32Array ids = nit->value->get_edge_ids();
		const int at = ids.find(p_edge_id);
		if (at >= 0) {
			ids.remove_at(at);
		}
		nit->value->set_edge_ids(ids);
	}
	edges.remove(it);
}

void CassieSketchGraph::_cumulative_lengths(const PackedVector3Array &p_poly,
		LocalVector<real_t> &r_cum) {
	r_cum.resize(p_poly.size());
	if (p_poly.is_empty()) {
		return;
	}
	r_cum[0] = 0;
	for (int k = 1; k < p_poly.size(); ++k) {
		r_cum[k] = r_cum[k - 1] + p_poly[k - 1].distance_to(p_poly[k]);
	}
}

// the stretch beside a knot two polylines share. p_a's
// end p_a_end (0 start, 1 end) and p_b's end p_b_end meet within
// merge_epsilon, so add_stroke merges them into one node; leaving it, the
// two stay within p_proximity of each other for a while (up to p_proximity
// along each when they leave in opposite directions, farther as the angle
// between them closes). The zone grows segment by segment from the knot on
// both polylines together, as long as each next segment stays within
// p_proximity of the other's zone (or its next segment). r_la / r_lb count
// the zone's segments from that end.
static void _knot_zone(const PackedVector3Array &p_a, int p_a_end,
		const PackedVector3Array &p_b, int p_b_end, real_t p_prox2,
		int &r_la, int &r_lb) {
	const int sa = p_a.size() - 1;
	const int sb = p_b.size() - 1;
	auto seg = [](const PackedVector3Array &p, int s, int end, int k, Vector3 &r0, Vector3 &r1) {
		r0 = end == 0 ? p[k] : p[s - k];
		r1 = end == 0 ? p[k + 1] : p[s - k - 1];
	};
	// Segment k of one polyline within p_proximity of any of the other's
	// first `upto` segments.
	auto within_other = [&](const PackedVector3Array &p, int s, int end, int k,
						const PackedVector3Array &q, int t, int qend, int upto) {
		Vector3 a0, a1, b0, b1;
		seg(p, s, end, k, a0, a1);
		for (int j = 0; j < MIN(upto, t); ++j) {
			seg(q, t, qend, j, b0, b1);
			real_t u, v, d2;
			_segment_segment_closest(a0, a1, b0, b1, u, v, d2);
			if (d2 <= p_prox2) {
				return true;
			}
		}
		return false;
	};
	r_la = 1;
	r_lb = 1;
	for (;;) {
		bool grown = false;
		if (r_la < sa && within_other(p_a, sa, p_a_end, r_la, p_b, sb, p_b_end, r_lb + 1)) {
			++r_la;
			grown = true;
		}
		if (r_lb < sb && within_other(p_b, sb, p_b_end, r_lb, p_a, sa, p_a_end, r_la + 1)) {
			++r_lb;
			grown = true;
		}
		if (!grown) {
			break;
		}
	}
}

// Pairwise segment-segment closest-pair tests between two polylines with a
// per-polyline and a per-segment AABB cull. Each hit within p_proximity is
// recorded on both polylines as the midpoint of the closest pair, so the
// crossing is one position from both points of view; merge_epsilon then
// collapses it to one graph node.
void CassieSketchGraph::_crossings(const PackedVector3Array &p_a,
		const PackedVector3Array &p_b, real_t p_proximity, real_t p_merge_epsilon,
		LocalVector<SplitPt> &r_a, LocalVector<SplitPt> &r_b) {
	const int na = p_a.size();
	const int nb = p_b.size();
	if (na < 2 || nb < 2) {
		return;
	}
	// a shared endpoint is an endpoint merge wherever
	// the two polylines have not yet parted beside it, not only on their
	// first or last segments (below): with p_proximity at or above the
	// sample spacing, hits on the second and third segments beside a knot
	// were split as crossings, trimming the edges short of the knot or
	// leaving a dangling node a few cm from it.
	struct Zone {
		int a_end, b_end, la, lb;
	};
	LocalVector<Zone> zones;
	for (int ae = 0; ae < 2; ++ae) {
		for (int be = 0; be < 2; ++be) {
			const Vector3 pa = ae == 0 ? p_a[0] : p_a[na - 1];
			const Vector3 pb = be == 0 ? p_b[0] : p_b[nb - 1];
			if (pa.distance_to(pb) > p_merge_epsilon) {
				continue;
			}
			Zone z = { ae, be, 1, 1 };
			_knot_zone(p_a, ae, p_b, be, p_proximity * p_proximity, z.la, z.lb);
			zones.push_back(z);
		}
	}
	auto in_zone = [&](int a, int b) {
		for (uint32_t k = 0; k < zones.size(); ++k) {
			const Zone &z = zones[k];
			const bool ina = z.a_end == 0 ? a < z.la : a >= na - 1 - z.la;
			const bool inb = z.b_end == 0 ? b < z.lb : b >= nb - 1 - z.lb;
			if (ina && inb) {
				return true;
			}
		}
		return false;
	};
	AABB box_a(p_a[0], Vector3());
	for (int k = 1; k < na; ++k) {
		box_a.expand_to(p_a[k]);
	}
	AABB box_b(p_b[0], Vector3());
	for (int k = 1; k < nb; ++k) {
		box_b.expand_to(p_b[k]);
	}
	if (!box_a.grow(p_proximity).intersects(box_b.grow(p_proximity))) {
		return;
	}
	LocalVector<real_t> cum_a;
	LocalVector<real_t> cum_b;
	_cumulative_lengths(p_a, cum_a);
	_cumulative_lengths(p_b, cum_b);
	const real_t prox2 = p_proximity * p_proximity;
	// A shared endpoint is an endpoint merge, not a crossing, and the tube
	// of near-parallel segments around one crossing coalesces to one hit
	// within cluster_eps, both as in the Lean model's findAllSplitsByCubic.
	const real_t cluster_eps = real_t(0.05);
	LocalVector<Vector3> reps;
	for (int a = 0; a < na - 1; ++a) {
		const Vector3 a0 = p_a[a];
		const Vector3 a1 = p_a[a + 1];
		AABB seg_a(a0, Vector3());
		seg_a.expand_to(a1);
		seg_a = seg_a.grow(p_proximity);
		for (int b = 0; b < nb - 1; ++b) {
			const Vector3 b0 = p_b[b];
			const Vector3 b1 = p_b[b + 1];
			AABB seg_b(b0, Vector3());
			seg_b.expand_to(b1);
			if (!seg_a.intersects(seg_b)) {
				continue;
			}
			real_t s, t, d2;
			_segment_segment_closest(a0, a1, b0, b1, s, t, d2);
			if (d2 > prox2) {
				continue;
			}
			const bool at_end_a = (a == 0 && s < real_t(0.05)) || (a == na - 2 && s > real_t(0.95));
			const bool at_end_b = (b == 0 && t < real_t(0.05)) || (b == nb - 2 && t > real_t(0.95));
			if ((at_end_a && at_end_b) || in_zone(a, b)) {
				continue;
			}
			const Vector3 mid = (a0 + (a1 - a0) * s + b0 + (b1 - b0) * t) * real_t(0.5);
			bool clustered = false;
			for (uint32_t r = 0; r < reps.size(); ++r) {
				if (mid.distance_to(reps[r]) < cluster_eps) {
					clustered = true;
					break;
				}
			}
			if (clustered) {
				continue;
			}
			reps.push_back(mid);
			const SplitPt sa = { cum_a[a] + s * (cum_a[a + 1] - cum_a[a]), mid };
			const SplitPt sb = { cum_b[b] + t * (cum_b[b + 1] - cum_b[b]), mid };
			r_a.push_back(sa);
			r_b.push_back(sb);
		}
	}
}

// Sorts p_splits by arc length, drops near-coincident ones, and emits one
// edge per slice of p_poly between consecutive splits. The endpoint merge
// inside add_stroke snaps each slice end to the shared crossing node.
int CassieSketchGraph::_add_polyline_sliced(const PackedVector3Array &p_poly,
		LocalVector<SplitPt> &p_splits, int p_source_idx, bool p_boundary) {
	const int n = p_poly.size();
	if (n < 2) {
		return 0;
	}
	for (uint32_t a = 1; a < p_splits.size(); ++a) {
		for (uint32_t b = a; b > 0; --b) {
			if (p_splits[b].t >= p_splits[b - 1].t) {
				break;
			}
			const SplitPt tmp = p_splits[b];
			p_splits[b] = p_splits[b - 1];
			p_splits[b - 1] = tmp;
		}
	}
	LocalVector<SplitPt> uniq;
	for (uint32_t k = 0; k < p_splits.size(); ++k) {
		if (!uniq.is_empty() &&
				p_splits[k].pos.distance_to(uniq[uniq.size() - 1].pos) <= merge_epsilon) {
			continue;
		}
		uniq.push_back(p_splits[k]);
	}

	LocalVector<real_t> cl;
	_cumulative_lengths(p_poly, cl);
	const PackedVector3Array empty_normals;
	int added = 0;
	PackedVector3Array current;
	current.push_back(p_poly[0]);
	uint32_t s_ix = 0;
	for (int k = 0; k < n - 1; ++k) {
		const real_t seg_end_t = cl[k + 1];
		while (s_ix < uniq.size() && uniq[s_ix].t <= seg_end_t) {
			const Vector3 cut = uniq[s_ix].pos;
			if (current.size() > 0 &&
					cut.distance_to(current[current.size() - 1]) > merge_epsilon) {
				current.push_back(cut);
			}
			if (current.size() >= 2) {
				const int new_eid = add_stroke(current, empty_normals);
				if (new_eid >= 0) {
					edges[new_eid]->set_source_polyline_idx(p_source_idx);
					edges[new_eid]->set_boundary(p_boundary);
					added++;
				}
			}
			current = PackedVector3Array();
			current.push_back(cut);
			s_ix++;
		}
		const Vector3 next = p_poly[k + 1];
		if (current.size() == 0 ||
				next.distance_to(current[current.size() - 1]) > merge_epsilon) {
			current.push_back(next);
		}
	}
	if (current.size() >= 2) {
		const int new_eid = add_stroke(current, empty_normals);
		if (new_eid >= 0) {
			edges[new_eid]->set_source_polyline_idx(p_source_idx);
			edges[new_eid]->set_boundary(p_boundary);
			added++;
		}
	}
	return added;
}

// One-shot planar arrangement build for offline replay. See header comment.
// O(P² × S²) closest-pair tests worst case, P polylines of S segments; the
// AABB culls drop most pairs. Hat (120 polylines × ~5 segs) runs in tens of ms.
int CassieSketchGraph::build_from_polylines(
		const TypedArray<PackedVector3Array> &p_polylines,
		real_t p_proximity) {
	clear();
	const int P = p_polylines.size();
	if (P == 0) {
		return 0;
	}
	LocalVector<PackedVector3Array> polys;
	polys.resize(P);
	for (int i = 0; i < P; ++i) {
		polys[i] = p_polylines[i];
	}
	LocalVector<LocalVector<SplitPt>> splits_per_poly;
	splits_per_poly.resize(P);
	for (int i = 0; i < P; ++i) {
		for (int j = i + 1; j < P; ++j) {
			_crossings(polys[i], polys[j], p_proximity, merge_epsilon,
					splits_per_poly[i], splits_per_poly[j]);
		}
	}
	int total_edges = 0;
	for (int i = 0; i < P; ++i) {
		total_edges += _add_polyline_sliced(polys[i], splits_per_poly[i], i);
	}
	return total_edges;
}

int CassieSketchGraph::add_stroke_intersecting(const PackedVector3Array &p_points,
		const PackedVector3Array &p_normals, real_t p_proximity, bool p_boundary) {
	if (p_points.size() < 2) {
		return 0;
	}
	LocalVector<SplitPt> new_splits;
	LocalVector<int> hit_ids;
	LocalVector<PackedVector3Array> hit_points;
	LocalVector<int> hit_source;
	LocalVector<bool> hit_boundary;
	LocalVector<LocalVector<SplitPt>> hit_splits;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		LocalVector<SplitPt> on_edge;
		_crossings(p_points, kv.value->get_points(), p_proximity, merge_epsilon, new_splits, on_edge);
		if (on_edge.is_empty()) {
			continue;
		}
		hit_ids.push_back(kv.key);
		hit_points.push_back(kv.value->get_points());
		hit_source.push_back(kv.value->get_source_polyline_idx());
		hit_boundary.push_back(kv.value->get_boundary());
		hit_splits.push_back(on_edge);
	}
	int added = 0;
	for (uint32_t h = 0; h < hit_ids.size(); ++h) {
		_remove_edge(hit_ids[h]);
		added += _add_polyline_sliced(hit_points[h], hit_splits[h], hit_source[h], hit_boundary[h]) - 1;
	}
	if (new_splits.is_empty()) {
		const int eid = add_stroke(p_points, p_normals);
		if (eid < 0) {
			return added;
		}
		edges[eid]->set_boundary(p_boundary);
		return added + 1;
	}
	return added + _add_polyline_sliced(p_points, new_splits, -1, p_boundary);
}

bool CassieSketchGraph::is_opening(const PackedInt32Array &p_cycle_edge_ids) const {
	if (p_cycle_edge_ids.is_empty()) {
		return false;
	}
	for (int i = 0; i < p_cycle_edge_ids.size(); ++i) {
		HashMap<int, Ref<CassieSketchGraphEdge>>::ConstIterator it = edges.find(p_cycle_edge_ids[i]);
		if (!it || it->value.is_null() || !it->value->get_boundary()) {
			return false;
		}
	}
	return true;
}

int CassieSketchGraph::_new_node(const Vector3 &p_pos) {
	const int id = next_node_id++;
	Ref<CassieSketchGraphNode> node;
	node.instantiate();
	node->set_id(id);
	node->set_position(p_pos);
	node->set_normal(Vector3(0, 1, 0));
	nodes.insert(id, node);
	return id;
}

PackedFloat32Array CassieSketchGraph::_uniform_params(int p_count) {
	PackedFloat32Array out;
	out.resize(p_count);
	for (int i = 0; i < p_count; ++i) {
		out.set(i, p_count > 1 ? float(i) / float(p_count - 1) : 0.0f);
	}
	return out;
}

int CassieSketchGraph::_new_edge(const PackedVector3Array &p_points, int p_node_a,
		int p_node_b, int p_source_idx, bool p_on_new_stroke, const PackedFloat32Array &p_params) {
	const int eid = next_edge_id++;
	if (p_on_new_stroke) {
		updated_edges.insert(eid);
	}
	Ref<CassieSketchGraphEdge> edge;
	edge.instantiate();
	edge->set_id(eid);
	edge->set_points(p_points);
	edge->set_params(p_params.size() == p_points.size() ? p_params : _uniform_params(p_points.size()));
	edge->set_node_a_id(p_node_a);
	edge->set_node_b_id(p_node_b);
	edge->set_source_polyline_idx(p_source_idx);
	edges.insert(eid, edge);
	nodes[p_node_a]->add_edge_id(eid);
	nodes[p_node_b]->add_edge_id(eid);
	_update_node_sharpness(p_node_a);
	_update_node_sharpness(p_node_b);
	_update_node_normal(p_node_a);
	_update_node_normal(p_node_b);
	return eid;
}

real_t CassieSketchGraph::_closest_on_polyline(const PackedVector3Array &p_poly,
		const Vector3 &p_pos, SplitPt &r_hit) {
	real_t best = Math::INF;
	real_t cum = 0;
	for (int k = 0; k + 1 < p_poly.size(); ++k) {
		const Vector3 a = p_poly[k];
		const Vector3 b = p_poly[k + 1];
		const Vector3 d = b - a;
		const real_t len2 = d.length_squared();
		const real_t s = len2 > real_t(1e-20) ? CLAMP((p_pos - a).dot(d) / len2, real_t(0), real_t(1)) : real_t(0);
		const Vector3 q = a + d * s;
		const real_t dist = q.distance_to(p_pos);
		if (dist < best) {
			best = dist;
			r_hit.t = cum + s * Math::sqrt(len2);
			r_hit.pos = q;
			r_hit.dir = d.normalized();
		}
		cum += Math::sqrt(len2);
	}
	return best;
}

int CassieSketchGraph::_nearest_edge(const Vector3 &p_pos, int p_source_idx,
		bool p_own, int p_target, const Vector3 &p_dir, SplitPt &r_hit) const {
	int best_eid = -1;
	int best_source = -1;
	real_t best = Math::INF;
	real_t best_sin = -1;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		const int source = kv.value->get_source_polyline_idx();
		if ((source == p_source_idx) != p_own || (p_target >= 0 && source != p_target)) {
			continue;
		}
		SplitPt hit;
		const real_t dist = _closest_on_polyline(kv.value->get_points(), p_pos, hit);
		const real_t sin = hit.dir.cross(p_dir).length();
		const bool closer = dist < best - real_t(1e-7);
		const bool tie = Math::abs(dist - best) <= real_t(1e-7);
		const bool steeper = sin > best_sin + real_t(1e-6);
		const bool later = Math::abs(sin - best_sin) <= real_t(1e-6) && source > best_source;
		if (closer || (tie && (steeper || later))) {
			best = dist;
			best_eid = kv.key;
			best_source = source;
			best_sin = sin;
			r_hit = hit;
		}
	}
	return best_eid;
}

int CassieSketchGraph::_closer_end(int p_edge_id, const Vector3 &p_pos) const {
	const Ref<CassieSketchGraphEdge> e = edges[p_edge_id];
	const real_t da = nodes[e->get_node_a_id()]->get_position().distance_to(p_pos);
	const real_t db = nodes[e->get_node_b_id()]->get_position().distance_to(p_pos);
	return da <= db ? e->get_node_a_id() : e->get_node_b_id();
}

// The first slice keeps p_edge_id and its start node; the rest becomes a
// new edge from p_node_id to the old end, registered at the node before
// the head is, as FinalStroke.AddNode orders it.
void CassieSketchGraph::_split_edge(int p_edge_id, const SplitPt &p_at, int p_node_id,
		bool p_on_new_stroke) {
	Ref<CassieSketchGraphEdge> e = edges[p_edge_id];
	const PackedVector3Array pts = e->get_points();
	const PackedFloat32Array prm = e->get_params();
	LocalVector<real_t> cl;
	_cumulative_lengths(pts, cl);
	PackedVector3Array head;
	PackedVector3Array tail;
	PackedFloat32Array head_prm;
	PackedFloat32Array tail_prm;
	float at_prm = 0.0f;
	for (int k = 0; k < pts.size(); ++k) {
		if (cl[k] < p_at.t) {
			head.push_back(pts[k]);
			head_prm.push_back(prm[k]);
		} else {
			if (tail.is_empty()) {
				const real_t span = k > 0 ? cl[k] - cl[k - 1] : real_t(0);
				const float f = span > real_t(0) ? float((p_at.t - cl[k - 1]) / span) : 0.0f;
				at_prm = k > 0 ? Math::lerp(prm[k - 1], prm[k], f) : prm[k];
			}
			tail.push_back(pts[k]);
			tail_prm.push_back(prm[k]);
		}
	}
	if (head.is_empty() || head[head.size() - 1].distance_to(p_at.pos) > real_t(1e-9)) {
		head.push_back(p_at.pos);
		head_prm.push_back(at_prm);
	}
	if (tail.is_empty() || tail[0].distance_to(p_at.pos) > real_t(1e-9)) {
		tail.insert(0, p_at.pos);
		tail_prm.insert(0, at_prm);
	}
	const int old_b = e->get_node_b_id();
	const int tail_eid = _new_edge(tail, p_node_id, old_b, e->get_source_polyline_idx(), p_on_new_stroke, tail_prm);
	PackedInt32Array ids = nodes[old_b]->get_edge_ids();
	ids.remove_at(ids.find(p_edge_id));
	nodes[old_b]->set_edge_ids(ids);
	e->set_points(head);
	e->set_params(head_prm);
	e->set_node_b_id(p_node_id);
	nodes[p_node_id]->add_edge_id(p_edge_id);
	_update_node_sharpness(old_b);
	_update_node_normal(old_b);
	_update_node_sharpness(p_node_id);
	_update_node_normal(p_node_id);
	if (!p_on_new_stroke) {
		_repair_cycles(p_edge_id, tail_eid);
	}
}

void CassieSketchGraph::_replace_node(int p_old_id, int p_keep_id) {
	const PackedInt32Array ids = nodes[p_old_id]->get_edge_ids();
	for (int i = 0; i < ids.size(); ++i) {
		Ref<CassieSketchGraphEdge> e = edges[ids[i]];
		if (e->get_node_a_id() == p_old_id) {
			e->set_node_a_id(p_keep_id);
		}
		if (e->get_node_b_id() == p_old_id) {
			e->set_node_b_id(p_keep_id);
		}
		nodes[p_keep_id]->add_edge_id(ids[i]);
	}
	nodes.erase(p_old_id);
	_update_node_sharpness(p_keep_id);
	_update_node_normal(p_keep_id);
	for (int i = 0; i < ids.size(); ++i) {
		_check_cycles_at(p_keep_id, ids[i]);
	}
}

int CassieSketchGraph::add_stroke_constrained(const PackedVector3Array &p_points,
		bool p_closed, const PackedVector3Array &p_intersections,
		const PackedInt32Array &p_targets, real_t p_snap, real_t p_merge,
		real_t p_reach, int p_source_idx) {
	last_constraint_sources.resize(p_intersections.size());
	last_constraint_sources.fill(-1);
	if (p_points.size() < 2) {
		return p_intersections.size();
	}
	const int node_a = _new_node(p_points[0]);
	const int node_b = p_closed ? node_a : _new_node(p_points[p_points.size() - 1]);
	_new_edge(p_points, node_a, node_b, p_source_idx, true);

	int unreached = 0;
	for (int i = 0; i < p_intersections.size(); ++i) {
		const Vector3 p = p_intersections[i];
		const int target = i < p_targets.size() ? p_targets[i] : -1;
		SplitPt own_hit;
		_closest_on_polyline(p_points, p, own_hit);
		SplitPt old_hit;
		const int old_eid = _nearest_edge(p, p_source_idx, false, target, own_hit.dir, old_hit);
		if (old_eid < 0 || old_hit.pos.distance_to(p) > p_reach) {
			unreached++;
			continue;
		}
		last_constraint_sources.set(i, edges[old_eid]->get_source_polyline_idx());
		int node = _closer_end(old_eid, p);
		if (nodes[node]->get_position().distance_to(p) >= p_snap) {
			node = _new_node(p);
			_split_edge(old_eid, old_hit, node, false);
		}
		SplitPt new_hit;
		const int new_eid = _nearest_edge(p, p_source_idx, true, -1, Vector3(), new_hit);
		const int closer = _closer_end(new_eid, p);
		if (nodes[closer]->get_position().distance_to(p) < p_merge) {
			if (closer != node) {
				_replace_node(closer, node);
			}
		} else {
			_split_edge(new_eid, new_hit, node, true);
		}
	}
	return unreached;
}

Ref<CassieSketchGraphNode> CassieSketchGraph::get_node(int p_id) const {
	HashMap<int, Ref<CassieSketchGraphNode>>::ConstIterator it = nodes.find(p_id);
	return it ? it->value : Ref<CassieSketchGraphNode>();
}

Ref<CassieSketchGraphEdge> CassieSketchGraph::get_edge(int p_id) const {
	HashMap<int, Ref<CassieSketchGraphEdge>>::ConstIterator it = edges.find(p_id);
	return it ? it->value : Ref<CassieSketchGraphEdge>();
}

TypedArray<CassieSketchGraphNode> CassieSketchGraph::get_all_nodes() const {
	TypedArray<CassieSketchGraphNode> result;
	for (const KeyValue<int, Ref<CassieSketchGraphNode>> &e : nodes) {
		result.push_back(e.value);
	}
	return result;
}

TypedArray<CassieSketchGraphEdge> CassieSketchGraph::get_all_edges() const {
	TypedArray<CassieSketchGraphEdge> result;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &e : edges) {
		result.push_back(e.value);
	}
	return result;
}

// Discrete parallel transport along the polyline. At each interior vertex
// of the polyline, the local tangent changes; rotate `v` by the same
// smallest-angle rotation that takes the previous tangent to the next.
// The result keeps v perpendicular-to-tangent to leading order, which is
// the property upstream's Stroke.ParallelTransport relies on.
Vector3 CassieSketchGraphEdge::parallel_transport(const Vector3 &p_v,
		int p_from_node_id) const {
	if (points.size() < 2) {
		return p_v;
	}
	const bool forward = (p_from_node_id == node_a_id);
	Vector3 v = p_v;
	const int n = points.size();
	const Vector3 p0 = forward ? points[0] : points[n - 1];
	const Vector3 p1 = forward ? points[1] : points[n - 2];
	Vector3 prev_t = p1 - p0;
	if (prev_t.length_squared() < real_t(1e-20)) {
		return v;
	}
	prev_t.normalize();
	for (int i = 1; i < n - 1; ++i) {
		const Vector3 pa = forward ? points[i] : points[n - 1 - i];
		const Vector3 pb = forward ? points[i + 1] : points[n - 2 - i];
		Vector3 next_t = pb - pa;
		if (next_t.length_squared() < real_t(1e-20)) {
			continue;
		}
		next_t.normalize();
		const Vector3 axis_unnorm = prev_t.cross(next_t);
		const real_t l2 = axis_unnorm.length_squared();
		if (l2 < real_t(1e-12)) {
			prev_t = next_t;
			continue; // collinear — no rotation
		}
		const real_t cos_a = CLAMP(prev_t.dot(next_t), real_t(-1.0), real_t(1.0));
		const real_t angle = Math::acos(cos_a);
		const Vector3 axis = axis_unnorm / Math::sqrt(l2);
		v = v.rotated(axis, angle);
		prev_t = next_t;
	}
	return v;
}

struct WalkNodeMeta {
	Vector3 normal;
	bool is_sharp = false;
	LocalVector<int> ring;
};

// Unity's Vector3.normalized: zero below 1e-5 rather than NaN.
static Vector3 _unity_normalized(const Vector3 &p_v) {
	const real_t len = p_v.length();
	return len > real_t(1e-5) ? p_v / len : Vector3();
}

static Vector3 _project_on_plane(const Vector3 &p_v, const Vector3 &p_n, bool p_normalize = true) {
	const Vector3 r = p_v - p_n * p_v.dot(p_n);
	return p_normalize ? _unity_normalized(r) : r;
}

// Utils.FitPlane(point, vectors): a plane through the vector tips and the
// point itself, normal from the best-conditioned covariance cofactor row.
// False when the vectors are collinear or the points span no plane.
static bool _fit_plane(const Vector3 &p_pos, const LocalVector<Vector3> &p_vectors,
		Vector3 &r_normal, real_t &r_err) {
	r_normal = Vector3();
	r_err = Math::INF;
	const uint32_t n = p_vectors.size();
	if (n < 2) {
		return false;
	}
	real_t score = 0;
	LocalVector<Vector3> pts;
	pts.resize(n + 1);
	for (uint32_t i = 0; i < n; ++i) {
		pts[i] = p_pos + p_vectors[i];
		score = MAX(score, p_vectors[i].cross(p_vectors[(i + 1) % n]).length());
	}
	pts[n] = p_pos;
	if (score < real_t(0.1)) {
		return false;
	}
	Vector3 c;
	for (uint32_t i = 0; i < pts.size(); ++i) {
		c += pts[i];
	}
	c /= real_t(pts.size());
	real_t xx = 0, xy = 0, xz = 0, yy = 0, yz = 0, zz = 0;
	for (uint32_t i = 0; i < pts.size(); ++i) {
		const Vector3 r = pts[i] - c;
		xx += r.x * r.x;
		xy += r.x * r.y;
		xz += r.x * r.z;
		yy += r.y * r.y;
		yz += r.y * r.z;
		zz += r.z * r.z;
	}
	const real_t det_x = yy * zz - yz * yz;
	const real_t det_y = xx * zz - xz * xz;
	const real_t det_z = xx * yy - xy * xy;
	const real_t det_max = MAX(det_x, MAX(det_y, det_z));
	if (det_max <= 0) {
		return false;
	}
	Vector3 nrm;
	if (det_max == det_x) {
		nrm = Vector3(det_x, xz * yz - xy * zz, xy * yz - xz * yy);
	} else if (det_max == det_y) {
		nrm = Vector3(xz * yz - xy * zz, det_y, xy * xz - yz * xx);
	} else {
		nrm = Vector3(xy * yz - xz * yy, xy * xz - yz * xx, det_z);
	}
	r_normal = _unity_normalized(nrm);
	r_err = 0;
	for (uint32_t i = 0; i < n; ++i) {
		r_err = MAX(r_err, Math::abs(r_normal.dot(p_vectors[i])));
	}
	return true;
}

// Node.SortSegments: insertion sort by angle about the normal, measured
// from the first neighbor; near-equal tangents compare by chord instead.
static void _sort_ring(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const HashMap<int, Ref<CassieSketchGraphNode>> &p_nodes, int p_nid,
		const Vector3 &p_normal, LocalVector<int> &r_ring) {
	if (r_ring.size() <= 2) {
		return;
	}
	const Vector3 pos = p_nodes[p_nid]->get_position();
	LocalVector<int> sorted;
	sorted.push_back(r_ring[0]);
	for (uint32_t k = 1; k < r_ring.size(); ++k) {
		const int s = r_ring[k];
		const Vector3 x0 = _project_on_plane(p_edges[sorted[0]]->get_tangent_away_from(p_nid), p_normal);
		const Vector3 y0 = x0.cross(p_normal);
		const Vector3 ts = _project_on_plane(p_edges[s]->get_tangent_away_from(p_nid), p_normal);
		const real_t ys = ts.dot(y0);
		const real_t xs = ts.dot(x0);
		bool after = true;
		uint32_t j = 0;
		while (j + 1 < sorted.size() && after) {
			j++;
			Vector3 tn = _project_on_plane(p_edges[sorted[j]]->get_tangent_away_from(p_nid), p_normal);
			real_t xsc = xs;
			real_t ysc = ys;
			if (tn.dot(ts) > real_t(0.99)) {
				tn = _unity_normalized(p_nodes[p_edges[sorted[j]]->get_opposite(p_nid)]->get_position() - pos);
				const Vector3 tsc = _unity_normalized(p_nodes[p_edges[s]->get_opposite(p_nid)]->get_position() - pos);
				xsc = tsc.dot(x0);
				ysc = tsc.dot(y0);
			}
			if (tn.dot(y0) >= 0) {
				if (ysc > 0 && tn.dot(x0) < xsc) {
					after = false;
				}
			} else if (ysc >= 0 || tn.dot(x0) > xsc) {
				after = false;
			}
		}
		if (after) {
			sorted.push_back(s);
		} else {
			sorted.insert(j, s);
		}
	}
	r_ring = sorted;
}

// Node.UpdateNormal plus the sort it triggers.
static void _node_meta(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const HashMap<int, Ref<CassieSketchGraphNode>> &p_nodes, int p_nid, WalkNodeMeta &r_m) {
	const Vector3 pos = p_nodes[p_nid]->get_position();
	const PackedInt32Array eids = p_nodes[p_nid]->get_edge_ids();
	LocalVector<Vector3> tangents;
	for (int i = 0; i < eids.size(); ++i) {
		if (!p_edges.has(eids[i])) {
			continue;
		}
		r_m.ring.push_back(eids[i]);
		tangents.push_back(p_edges[eids[i]]->get_tangent_away_from(p_nid));
	}
	r_m.normal = Vector3();
	r_m.is_sharp = false;
	if (r_m.ring.size() < 2) {
		return;
	}
	if (r_m.ring.size() > 2 || tangents[0].cross(tangents[1]).length() > real_t(0.1)) {
		Vector3 n;
		real_t err = 0;
		bool ok = _fit_plane(pos, tangents, n, err);
		if (!ok) {
			LocalVector<Vector3> pseudo;
			for (uint32_t i = 0; i < r_m.ring.size(); ++i) {
				pseudo.push_back(_unity_normalized(p_nodes[p_edges[r_m.ring[i]]->get_opposite(p_nid)]->get_position() - pos));
			}
			ok = _fit_plane(pos, pseudo, n, err);
		}
		r_m.is_sharp = ok && err > real_t(0.5);
		if (ok) {
			r_m.normal = n;
		}
	}
	if (r_m.normal.length() > 0) {
		_sort_ring(p_edges, p_nodes, p_nid, r_m.normal, r_m.ring);
	}
}

static int _ring_step(const LocalVector<int> &p_ring, int p_eid, int p_step) {
	const int64_t idx = p_ring.find(p_eid);
	if (idx < 0) {
		return -1;
	}
	const int64_t n = int64_t(p_ring.size());
	return p_ring[uint32_t((idx + p_step + n) % n)];
}

// Node.GetInPlane: the neighbor next (or previous) to the incoming edge
// by angle in the plane of N, ignoring edges that project below 0.7.
static int _get_in_plane(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		int p_nid, int p_incoming, const Vector3 &p_n, bool p_want_next,
		const LocalVector<int> &p_ring) {
	const Vector3 x0 = _project_on_plane(p_edges[p_incoming]->get_tangent_away_from(p_nid), p_n);
	const Vector3 y0 = x0.cross(p_n);
	int chosen = -1;
	int fallback = -1;
	real_t fallback_mag = 0;
	for (uint32_t i = 0; i < p_ring.size(); ++i) {
		const int eid = p_ring[i];
		if (eid == p_incoming) {
			continue;
		}
		Vector3 ts = _project_on_plane(p_edges[eid]->get_tangent_away_from(p_nid), p_n, false);
		const real_t mag = ts.length();
		if (mag < real_t(0.7)) {
			if (mag > fallback_mag) {
				fallback = eid;
				fallback_mag = mag;
			}
			continue;
		}
		ts = ts / mag;
		const real_t ys = ts.dot(y0);
		const real_t xs = ts.dot(x0);
		if (chosen < 0) {
			chosen = eid;
			continue;
		}
		const Vector3 tn = p_edges[chosen]->get_tangent_away_from(p_nid);
		bool take = false;
		if (tn.dot(y0) >= 0) {
			take = p_want_next ? (ys > 0 && tn.dot(x0) < xs) : (ys <= 0 || tn.dot(x0) > xs);
		} else {
			take = p_want_next ? (ys >= 0 || tn.dot(x0) > xs) : (ys < 0 && tn.dot(x0) < xs);
		}
		if (take) {
			chosen = eid;
		}
	}
	return chosen >= 0 ? chosen : fallback;
}

// Segment.ProjectInPlane: the tangent projected into the plane, or the
// chord when the tangent projects below 0.7 and the chord does better.
static Vector3 _project_in_plane(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const HashMap<int, Ref<CassieSketchGraphNode>> &p_nodes, int p_eid, int p_nid,
		const Vector3 &p_n) {
	const Vector3 proj = _project_on_plane(p_edges[p_eid]->get_tangent_away_from(p_nid), p_n, false);
	if (proj.length() < real_t(0.7)) {
		const Vector3 v2 = _unity_normalized(p_nodes[p_edges[p_eid]->get_opposite(p_nid)]->get_position() - p_nodes[p_nid]->get_position());
		const Vector3 proj2 = _project_on_plane(v2, p_n, false);
		if (proj2.length() > proj.length()) {
			return _unity_normalized(proj2);
		}
	}
	return _unity_normalized(proj);
}

static Vector3 _transport_across_node(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		int p_nid, int p_prev_eid, int p_next_eid, const Vector3 &p_normal) {
	const Vector3 prev_t = -p_edges[p_prev_eid]->get_tangent_away_from(p_nid);
	const Vector3 t = p_edges[p_next_eid]->get_tangent_away_from(p_nid);
	Vector3 axis = prev_t.cross(t);
	if (axis.length() > real_t(CMP_EPSILON)) {
		axis.normalize();
		const real_t theta = Math::acos(CLAMP(prev_t.dot(t), real_t(-1), real_t(1)));
		return p_normal.rotated(axis, theta);
	}
	return p_normal;
}

// CycleDetection.ShouldReverse: flip when the carried normal, transported to
// the next node, disagrees with that node's normal; when the two are near
// perpendicular, flip when the ring's next neighbor sits past its previous
// in the carried plane instead.
static void _should_reverse(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const HashMap<int, Ref<CassieSketchGraphNode>> &p_nodes, const WalkNodeMeta &p_next_meta,
		bool &r_reversed, const Vector3 &p_normal, int p_next_nid, int p_eid) {
	const Vector3 transported = p_edges[p_eid]->parallel_transport(p_normal, p_edges[p_eid]->get_opposite(p_next_nid));
	const real_t agreement = transported.dot(p_next_meta.normal);
	if (Math::abs(agreement) < real_t(0.5)) {
		const int next_at = _ring_step(p_next_meta.ring, p_eid, 1);
		const int prev_at = _ring_step(p_next_meta.ring, p_eid, -1);
		if (next_at < 0 || prev_at < 0) {
			return;
		}
		const Vector3 proj_next = _project_in_plane(p_edges, p_nodes, next_at, p_next_nid, transported);
		const Vector3 proj_prev = _project_in_plane(p_edges, p_nodes, prev_at, p_next_nid, transported);
		const Vector3 x0 = _project_in_plane(p_edges, p_nodes, p_eid, p_next_nid, transported);
		const Vector3 y0 = x0.cross(transported);
		const real_t two_pi = real_t(2.0) * real_t(Math::PI);
		const real_t theta_next = Math::fmod(Math::atan2(proj_next.dot(y0), proj_next.dot(x0)) + two_pi, two_pi);
		const real_t theta_prev = Math::fmod(Math::atan2(proj_prev.dot(y0), proj_prev.dot(x0)) + two_pi, two_pi);
		if (theta_next > theta_prev) {
			r_reversed = !r_reversed;
		}
	} else if (agreement < 0) {
		r_reversed = !r_reversed;
	}
}

// CycleDetection.DetectCycle. `p_cycle_count` is how many accepted cycles
// already border each edge; a third is never opened through it.
static bool _detect_cycle(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const HashMap<int, Ref<CassieSketchGraphNode>> &p_nodes,
		const HashMap<int, WalkNodeMeta> &p_meta, const HashMap<int, int> &p_cycle_count,
		int p_start_eid, int p_start_nid, bool p_reversed, LocalVector<int> &r_path) {
	r_path.clear();
	const int opposite_nid = p_edges[p_start_eid]->get_opposite(p_start_nid);
	if (opposite_nid == p_start_nid) {
		r_path.push_back(p_start_eid);
		return true;
	}
	if (p_meta[p_start_nid].ring.size() < 2 || p_meta[opposite_nid].ring.size() < 2) {
		return false;
	}
	bool reversed = p_reversed;
	int cur_eid = p_start_eid;
	int cur_nid = p_start_nid;
	Vector3 cur_n;
	HashSet<int> in_path;
	while (p_meta[cur_nid].ring.size() > 1) {
		if (in_path.has(cur_eid)) {
			break;
		}
		if (p_cycle_count.has(cur_eid) && p_cycle_count[cur_eid] >= 2) {
			break;
		}
		const WalkNodeMeta &m = p_meta[cur_nid];
		const Ref<CassieSketchGraphEdge> cur = p_edges[cur_eid];
		int next_eid = -1;
		if (m.ring.size() > 2) {
			const Vector3 transported = cur->parallel_transport(cur_n, cur->get_opposite(cur_nid));
			if (m.is_sharp && cur_n.length() > real_t(0.9)) {
				next_eid = _get_in_plane(p_edges, cur_nid, cur_eid, transported, reversed, m.ring);
			} else {
				next_eid = _ring_step(m.ring, cur_eid, reversed ? 1 : -1);
			}
			if (next_eid < 0) {
				break;
			}
			if (m.is_sharp) {
				if (transported.length() < real_t(0.1)) {
					cur_n = _unity_normalized(cur->get_tangent_away_from(cur_nid).cross(p_edges[next_eid]->get_tangent_away_from(cur_nid)));
					if (reversed) {
						cur_n = -cur_n;
					}
				} else {
					cur_n = _transport_across_node(p_edges, cur_nid, cur_eid, next_eid, transported);
				}
			} else {
				cur_n = m.normal;
			}
			const int next_nid = p_edges[next_eid]->get_opposite(cur_nid);
			if (p_meta[next_nid].ring.size() > 2 && !p_meta[next_nid].is_sharp) {
				_should_reverse(p_edges, p_nodes, p_meta[next_nid], reversed, cur_n, next_nid, next_eid);
			}
		} else {
			next_eid = _ring_step(m.ring, cur_eid, 1);
			if (next_eid < 0) {
				break;
			}
			if (cur_n.length() > real_t(0.9)) {
				cur_n = cur->parallel_transport(cur_n, cur->get_opposite(cur_nid));
				cur_n = _transport_across_node(p_edges, cur_nid, cur_eid, next_eid, cur_n);
			} else {
				const Vector3 n_trivial = cur->get_tangent_away_from(cur_nid).cross(p_edges[next_eid]->get_tangent_away_from(cur_nid));
				if (n_trivial.length() > real_t(0.5)) {
					cur_n = _unity_normalized(n_trivial);
				}
			}
			const int next_nid = p_edges[next_eid]->get_opposite(cur_nid);
			if (cur_n.length() > real_t(0.9) && p_meta[next_nid].ring.size() > 2 && !p_meta[next_nid].is_sharp) {
				_should_reverse(p_edges, p_nodes, p_meta[next_nid], reversed, cur_n, next_nid, next_eid);
			}
		}
		r_path.push_back(cur_eid);
		in_path.insert(cur_eid);
		cur_nid = p_edges[next_eid]->get_opposite(cur_nid);
		cur_eid = next_eid;
	}
	return cur_eid == p_start_eid && p_edges[cur_eid]->get_opposite(cur_nid) == opposite_nid && r_path.size() > 1;
}

// Port of Unity CASSIE's CycleDetection.cs and the Node.cs it leans on
// (Assets/Scripts/Data/Graph). Each edge seeds two walks, one per turning
// sense, from its non-sharp endpoint; smooth nodes step around a ring
// sorted about the fitted normal and reset the carried normal to it, sharp
// nodes pick in the plane of the transported normal, and an edge already
// bordering two accepted cycles closes the search. Edges are seeded in id
// order, which is the order the strokes arrived.
Array CassieSketchGraph::find_cycles() const {
	Array out;
	const int edge_count = edges.size();
	if (edge_count < 2) {
		return out;
	}

	HashMap<int, WalkNodeMeta> meta;
	for (const KeyValue<int, Ref<CassieSketchGraphNode>> &kv : nodes) {
		WalkNodeMeta m;
		_node_meta(edges, nodes, kv.key, m);
		meta.insert(kv.key, m);
	}

	LocalVector<int> order;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		order.push_back(kv.key);
	}
	SortArray<int> order_sorter;
	order_sorter.sort(order.ptr(), order.size());

	HashMap<int, int> cycle_count;
	HashSet<String> seen;
	for (uint32_t oi = 0; oi < order.size(); ++oi) {
		const int seed_eid = order[oi];
		const Ref<CassieSketchGraphEdge> seed = edges[seed_eid];
		int start_nid = seed->get_node_a_id();
		if (meta[start_nid].is_sharp && !meta[seed->get_node_b_id()].is_sharp) {
			start_nid = seed->get_node_b_id();
		}
		for (int sense = 0; sense < 2; ++sense) {
			LocalVector<int> path;
			if (!_detect_cycle(edges, nodes, meta, cycle_count, seed_eid, start_nid, sense == 1, path)) {
				continue;
			}
			LocalVector<int> sig;
			sig.resize(path.size());
			for (uint32_t i = 0; i < path.size(); ++i) {
				sig[i] = path[i];
			}
			SortArray<int> sig_sorter;
			sig_sorter.sort(sig.ptr(), sig.size());
			String key;
			for (uint32_t i = 0; i < sig.size(); ++i) {
				key += itos(sig[i]) + ",";
			}
			if (seen.has(key)) {
				continue;
			}
			seen.insert(key);
			PackedInt32Array cycle;
			cycle.resize(int(path.size()));
			int *w = cycle.ptrw();
			for (uint32_t i = 0; i < path.size(); ++i) {
				w[i] = path[i];
				cycle_count[path[i]] = cycle_count.has(path[i]) ? cycle_count[path[i]] + 1 : 1;
			}
			out.push_back(cycle);
		}
	}

	// Determinism guard: although Godot's HashMap/HashSet iterate in
	// insertion order — so this loop is already insertion-stable — we
	// further canonicalize the emitted list by sorting cycles
	// lexicographically by their SORTED edge-id signature. This keeps the
	// list order invariant under stroke-insertion-order changes (e.g. a
	// session replayed with strokes committed in a different order across
	// peers) and makes multi-peer comparisons trivial. Per-cycle traversal
	// order is preserved so sample_cycle_boundary still picks the correct
	// edge orientation.
	struct CycleEntry {
		PackedInt32Array cycle;
		LocalVector<int> sig;
	};
	LocalVector<CycleEntry> entries;
	entries.resize(out.size());
	for (int i = 0; i < out.size(); ++i) {
		CycleEntry &e = entries[i];
		e.cycle = out[i];
		e.sig.resize(e.cycle.size());
		for (int j = 0; j < e.cycle.size(); ++j) {
			e.sig[j] = e.cycle[j];
		}
		SortArray<int> sorter;
		sorter.sort(e.sig.ptr(), e.sig.size());
	}
	struct CycleLess {
		bool operator()(const CycleEntry &a, const CycleEntry &b) const {
			const int n = MIN(int(a.sig.size()), int(b.sig.size()));
			for (int i = 0; i < n; ++i) {
				if (a.sig[i] != b.sig[i]) {
					return a.sig[i] < b.sig[i];
				}
			}
			return a.sig.size() < b.sig.size();
		}
	};
	SortArray<CycleEntry, CycleLess> entries_sorter;
	entries_sorter.sort(entries.ptr(), entries.size());
	out.clear();
	for (uint32_t i = 0; i < entries.size(); ++i) {
		out.push_back(entries[i].cycle);
	}
	return out;
}

void CassieSketchGraph::_push_unique(LocalVector<int> &r_list, int p_value) {
	if (r_list.find(p_value) < 0) {
		r_list.push_back(p_value);
	}
}

void CassieSketchGraph::SlotSet::insert(int p_value) {
	if (p_value < 0 || slots.find(p_value) >= 0) {
		return;
	}
	if (free_slots.is_empty()) {
		slots.push_back(p_value);
		return;
	}
	const uint32_t at = free_slots[free_slots.size() - 1];
	free_slots.resize(free_slots.size() - 1);
	slots[at] = p_value;
}

void CassieSketchGraph::SlotSet::erase(int p_value) {
	const int64_t at = slots.find(p_value);
	if (at < 0) {
		return;
	}
	slots[uint32_t(at)] = -1;
	free_slots.push_back(uint32_t(at));
}

void CassieSketchGraph::SlotSet::clear() {
	slots.clear();
	free_slots.clear();
}

LocalVector<int> CassieSketchGraph::SlotSet::values() const {
	LocalVector<int> out;
	for (uint32_t i = 0; i < slots.size(); ++i) {
		if (slots[i] >= 0) {
			out.push_back(slots[i]);
		}
	}
	return out;
}

bool CassieSketchGraph::_cycle_adjacent(const WalkCycle &p_cycle, int p_eid_1, int p_eid_2) {
	const int64_t idx = p_cycle.edges.find(p_eid_1);
	if (idx < 0) {
		return false;
	}
	const int64_t n = int64_t(p_cycle.edges.size());
	return p_cycle.edges[uint32_t((idx + 1) % n)] == p_eid_2 || p_cycle.edges[uint32_t((idx + n - 1) % n)] == p_eid_2;
}

static String _cycle_key(const LocalVector<int> &p_edges) {
	LocalVector<int> sig(p_edges);
	SortArray<int> sorter;
	sorter.sort(sig.ptr(), sig.size());
	String key;
	for (uint32_t i = 0; i < sig.size(); ++i) {
		key += itos(sig[i]) + ",";
	}
	return key;
}

bool CassieSketchGraph::_try_add_cycle(const LocalVector<int> &p_path, int p_start_nid, Array &r_added) {
	const String key = _cycle_key(p_path);
	for (const KeyValue<int, WalkCycle> &kv : live_cycles) {
		if (_cycle_key(kv.value.edges) == key) {
			return false;
		}
	}
	WalkCycle c;
	c.edges = p_path;
	int entry = p_start_nid;
	for (uint32_t i = 0; i < p_path.size(); ++i) {
		const Ref<CassieSketchGraphEdge> e = edges[p_path[i]];
		c.reversed.push_back(e->get_node_a_id() != e->get_node_b_id() && entry == e->get_node_b_id());
		entry = e->get_opposite(entry);
	}
	const int cid = next_cycle_id++;
	for (uint32_t i = 0; i < p_path.size(); ++i) {
		if (!cycles_by_edge.has(p_path[i])) {
			cycles_by_edge.insert(p_path[i], LocalVector<int>());
		}
		cycles_by_edge[p_path[i]].push_back(cid);
	}
	live_cycles.insert(cid, c);
	PackedInt32Array out;
	out.resize(int(p_path.size()));
	for (uint32_t i = 0; i < p_path.size(); ++i) {
		out.set(int(i), p_path[i]);
	}
	r_added.push_back(out);
	return true;
}

void CassieSketchGraph::_remove_cycle(int p_cycle_id) {
	const WalkCycle &c = live_cycles[p_cycle_id];
	for (uint32_t i = 0; i < c.edges.size(); ++i) {
		if (cycles_by_edge.has(c.edges[i])) {
			cycles_by_edge[c.edges[i]].erase(p_cycle_id);
		}
	}
	cycles_to_check.erase(p_cycle_id);
	live_cycles.erase(p_cycle_id);
}

// Graph.RepairCycles: the tail joins every cycle the head borders, right
// after it along the cycle's own direction, and the cycle is queued.
void CassieSketchGraph::_repair_cycles(int p_old_eid, int p_new_eid) {
	if (!cycles_by_edge.has(p_old_eid)) {
		return;
	}
	const LocalVector<int> touched(cycles_by_edge[p_old_eid]);
	for (uint32_t k = 0; k < touched.size(); ++k) {
		WalkCycle &c = live_cycles[touched[k]];
		const int64_t idx = c.edges.find(p_old_eid);
		if (idx < 0) {
			continue;
		}
		const bool rev = c.reversed[uint32_t(idx)];
		const uint32_t at = uint32_t(rev ? idx : idx + 1);
		c.edges.insert(at, p_new_eid);
		c.reversed.insert(at, rev);
		if (!cycles_by_edge.has(p_new_eid)) {
			cycles_by_edge.insert(p_new_eid, LocalVector<int>());
		}
		cycles_by_edge[p_new_eid].push_back(touched[k]);
		_push_unique(cycles_to_check, touched[k]);
	}
}

// Graph.UpdateNeighborsCycles.
void CassieSketchGraph::_check_cycles_at(int p_node_id, int p_edge_id) {
	WalkNodeMeta m;
	_node_meta(edges, nodes, p_node_id, m);
	const int next = _ring_step(m.ring, p_edge_id, 1);
	const int prev = _ring_step(m.ring, p_edge_id, -1);
	if (next < 0 || !cycles_by_edge.has(next)) {
		return;
	}
	const LocalVector<int> &cids = cycles_by_edge[next];
	for (uint32_t k = 0; k < cids.size(); ++k) {
		if (_cycle_adjacent(live_cycles[cids[k]], next, prev)) {
			_push_unique(cycles_to_check, cids[k]);
		}
	}
}

// Graph.TryReachCycle: from p_start_nid, walk away from p_start_eid through
// degree-2 nodes for at most five steps looking for a node whose two other
// edges are consecutive in the cycle.
static bool _try_reach_cycle(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const HashMap<int, WalkNodeMeta> &p_meta, const CassieSketchGraph::WalkCycle &p_cycle,
		int p_start_eid, int p_start_nid) {
	int node = p_start_nid;
	int s1 = _ring_step(p_meta[node].ring, p_start_eid, 1);
	int s2 = _ring_step(p_meta[node].ring, p_start_eid, -1);
	for (int i = 0; i < 5 && p_meta[node].ring.size() > 1; ++i) {
		if (CassieSketchGraph::_cycle_adjacent(p_cycle, s1, s2)) {
			return true;
		}
		if (p_meta[node].ring.size() > 2) {
			return false;
		}
		node = p_edges[s1]->get_opposite(node);
		s2 = _ring_step(p_meta[node].ring, s1, -1);
		s1 = _ring_step(p_meta[node].ring, s1, 1);
		if (s1 < 0) {
			return false;
		}
	}
	return false;
}

Array CassieSketchGraph::update_cycles() {
	Array added;
	HashMap<int, WalkNodeMeta> meta;
	for (const KeyValue<int, Ref<CassieSketchGraphNode>> &kv : nodes) {
		WalkNodeMeta m;
		_node_meta(edges, nodes, kv.key, m);
		meta.insert(kv.key, m);
	}

	LocalVector<int> seeds;
	const LocalVector<int> to_check(cycles_to_check);
	const LocalVector<int> updated = updated_edges.values();
	for (uint32_t ci = 0; ci < to_check.size(); ++ci) {
		if (!live_cycles.has(to_check[ci])) {
			continue;
		}
		for (uint32_t si = 0; si < updated.size(); ++si) {
			const int s = updated[si];
			const Ref<CassieSketchGraphEdge> e = edges[s];
			const int a = e->get_node_a_id();
			const int b = e->get_node_b_id();
			const WalkCycle &c = live_cycles[to_check[ci]];
			bool cut = false;
			if (_cycle_adjacent(c, _ring_step(meta[a].ring, s, 1), _ring_step(meta[a].ring, s, -1))) {
				cut = _try_reach_cycle(edges, meta, c, s, b);
			} else if (_cycle_adjacent(c, _ring_step(meta[b].ring, s, 1), _ring_step(meta[b].ring, s, -1))) {
				cut = _try_reach_cycle(edges, meta, c, s, a);
			}
			if (cut) {
				_push_unique(seeds, c.edges[0]);
				_remove_cycle(to_check[ci]);
				break;
			}
		}
	}
	cycles_to_check.clear();

	HashMap<int, int> cycle_count;
	for (const KeyValue<int, LocalVector<int>> &kv : cycles_by_edge) {
		cycle_count.insert(kv.key, int(kv.value.size()));
	}
	for (uint32_t si = 0; si < updated.size(); ++si) {
		seeds.push_back(updated[si]);
	}
	updated_edges.clear();

	for (uint32_t si = 0; si < seeds.size(); ++si) {
		const int seed_eid = seeds[si];
		if (!edges.has(seed_eid)) {
			continue;
		}
		const Ref<CassieSketchGraphEdge> seed = edges[seed_eid];
		int start_nid = seed->get_node_a_id();
		if (meta[start_nid].is_sharp && !meta[seed->get_node_b_id()].is_sharp) {
			start_nid = seed->get_node_b_id();
		}
		for (int sense = 0; sense < 2; ++sense) {
			LocalVector<int> path;
			if (!_detect_cycle(edges, nodes, meta, cycle_count, seed_eid, start_nid, sense == 1, path)) {
				continue;
			}
			if (!_try_add_cycle(path, start_nid, added)) {
				continue;
			}
			for (uint32_t i = 0; i < path.size(); ++i) {
				cycle_count[path[i]] = cycle_count.has(path[i]) ? cycle_count[path[i]] + 1 : 1;
			}
		}
	}
	return added;
}

// Graph.MendSegments at a joint holding two edges of one stroke: the edge
// ending here absorbs the one starting here. The stroke's first edge keeps
// its id through every split, so a "right" with the stroke's lowest id is
// its first edge and the joint closes the stroke; Unity leaves that one.
void CassieSketchGraph::_mend_at(int p_node_id) {
	const PackedInt32Array ids = nodes[p_node_id]->get_edge_ids();
	if (ids.size() != 2 || ids[0] == ids[1]) {
		return;
	}
	const Ref<CassieSketchGraphEdge> e0 = edges[ids[0]];
	const Ref<CassieSketchGraphEdge> e1 = edges[ids[1]];
	const int source = e0->get_source_polyline_idx();
	if (source != e1->get_source_polyline_idx()) {
		return;
	}
	int left = -1;
	int right = -1;
	if (e0->get_node_b_id() == p_node_id && e1->get_node_a_id() == p_node_id) {
		left = ids[0];
		right = ids[1];
	} else if (e1->get_node_b_id() == p_node_id && e0->get_node_a_id() == p_node_id) {
		left = ids[1];
		right = ids[0];
	} else {
		return;
	}
	bool closing = true;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		closing = closing && !(kv.value->get_source_polyline_idx() == source && kv.key < right);
	}
	if (closing) {
		return;
	}
	if (cycles_by_edge.has(right)) {
		const LocalVector<int> touched(cycles_by_edge[right]);
		for (uint32_t k = 0; k < touched.size(); ++k) {
			WalkCycle &c = live_cycles[touched[k]];
			const int64_t idx = c.edges.find(right);
			if (idx >= 0) {
				c.edges.remove_at(uint32_t(idx));
				c.reversed.remove_at(uint32_t(idx));
			}
		}
		cycles_by_edge.erase(right);
	}
	Ref<CassieSketchGraphEdge> keep = edges[left];
	const Ref<CassieSketchGraphEdge> gone = edges[right];
	PackedVector3Array pts = keep->get_points();
	PackedFloat32Array prm = keep->get_params();
	const PackedVector3Array tail = gone->get_points();
	const PackedFloat32Array tail_prm = gone->get_params();
	for (int i = 0; i < tail.size(); ++i) {
		if (i == 0 && pts.size() > 0 && pts[pts.size() - 1].distance_to(tail[0]) <= real_t(1e-9)) {
			continue;
		}
		pts.push_back(tail[i]);
		prm.push_back(tail_prm[i]);
	}
	keep->set_points(pts);
	keep->set_params(prm);
	const int far_node = gone->get_node_b_id();
	keep->set_node_b_id(far_node);
	PackedInt32Array far_ids = nodes[far_node]->get_edge_ids();
	far_ids.remove_at(far_ids.find(right));
	far_ids.push_back(left);
	nodes[far_node]->set_edge_ids(far_ids);
	edges.erase(right);
	updated_edges.erase(right);
	nodes.erase(p_node_id);
	_update_node_sharpness(far_node);
	_update_node_normal(far_node);
}

// Graph.Remove(ISegment).
void CassieSketchGraph::_remove_stroke_edge(int p_edge_id) {
	const Ref<CassieSketchGraphEdge> e = edges[p_edge_id];
	const int ends[2] = { e->get_node_a_id(), e->get_node_b_id() };
	for (int k = 0; k < 2; ++k) {
		WalkNodeMeta m;
		_node_meta(edges, nodes, ends[k], m);
		if (m.ring.size() < 2) {
			continue;
		}
		if (m.ring.size() > 2) {
			updated_edges.insert(_ring_step(m.ring, p_edge_id, -1));
		}
		updated_edges.insert(_ring_step(m.ring, p_edge_id, 1));
	}
	_remove_edge(p_edge_id);
	for (int k = 0; k < 2; ++k) {
		if (!nodes.has(ends[k])) {
			continue;
		}
		if (nodes[ends[k]]->get_edge_ids().is_empty()) {
			nodes.erase(ends[k]);
		} else {
			_mend_at(ends[k]);
		}
	}
	if (cycles_by_edge.has(p_edge_id)) {
		const LocalVector<int> gone(cycles_by_edge[p_edge_id]);
		for (uint32_t k = 0; k < gone.size(); ++k) {
			_remove_cycle(gone[k]);
		}
		cycles_by_edge.erase(p_edge_id);
	}
	updated_edges.erase(p_edge_id);
}

int CassieSketchGraph::remove_stroke(int p_source_idx) {
	LocalVector<int> owned;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		if (kv.value->get_source_polyline_idx() == p_source_idx) {
			owned.push_back(kv.key);
		}
	}
	SortArray<int> sorter;
	sorter.sort(owned.ptr(), owned.size());
	// Unity walks the stroke's segment list head to tail, so chain the edges
	// by shared joint; an id that no other owned edge ends at is the head.
	LocalVector<int> ordered;
	int head = owned.is_empty() ? -1 : owned[0];
	for (uint32_t i = 0; i < owned.size(); ++i) {
		bool entered = false;
		for (uint32_t k = 0; k < owned.size() && !entered; ++k) {
			entered = k != i && edges[owned[k]]->get_node_b_id() == edges[owned[i]]->get_node_a_id();
		}
		if (!entered) {
			head = owned[i];
			break;
		}
	}
	while (head >= 0 && ordered.size() < owned.size()) {
		ordered.push_back(head);
		const int tail_node = edges[head]->get_node_b_id();
		int next = -1;
		for (uint32_t k = 0; k < owned.size() && next < 0; ++k) {
			if (ordered.find(owned[k]) < 0 && edges[owned[k]]->get_node_a_id() == tail_node) {
				next = owned[k];
			}
		}
		head = next;
	}
	for (uint32_t i = 0; i < owned.size(); ++i) {
		if (ordered.find(owned[i]) < 0) {
			ordered.push_back(owned[i]);
		}
	}
	int removed = 0;
	for (uint32_t i = 0; i < ordered.size(); ++i) {
		if (!edges.has(ordered[i])) {
			continue;
		}
		_remove_stroke_edge(ordered[i]);
		removed++;
	}
	return removed;
}

bool CassieSketchGraph::remove_cycle(const PackedInt32Array &p_edge_ids) {
	LocalVector<int> path;
	for (int i = 0; i < p_edge_ids.size(); ++i) {
		path.push_back(p_edge_ids[i]);
	}
	const String key = _cycle_key(path);
	for (const KeyValue<int, WalkCycle> &kv : live_cycles) {
		if (_cycle_key(kv.value.edges) == key) {
			_remove_cycle(kv.key);
			return true;
		}
	}
	return false;
}

int CassieSketchGraph::_live_degree(int p_node_id) const {
	const PackedInt32Array eids = nodes[p_node_id]->get_edge_ids();
	int degree = 0;
	for (int i = 0; i < eids.size(); ++i) {
		degree += edges.has(eids[i]) ? 1 : 0;
	}
	return degree;
}

int CassieSketchGraph::_cycle_count(int p_edge_id) const {
	return cycles_by_edge.has(p_edge_id) ? int(cycles_by_edge[p_edge_id].size()) : 0;
}

// Graph.FindClosestSegment: the edge whose two ends and midpoint sit closest
// to the press on average, skipping edges with a loose end and, unless asked
// to look at non-manifold ones, edges already bordering two cycles.
int CassieSketchGraph::_closest_edge_to(const Vector3 &p_pos, bool p_look_at_non_manifold) const {
	int closest = -1;
	real_t min_dist = real_t(10);
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		if (!p_look_at_non_manifold && _cycle_count(kv.key) >= 2) {
			continue;
		}
		const int a = kv.value->get_node_a_id();
		const int b = kv.value->get_node_b_id();
		if (_live_degree(a) < 2 || _live_degree(b) < 2) {
			continue;
		}
		const real_t avg = (nodes[a]->get_position().distance_to(p_pos) + nodes[b]->get_position().distance_to(p_pos) + kv.value->get_point_at(real_t(0.5)).distance_to(p_pos)) / real_t(3);
		if (avg < min_dist) {
			closest = kv.key;
			min_dist = avg;
		}
	}
	return closest;
}

// Graph.FindClosestAmongNeighbors: the ring neighbor with the nearest of
// five samples to the press. Reaching a neighbor that already borders two
// cycles ends the scan where it stands, which is upstream's `break`.
int CassieSketchGraph::_closest_neighbor_to(const Vector3 &p_pos, int p_node_id, int p_edge_id, bool p_look_at_non_manifold) const {
	WalkNodeMeta m;
	_node_meta(edges, nodes, p_node_id, m);
	int closest = -1;
	real_t min_dist = real_t(10);
	for (uint32_t i = 0; i < m.ring.size(); ++i) {
		const int eid = m.ring[i];
		if (eid == p_edge_id) {
			continue;
		}
		if (!p_look_at_non_manifold && _cycle_count(eid) >= 2) {
			break;
		}
		if (_live_degree(edges[eid]->get_opposite(p_node_id)) < 2) {
			continue;
		}
		real_t dist = Math::INF;
		for (int k = 1; k <= 5; ++k) {
			dist = MIN(dist, edges[eid]->get_point_at(real_t(k) / real_t(5)).distance_to(p_pos));
		}
		if (dist < min_dist) {
			closest = eid;
			min_dist = dist;
		}
	}
	return closest;
}

Array CassieSketchGraph::find_cycle_at(const Vector3 &p_pos, bool p_look_at_non_manifold) {
	Array added;
	const int closest = _closest_edge_to(p_pos, p_look_at_non_manifold);
	if (closest < 0) {
		return added;
	}
	const int start_nid = edges[closest]->get_node_a_id();
	LocalVector<int> path;
	if (edges[closest]->get_node_b_id() == start_nid) {
		path.push_back(closest);
		_try_add_cycle(path, start_nid, added);
		return added;
	}
	int cur_nid = start_nid;
	int cur_eid = closest;
	int counter = 0;
	while (counter <= 10) {
		if (path.find(cur_eid) >= 0) {
			break;
		}
		path.push_back(cur_eid);
		cur_nid = edges[cur_eid]->get_opposite(cur_nid);
		const int next_eid = _closest_neighbor_to(p_pos, cur_nid, cur_eid, p_look_at_non_manifold);
		if (next_eid < 0) {
			break;
		}
		if (edges[cur_eid]->get_tangent_away_from(cur_nid).dot(edges[next_eid]->get_tangent_away_from(cur_nid)) < real_t(0.8)) {
			counter++;
		}
		cur_eid = next_eid;
	}
	if (cur_nid == start_nid && cur_eid == closest && path.size() > 1) {
		_try_add_cycle(path, start_nid, added);
	}
	return added;
}

Array CassieSketchGraph::get_live_cycles() const {
	Array out;
	for (const KeyValue<int, WalkCycle> &kv : live_cycles) {
		PackedInt32Array cycle;
		cycle.resize(int(kv.value.edges.size()));
		for (uint32_t i = 0; i < kv.value.edges.size(); ++i) {
			cycle.set(int(i), kv.value.edges[i]);
		}
		out.push_back(cycle);
	}
	return out;
}

PackedVector3Array CassieSketchGraph::sample_cycle_boundary(
		const PackedInt32Array &p_cycle_edge_ids,
		real_t p_target_edge_length) const {
	PackedVector3Array out;
	const int N = p_cycle_edge_ids.size();
	// two-edge cycles too (a closed stroke split in
	// two by CassieSketcher::split_closed_strokes), which find_cycles
	// reports and this used to refuse (N < 3), so they never became patches.
	if (N < 2 || p_target_edge_length <= 0) {
		return out;
	}
	int prev_exit = -1;
	for (int i = 0; i < N; ++i) {
		const int eid = p_cycle_edge_ids[i];
		Ref<CassieSketchGraphEdge> edge = get_edge(eid);
		if (edge.is_null()) {
			continue;
		}
		// The exit node from edge `eid` is the node it shares with the
		// next edge in the cycle (wraparound at the end).
		const int next_eid = p_cycle_edge_ids[(i + 1) % N];
		Ref<CassieSketchGraphEdge> next_edge = get_edge(next_eid);
		if (next_edge.is_null()) {
			continue;
		}
		const int a = edge->get_node_a_id();
		const int b = edge->get_node_b_id();
		const int na = next_edge->get_node_a_id();
		const int nb = next_edge->get_node_b_id();
		int exit_nid = -1;
		if ((a == na || a == nb) && (b == na || b == nb) && a != b) {
			// Both ends shared with the next edge (a two-edge cycle): enter
			// where the previous edge left off, else at node a.
			exit_nid = (prev_exit == a) ? b : (prev_exit == b ? a : b);
		} else if (a == na || a == nb) {
			exit_nid = a;
		} else if (b == na || b == nb) {
			exit_nid = b;
		} else {
			continue;
		}
		prev_exit = exit_nid;
		const int entry_nid = edge->get_opposite(exit_nid);
		const bool reversed = (entry_nid == b);

		const PackedVector3Array pts = edge->get_points();
		const int M = pts.size();
		if (M < 2) {
			continue;
		}
		real_t seg_len = 0;
		for (int j = 0; j < M - 1; ++j) {
			seg_len += pts[j].distance_to(pts[j + 1]);
		}
		const int samples = MAX(2,
				int(Math::round(seg_len / p_target_edge_length)));

		// Walk edge points from entry → exit. When `reversed`, mirror
		// the parametric lookup so the output stays in CCW cycle order.
		for (int s = 0; s < samples; ++s) {
			const real_t t = real_t(s) / real_t(samples);
			const real_t idx_f = t * real_t(M - 1);
			const int idx0 = CLAMP(int(Math::floor(idx_f)), 0, M - 2);
			const real_t frac = idx_f - real_t(idx0);
			Vector3 p;
			if (!reversed) {
				p = pts[idx0].lerp(pts[idx0 + 1], frac);
			} else {
				p = pts[M - 1 - idx0].lerp(pts[M - 2 - idx0], frac);
			}
			out.push_back(p);
		}
	}
	return out;
}

void CassieSketchGraph::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_merge_epsilon", "value"),
			&CassieSketchGraph::set_merge_epsilon);
	ClassDB::bind_method(D_METHOD("get_merge_epsilon"),
			&CassieSketchGraph::get_merge_epsilon);
	ClassDB::bind_method(D_METHOD("clear"), &CassieSketchGraph::clear);
	ClassDB::bind_method(D_METHOD("add_stroke", "points", "normals"),
			&CassieSketchGraph::add_stroke);
	ClassDB::bind_method(D_METHOD("build_from_polylines", "polylines", "proximity"),
			&CassieSketchGraph::build_from_polylines);
	ClassDB::bind_method(D_METHOD("add_stroke_intersecting", "points", "normals", "proximity", "boundary"),
			&CassieSketchGraph::add_stroke_intersecting, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("is_opening", "cycle_edge_ids"),
			&CassieSketchGraph::is_opening);
	ClassDB::bind_method(D_METHOD("add_stroke_constrained", "points", "closed", "intersections", "targets", "snap", "merge", "reach", "source_idx"),
			&CassieSketchGraph::add_stroke_constrained);
	ClassDB::bind_method(D_METHOD("get_last_constraint_sources"), &CassieSketchGraph::get_last_constraint_sources);
	ClassDB::bind_method(D_METHOD("remove_stroke", "source_idx"), &CassieSketchGraph::remove_stroke);
	ClassDB::bind_method(D_METHOD("remove_cycle", "edge_ids"), &CassieSketchGraph::remove_cycle);
	ClassDB::bind_method(D_METHOD("update_cycles"), &CassieSketchGraph::update_cycles);
	ClassDB::bind_method(D_METHOD("get_live_cycles"), &CassieSketchGraph::get_live_cycles);
	ClassDB::bind_method(D_METHOD("find_cycle_at", "pos", "look_at_non_manifold"), &CassieSketchGraph::find_cycle_at);
	ClassDB::bind_method(D_METHOD("get_edge_count"),
			&CassieSketchGraph::get_edge_count);
	ClassDB::bind_method(D_METHOD("get_node_count"),
			&CassieSketchGraph::get_node_count);
	ClassDB::bind_method(D_METHOD("get_node", "id"),
			&CassieSketchGraph::get_node);
	ClassDB::bind_method(D_METHOD("get_edge", "id"),
			&CassieSketchGraph::get_edge);
	ClassDB::bind_method(D_METHOD("get_all_nodes"),
			&CassieSketchGraph::get_all_nodes);
	ClassDB::bind_method(D_METHOD("get_all_edges"),
			&CassieSketchGraph::get_all_edges);
	ClassDB::bind_method(D_METHOD("find_cycles"),
			&CassieSketchGraph::find_cycles);
	ClassDB::bind_method(D_METHOD("sample_cycle_boundary",
								 "cycle_edge_ids", "target_edge_length"),
			&CassieSketchGraph::sample_cycle_boundary);
}
