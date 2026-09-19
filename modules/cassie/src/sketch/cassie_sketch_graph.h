/**************************************************************************/
/*  cassie_sketch_graph.h                                                 */
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

#include "core/io/resource.h"
#include "core/math/vector3.h"
#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/variant/array.h"
#include "core/variant/typed_array.h"

// CassieSketchGraph — Tier 4 graph topology for CASSIE editing (ENG-77).
//
// Ships the data classes plus a *planar* minimal-cycle finder —
// sufficient for the verification gate of three strokes forming a
// triangle yielding exactly one cycle of three edges. The full
// CycleDetection.cs port (Rodrigues parallel transport across each
// segment + sharp/smooth node distinction + ShouldReverse normal-flip
// check) stays in the GDScript scaffold as a follow-up; this slice
// closes the topology gap end-to-end via the cheaper planar variant
// that the Yu 2021 reference reduces to when the surface normals don't
// twist along the curves.
//
// Editing-track per [ENG-69 / ENG-74-76] direction; matches the C++
// scope already in `sketch/`.

class CassieSketchGraphNode : public Resource {
	GDCLASS(CassieSketchGraphNode, Resource);

	int id = -1;
	Vector3 position;
	Vector3 normal;
	PackedInt32Array edge_ids;
	bool is_sharp = false;

protected:
	static void _bind_methods();

public:
	CassieSketchGraphNode() = default;
	void set_id(int p_id) { id = p_id; }
	int get_id() const { return id; }
	void set_position(const Vector3 &p) { position = p; }
	Vector3 get_position() const { return position; }
	void set_normal(const Vector3 &n) { normal = n; }
	Vector3 get_normal() const { return normal; }
	void set_edge_ids(const PackedInt32Array &e) { edge_ids = e; }
	PackedInt32Array get_edge_ids() const { return edge_ids; }
	int get_degree() const { return edge_ids.size(); }
	void set_is_sharp(bool s) { is_sharp = s; }
	bool get_is_sharp() const { return is_sharp; }

	void add_edge_id(int p_edge_id);
};

class CassieSketchGraphEdge : public Resource {
	GDCLASS(CassieSketchGraphEdge, Resource);

	int id = -1;
	PackedVector3Array points;
	PackedVector3Array normals;
	// Curve parameter of each point within its source stroke, so a fraction
	// along the edge samples where Segment.GetPointAt does.
	PackedFloat32Array params;
	int node_a_id = -1;
	int node_b_id = -1;
	// Populated by build_from_polylines: index into the input polyline
	// array. Lets callers map cycle → set-of-strokes for the border-set
	// diff. -1 for edges added via the per-stroke add_stroke path.
	int source_polyline_idx = -1;

protected:
	static void _bind_methods();

public:
	CassieSketchGraphEdge() = default;
	void set_id(int p_id) { id = p_id; }
	int get_id() const { return id; }
	void set_points(const PackedVector3Array &p) { points = p; }
	PackedVector3Array get_points() const { return points; }
	void set_normals(const PackedVector3Array &n) { normals = n; }
	PackedVector3Array get_normals() const { return normals; }
	void set_params(const PackedFloat32Array &p) { params = p; }
	PackedFloat32Array get_params() const { return params; }
	Vector3 get_point_at(real_t p_u) const;
	void set_node_a_id(int n) { node_a_id = n; }
	int get_node_a_id() const { return node_a_id; }
	void set_node_b_id(int n) { node_b_id = n; }
	int get_node_b_id() const { return node_b_id; }
	void set_source_polyline_idx(int n) { source_polyline_idx = n; }
	int get_source_polyline_idx() const { return source_polyline_idx; }

	int get_opposite(int node_id) const;
	Vector3 get_tangent_away_from(int node_id) const;

	// Discrete parallel transport of a vector along this edge's polyline,
	// from one endpoint node to the other. Mirror of upstream
	// Segment.Transport(v, to). Caller passes the FROM node; we walk the
	// polyline and rotate v by the smallest rotation between successive
	// tangents at each polyline vertex. Returns v unchanged if the edge
	// is too short for meaningful transport.
	Vector3 parallel_transport(const Vector3 &p_v, int p_from_node_id) const;
};

class CassieSketchGraph : public Resource {
	GDCLASS(CassieSketchGraph, Resource);

	HashMap<int, Ref<CassieSketchGraphNode>> nodes;
	HashMap<int, Ref<CassieSketchGraphEdge>> edges;
	int next_node_id = 0;
	int next_edge_id = 0;

	real_t merge_epsilon = real_t(0.02);
	real_t sharp_angle_threshold = real_t(Math::PI / 6.0); // 30°

	void _update_node_sharpness(int p_node_id);
	void _update_node_normal(int p_node_id);
	int _find_or_create_node(const Vector3 &p_pos, const Vector3 &p_normal);
	void _remove_edge(int p_edge_id);

	// A place to cut a polyline: arc length from its start, and the world
	// position both crossing polylines agree on.
	struct SplitPt {
		real_t t;
		Vector3 pos;
		Vector3 dir;
	};
	static void _cumulative_lengths(const PackedVector3Array &p_poly,
			LocalVector<real_t> &r_cum);
	static void _crossings(const PackedVector3Array &p_a,
			const PackedVector3Array &p_b, real_t p_proximity,
			LocalVector<SplitPt> &r_a, LocalVector<SplitPt> &r_b);
	int _add_polyline_sliced(const PackedVector3Array &p_poly,
			LocalVector<SplitPt> &p_splits, int p_source_idx);

public:
	struct WalkCycle {
		LocalVector<int> edges;
		LocalVector<bool> reversed;
	};
	// Cycle.Contains(s1, s2): the two edges sit next to each other in the
	// cycle, in either order, wrapping at the ends.
	static bool _cycle_adjacent(const WalkCycle &p_cycle, int p_eid_1, int p_eid_2);

private:
	// Live cycles and the two caches Graph.TryFindAllCycles reads: edges
	// laid on the stroke being committed, and cycles a split or a merge
	// touched. Both drain on update_cycles.
	HashMap<int, WalkCycle> live_cycles;
	HashMap<int, LocalVector<int>> cycles_by_edge;
	// _updatedSegmentsCache is a .NET HashSet, enumerated by slot: an erased
	// slot is refilled by the next insert, last freed first, and the seed
	// order of a search follows it.
	struct SlotSet {
		LocalVector<int> slots;
		LocalVector<uint32_t> free_slots;
		int64_t find(int p_value) const { return slots.find(p_value); }
		void insert(int p_value);
		void erase(int p_value);
		void clear();
		LocalVector<int> values() const;
	};
	SlotSet updated_edges;
	LocalVector<int> cycles_to_check;
	int next_cycle_id = 0;
	PackedInt32Array last_constraint_sources;

	static void _push_unique(LocalVector<int> &r_list, int p_value);
	bool _try_add_cycle(const LocalVector<int> &p_path, int p_start_nid, Array &r_added);
	void _remove_cycle(int p_cycle_id);
	void _repair_cycles(int p_old_eid, int p_new_eid);
	void _check_cycles_at(int p_node_id, int p_edge_id);

	int _new_node(const Vector3 &p_pos);
	int _new_edge(const PackedVector3Array &p_points, int p_node_a, int p_node_b,
			int p_source_idx, bool p_on_new_stroke, const PackedFloat32Array &p_params = PackedFloat32Array());
	static PackedFloat32Array _uniform_params(int p_count);
	static real_t _closest_on_polyline(const PackedVector3Array &p_poly,
			const Vector3 &p_pos, SplitPt &r_hit);
	int _nearest_edge(const Vector3 &p_pos, int p_source_idx, bool p_own,
			int p_target, const Vector3 &p_dir, SplitPt &r_hit) const;
	int _closer_end(int p_edge_id, const Vector3 &p_pos) const;
	void _split_edge(int p_edge_id, const SplitPt &p_at, int p_node_id, bool p_on_new_stroke);
	void _replace_node(int p_old_id, int p_keep_id);
	void _remove_stroke_edge(int p_edge_id);
	void _mend_at(int p_node_id);
	int _live_degree(int p_node_id) const;
	int _cycle_count(int p_edge_id) const;
	int _closest_edge_to(const Vector3 &p_pos, bool p_look_at_non_manifold) const;
	int _closest_neighbor_to(const Vector3 &p_pos, int p_node_id, int p_edge_id, bool p_look_at_non_manifold) const;

protected:
	static void _bind_methods();

public:
	CassieSketchGraph() = default;

	void set_merge_epsilon(real_t e) { merge_epsilon = e; }
	real_t get_merge_epsilon() const { return merge_epsilon; }

	void clear();

	// Add a stroke as a single edge. Endpoints within merge_epsilon of
	// an existing node merge into that node; otherwise new nodes are
	// created. Returns the edge id, or -1 if the input has fewer than
	// two points. NOTE: this is the simple per-stroke commit path used
	// for online drawing — it does NOT detect mid-stroke intersections
	// with previously committed edges. Offline replay of a full sketch
	// should call build_from_polylines (see below) instead, which
	// computes the entire planar arrangement in one pass.
	int add_stroke(const PackedVector3Array &p_points,
			const PackedVector3Array &p_normals);

	// One-shot planar-arrangement build for offline replay. Takes all
	// strokes' flattened polylines at once, finds every pairwise
	// segment-segment intersection within p_proximity, clusters
	// crossings within merge_epsilon into unique node positions, then
	// emits one edge per slice of each polyline between consecutive
	// intersections. Equivalent to running the upstream draw-time
	// intersection enforcement on every stroke in order, but without
	// the incremental split-and-repair bookkeeping needed for VR
	// interactivity. Clears the graph before building. Returns the
	// number of edges added.
	int build_from_polylines(const TypedArray<PackedVector3Array> &p_polylines,
			real_t p_proximity);

	// The online counterpart: adds one stroke and splits it, and every
	// existing edge it crosses within p_proximity, at the crossings.
	// Untouched edges keep their ids, so patch signatures over them
	// survive. Returns the number of edges added.
	int add_stroke_intersecting(const PackedVector3Array &p_points,
			const PackedVector3Array &p_normals, real_t p_proximity);

	// Commits one stroke the way the upstream draw controller does
	// (FinalStroke.SetCurve + AddIntersectionOldStroke/NewStroke): fresh
	// nodes at both ends and one edge, then each intersection constraint
	// in turn snaps to the nearer end node of the edge it crosses when
	// that is within p_snap, or splits that edge at the crossing; on this
	// stroke it replaces the nearer end node when within p_merge, or
	// splits this stroke. The crossed edge is the nearest one of the stroke
	// p_targets names for that constraint (its source index), or of any
	// other stroke when the entry is -1 or missing, ties going to the
	// stroke committed last. Endpoints never merge on proximity alone, so
	// two strokes meeting at a point are joined only by a constraint that
	// says so. Returns the number of constraints with no stroke within
	// p_reach; get_last_constraint_sources reports the source each one
	// attached to, -1 where it did not.
	int add_stroke_constrained(const PackedVector3Array &p_points, bool p_closed,
			const PackedVector3Array &p_intersections, const PackedInt32Array &p_targets,
			real_t p_snap, real_t p_merge, real_t p_reach, int p_source_idx);
	PackedInt32Array get_last_constraint_sources() const { return last_constraint_sources; }

	// FinalStroke.Destroy: Graph.Remove on every edge the stroke owns. Each
	// removal queues the edges beside it at both ends for the next
	// update_cycles, drops the cycles it bordered without re-seeding them,
	// and mends any joint left holding exactly two edges of one other
	// stroke back into a single edge (Node.TryRemove), unless that joint
	// closes the stroke. Returns the number of edges removed.
	int remove_stroke(int p_source_idx);

	// Graph.ManualDeletePatch: drops the live cycle over exactly these
	// edges and nothing else, so the walk does not look for it again.
	// Returns false when no live cycle has that edge set.
	bool remove_cycle(const PackedInt32Array &p_edge_ids);

	// Graph.TryFindAllCycles: drops every checked cycle that an edge laid
	// since the last update cuts through, re-seeds the walk from the first
	// edge of each dropped cycle, then from every laid edge. Returns the
	// cycles accepted by this update; get_live_cycles has the standing set.
	Array update_cycles();
	Array get_live_cycles() const;

	// Graph.TryFindCycleAt: the guided search behind a hand-placed patch.
	// Starts from the edge nearest p_pos and at each node steps to the
	// neighbor nearest it, accepting the walk if it returns to the start
	// edge within ten turns. Edges bordering two cycles are skipped unless
	// p_look_at_non_manifold. Returns the cycle accepted, as one edge list,
	// or an empty Array.
	Array find_cycle_at(const Vector3 &p_pos, bool p_look_at_non_manifold);

	int get_edge_count() const { return edges.size(); }
	int get_node_count() const { return nodes.size(); }

	Ref<CassieSketchGraphNode> get_node(int p_id) const;
	Ref<CassieSketchGraphEdge> get_edge(int p_id) const;

	TypedArray<CassieSketchGraphNode> get_all_nodes() const;
	TypedArray<CassieSketchGraphEdge> get_all_edges() const;

	// Find all minimal cycles. Each returned cycle is a list of edge
	// ids in traversal order. The planar finder works for graphs whose
	// strokes lie roughly in a common tangent plane (true for the
	// triangle gate); the GDScript scaffold retains the full normal-
	// transport variant for non-planar curve networks.
	Array find_cycles() const;

	// Sample a detected cycle's boundary as a sequence of Vector3
	// points at roughly p_target_edge_length spacing. Walks each edge
	// in traversal order (CCW per find_cycles output), flipping the
	// edge sample order at nodes where the cycle visits the edge from
	// node_b. The output is ready to feed into
	// CassieTriangulator::triangulate(boundary, edge_length).
	PackedVector3Array sample_cycle_boundary(const PackedInt32Array &p_cycle_edge_ids,
			real_t p_target_edge_length) const;
};
