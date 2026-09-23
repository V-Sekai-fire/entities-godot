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
	// of within_other-parallel segments around one crossing coalesces to one hit
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

// Sorts p_splits by arc length, drops within_other-coincident ones, and emits one
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

// Rotation taking unit `p_from` onto unit `p_to`, applied to `p_v`.
static Vector3 _rotate_between(const Vector3 &p_from, const Vector3 &p_to,
		const Vector3 &p_v) {
	const real_t c = p_from.dot(p_to);
	if (c > real_t(0.99999)) {
		return p_v;
	}
	if (c < real_t(-0.99999)) {
		const Vector3 helper = Math::abs(p_from.x) < real_t(0.9) ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
		const Vector3 k = p_from.cross(helper).normalized();
		return p_v * real_t(-1.0) + k * (k.dot(p_v) * real_t(2.0));
	}
	const Vector3 k_raw = p_from.cross(p_to);
	const real_t k_len = k_raw.length();
	const Vector3 k = k_len > real_t(1e-12) ? k_raw / k_len : Vector3(0, 0, 1);
	const real_t cc = CLAMP(c, real_t(-1.0), real_t(1.0));
	const real_t s = Math::sqrt(real_t(1.0) - cc * cc);
	return p_v * cc + k.cross(p_v) * s + k * (k.dot(p_v) * (real_t(1.0) - cc));
}

struct WalkNodeMeta {
	Vector3 normal = Vector3(0, 1, 0);
	bool is_sharp = false;
	LocalVector<int> ring;
};

// Best-fit plane through the incident unit tangents; the residual is the
// largest |t·n| and feeds the sharp test.
static real_t _fit_plane(const LocalVector<Vector3> &p_tangents, Vector3 &r_normal) {
	r_normal = Vector3(0, 1, 0);
	if (p_tangents.size() < 2) {
		return 0;
	}
	if (p_tangents.size() == 2 && p_tangents[0].cross(p_tangents[1]).length() < real_t(0.1)) {
		r_normal = Vector3();
		return 0;
	}
	Vector3 c;
	for (uint32_t i = 0; i < p_tangents.size(); ++i) {
		c += p_tangents[i];
	}
	c /= real_t(p_tangents.size());
	real_t cxx = 0, cyy = 0, czz = 0, cxy = 0, cxz = 0, cyz = 0;
	for (uint32_t i = 0; i < p_tangents.size(); ++i) {
		const Vector3 d = p_tangents[i] - c;
		cxx += d.x * d.x;
		cyy += d.y * d.y;
		czz += d.z * d.z;
		cxy += d.x * d.y;
		cxz += d.x * d.z;
		cyz += d.y * d.z;
	}
	const real_t trace = cxx + cyy + czz;
	// Power iteration cannot leave the seed's invariant subspace, so a
	// Y-up seed on a z=0 sketch never gains a z component; seed from the
	// tangents instead.
	Vector3 n(0, 1, 0);
	for (uint32_t i = 1; i < p_tangents.size(); ++i) {
		const Vector3 c0 = p_tangents[0].cross(p_tangents[i]);
		if (c0.length_squared() > real_t(1e-10)) {
			n = c0.normalized();
			break;
		}
	}
	for (int it = 0; it < 256; ++it) {
		const Vector3 cn(
				cxx * n.x + cxy * n.y + cxz * n.z,
				cxy * n.x + cyy * n.y + cyz * n.z,
				cxz * n.x + cyz * n.y + czz * n.z);
		const Vector3 r = n * trace - cn;
		const real_t r2 = r.length_squared();
		if (r2 < real_t(1e-20)) {
			break;
		}
		const Vector3 next = r / Math::sqrt(r2);
		const bool converged = (next - n).length_squared() < real_t(1e-14);
		n = next;
		if (converged) {
			break;
		}
	}
	r_normal = n;
	real_t max_abs = 0;
	for (uint32_t i = 0; i < p_tangents.size(); ++i) {
		max_abs = MAX(max_abs, Math::abs(p_tangents[i].dot(n)));
	}
	return max_abs;
}

static void _sort_ring_ccw(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		int p_nid, const Vector3 &p_normal, LocalVector<int> &r_ring) {
	if (r_ring.size() <= 2) {
		return;
	}
	const Vector3 t0 = p_edges[r_ring[0]]->get_tangent_away_from(p_nid);
	const Vector3 x_raw = t0 - p_normal * t0.dot(p_normal);
	const real_t x_len = x_raw.length();
	if (x_len < real_t(1e-6)) {
		return;
	}
	const Vector3 x_axis = x_raw / x_len;
	const Vector3 y_axis = p_normal.cross(x_axis);
	const real_t two_pi = real_t(2.0) * real_t(Math::PI);
	struct Keyed {
		real_t theta;
		int eid;
		bool operator<(const Keyed &p_o) const { return theta < p_o.theta; }
	};
	LocalVector<Keyed> keyed;
	keyed.resize(r_ring.size());
	for (uint32_t i = 0; i < r_ring.size(); ++i) {
		const Vector3 t = p_edges[r_ring[i]]->get_tangent_away_from(p_nid);
		const Vector3 p_raw = t - p_normal * t.dot(p_normal);
		const real_t p_len = p_raw.length();
		real_t theta = two_pi;
		if (p_len >= real_t(1e-6)) {
			const Vector3 p = p_raw / p_len;
			theta = Math::atan2(p.dot(y_axis), p.dot(x_axis));
			if (theta < 0) {
				theta += two_pi;
			}
		}
		keyed[i] = Keyed{ theta, r_ring[i] };
	}
	SortArray<Keyed> sorter;
	sorter.sort(keyed.ptr(), keyed.size());
	for (uint32_t i = 0; i < keyed.size(); ++i) {
		r_ring[i] = keyed[i].eid;
	}
}

// Sharp-node pick: the incident edge whose in-plane tangent sits next
// (or previous) to the incoming edge's, with Unity's 0.7 projection floor.
static int _get_in_plane(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		int p_nid, int p_incoming, const Vector3 &p_n, bool p_want_next,
		const LocalVector<int> &p_ring) {
	const Vector3 t_in = p_edges[p_incoming]->get_tangent_away_from(p_nid);
	const Vector3 x0_raw = t_in - p_n * t_in.dot(p_n);
	const real_t x0_len = x0_raw.length();
	if (x0_len < real_t(1e-6)) {
		return -1;
	}
	const Vector3 x0 = x0_raw / x0_len;
	const Vector3 y0 = x0.cross(p_n);
	int chosen = -1;
	real_t chosen_x = 0;
	real_t chosen_y = 0;
	int fallback = -1;
	real_t fallback_mag = 0;
	for (uint32_t i = 0; i < p_ring.size(); ++i) {
		const int eid = p_ring[i];
		if (eid == p_incoming) {
			continue;
		}
		const Vector3 t = p_edges[eid]->get_tangent_away_from(p_nid);
		const Vector3 p_raw = t - p_n * t.dot(p_n);
		const real_t p_mag = p_raw.length();
		if (p_mag < real_t(0.7)) {
			if (p_mag > fallback_mag) {
				fallback = eid;
				fallback_mag = p_mag;
			}
			continue;
		}
		const Vector3 p = p_raw / p_mag;
		const real_t xs = p.dot(x0);
		const real_t ys = p.dot(y0);
		bool take = chosen < 0;
		if (!take) {
			if (chosen_y >= 0) {
				take = p_want_next ? (ys > 0 && chosen_x < xs) : (ys <= 0 || chosen_x > xs);
			} else {
				take = p_want_next ? (ys >= 0 || chosen_x > xs) : (ys < 0 && chosen_x < xs);
			}
		}
		if (take) {
			chosen = eid;
			chosen_x = xs;
			chosen_y = ys;
		}
	}
	return chosen >= 0 ? chosen : fallback;
}

static int _next_edge_port(const HashMap<int, Ref<CassieSketchGraphEdge>> &p_edges,
		const WalkNodeMeta &p_meta, int p_nid, int p_incoming,
		const Vector3 &p_transported, bool p_reversed) {
	const LocalVector<int> &ring = p_meta.ring;
	if (ring.size() < 2) {
		return -1;
	}
	if (p_meta.is_sharp && p_transported.length() > real_t(0.9)) {
		return _get_in_plane(p_edges, p_nid, p_incoming, p_transported, !p_reversed, ring);
	}
	const int64_t idx = ring.find(p_incoming);
	if (idx < 0) {
		return _get_in_plane(p_edges, p_nid, p_incoming, p_transported, !p_reversed, ring);
	}
	const int64_t n = int64_t(ring.size());
	const int64_t step = p_reversed ? -1 : 1;
	return ring[uint32_t((idx + step + n) % n)];
}

// Port of the Lean `findCyclesPort` walk (modules/cassie/lean/CassieAvbd/
// CycleDetect/Walk.lean), itself a port of Unity CASSIE's CycleDetection.cs:
// a normal is parallel-transported along each edge and across each node,
// smooth nodes step ±1 around a CCW-sorted ring, sharp nodes pick in the
// plane of the transported normal, and at each node `reversed` is set when
// the transported normal disagrees with the node's fitted normal by more
// than 60°. Two edges closing on each other is a cycle (a lens).
Array CassieSketchGraph::find_cycles() const {
	Array out;
	const int edge_count = edges.size();
	if (edge_count < 2) {
		return out;
	}

	HashMap<int, WalkNodeMeta> meta;
	for (const KeyValue<int, Ref<CassieSketchGraphNode>> &kv : nodes) {
		WalkNodeMeta m;
		const PackedInt32Array eids = kv.value->get_edge_ids();
		LocalVector<Vector3> tangents;
		for (int i = 0; i < eids.size(); ++i) {
			if (!edges.has(eids[i])) {
				continue;
			}
			m.ring.push_back(eids[i]);
			tangents.push_back(edges[eids[i]]->get_tangent_away_from(kv.key));
		}
		if (m.ring.size() >= 2) {
			const real_t residual = _fit_plane(tangents, m.normal);
			m.is_sharp = residual > real_t(0.5);
			if (m.normal.length() > real_t(0.5)) {
				_sort_ring_ccw(edges, kv.key, m.normal, m.ring);
			}
		}
		meta.insert(kv.key, m);
	}

	HashSet<String> seen;
	for (const KeyValue<int, Ref<CassieSketchGraphEdge>> &kv : edges) {
		const int seed_eid = kv.key;
		const int starts[2] = { kv.value->get_node_a_id(), kv.value->get_node_b_id() };
		for (int side = 0; side < 2; ++side) {
			const int start_nid = starts[side];
			if (!meta.has(start_nid)) {
				continue;
			}
			LocalVector<int> path;
			HashSet<int> path_set;
			int current_eid = seed_eid;
			int current_nid = start_nid;
			Vector3 transported = meta[start_nid].normal;
			bool reversed = false;
			bool closed = false;
			const int max_steps = edge_count + 2;
			for (int step = 0; step < max_steps; ++step) {
				path.push_back(current_eid);
				path_set.insert(current_eid);
				const Ref<CassieSketchGraphEdge> cur_edge = edges[current_eid];
				const int next_nid = cur_edge->get_opposite(current_nid);
				if (!meta.has(next_nid)) {
					break;
				}
				transported = cur_edge->parallel_transport(transported, current_nid);
				reversed = transported.dot(meta[next_nid].normal) < real_t(0.5);
				const int next_eid = _next_edge_port(edges, meta[next_nid], next_nid,
						current_eid, transported, reversed);
				if (next_eid < 0) {
					break;
				}
				if (next_eid == seed_eid && next_nid == start_nid) {
					closed = path.size() >= 2;
					break;
				}
				if (path_set.has(next_eid)) {
					break;
				}
				// Unity rotates from the direction of travel into the node, which
				// is the incoming edge's tangent pointed away from it and negated;
				// the Lean model drops the negation and turns a straight-through
				// node into a half-turn about an arbitrary axis.
				const Vector3 t_in = -cur_edge->get_tangent_away_from(next_nid);
				const Vector3 t_next = edges[next_eid]->get_tangent_away_from(next_nid);
				transported = _rotate_between(t_in, t_next, transported);
				current_eid = next_eid;
				current_nid = next_nid;
			}
			if (!closed) {
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
