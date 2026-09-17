/**************************************************************************/
/*  test_cassie_sketch_graph.h                                            */
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

#include "../src/cassie_triangulator.h"
#include "../src/sketch/cassie_sketch_graph.h"

#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/templates/hash_map.h"
#include "tests/test_macros.h"

namespace TestCassieSketchGraph {

static PackedVector3Array _segment(const Vector3 &a, const Vector3 &b,
		int p_samples = 8) {
	PackedVector3Array pts;
	pts.resize(p_samples);
	for (int i = 0; i < p_samples; ++i) {
		const real_t t = real_t(i) / real_t(p_samples - 1);
		pts.write[i] = a.lerp(b, t);
	}
	return pts;
}

static PackedVector3Array _up_normals(int p_count) {
	PackedVector3Array out;
	out.resize(p_count);
	for (int i = 0; i < p_count; ++i) {
		out.write[i] = Vector3(0, 0, 1); // strokes lie in the xy plane
	}
	return out;
}

TEST_CASE("[Cassie][SketchGraph] empty graph has no edges or cycles") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	CHECK_EQ(g->get_node_count(), 0);
	CHECK_EQ(g->get_edge_count(), 0);
	CHECK_EQ(g->find_cycles().size(), 0);
}

TEST_CASE("[Cassie][SketchGraph] add_stroke creates two nodes and one edge") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	const PackedVector3Array pts = _segment(Vector3(0, 0, 0), Vector3(1, 0, 0));
	const int eid = g->add_stroke(pts, _up_normals(pts.size()));
	CHECK(eid >= 0);
	CHECK_EQ(g->get_edge_count(), 1);
	CHECK_EQ(g->get_node_count(), 2);
}

TEST_CASE("[Cassie][SketchGraph] coincident endpoint merges into the same node") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	const Vector3 shared(0.5, 0.5, 0);
	const PackedVector3Array a = _segment(Vector3(0, 0, 0), shared);
	const PackedVector3Array b = _segment(shared, Vector3(1, 1, 0));
	g->add_stroke(a, _up_normals(a.size()));
	g->add_stroke(b, _up_normals(b.size()));
	CHECK_MESSAGE(g->get_node_count() == 3,
			vformat("two strokes sharing one endpoint should yield 3 nodes; got %d",
					g->get_node_count()));
	CHECK_EQ(g->get_edge_count(), 2);
}

TEST_CASE("[Cassie][SketchGraph] three strokes forming a triangle yield exactly one cycle of three edges") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	// Equilateral-ish triangle in the xy plane.
	const Vector3 v0(0, 0, 0);
	const Vector3 v1(1, 0, 0);
	const Vector3 v2(0.5, real_t(0.866), 0); // ~ √3/2
	const PackedVector3Array e0 = _segment(v0, v1);
	const PackedVector3Array e1 = _segment(v1, v2);
	const PackedVector3Array e2 = _segment(v2, v0);
	g->add_stroke(e0, _up_normals(e0.size()));
	g->add_stroke(e1, _up_normals(e1.size()));
	g->add_stroke(e2, _up_normals(e2.size()));

	CHECK_MESSAGE(g->get_node_count() == 3,
			vformat("triangle should yield 3 nodes; got %d",
					g->get_node_count()));
	CHECK_MESSAGE(g->get_edge_count() == 3,
			vformat("triangle should yield 3 edges; got %d",
					g->get_edge_count()));

	const Array cycles = g->find_cycles();
	REQUIRE_MESSAGE(cycles.size() >= 1,
			"closed-triangle graph must detect at least one cycle");
	const PackedInt32Array first_cycle = cycles[0];
	CHECK_MESSAGE(first_cycle.size() == 3,
			vformat("first detected cycle should walk 3 edges; got %d",
					first_cycle.size()));
}

TEST_CASE("[Cassie][SketchGraph] add_stroke_intersecting splits three overshooting strokes into a cycle") {
	// The pen-demo run of 2026-09-17: three strokes that overshoot each
	// corner. Each pair crosses once, so the arrangement is 3 crossing
	// nodes + 6 free ends, 9 edges, and one 3-edge cycle.
	const PackedVector3Array s0 = _segment(Vector3(-1.2, 0, 0), Vector3(1.2, 0, 0), 16);
	const PackedVector3Array s1 = _segment(Vector3(1.1, -0.3, 0), Vector3(-0.2, 1.8, 0), 16);
	const PackedVector3Array s2 = _segment(Vector3(0.2, 1.8, 0), Vector3(-1.1, -0.3, 0), 16);

	Ref<CassieSketchGraph> g;
	g.instantiate();
	int added = 0;
	added += g->add_stroke_intersecting(s0, _up_normals(s0.size()), 0.02);
	added += g->add_stroke_intersecting(s1, _up_normals(s1.size()), 0.02);
	added += g->add_stroke_intersecting(s2, _up_normals(s2.size()), 0.02);
	CHECK_EQ(added, 9);
	CHECK_EQ(g->get_edge_count(), 9);
	CHECK_EQ(g->get_node_count(), 9);
	const Array cycles = g->find_cycles();
	REQUIRE_MESSAGE(cycles.size() >= 1, "crossing strokes must close one cycle");
	const PackedInt32Array cycle = cycles[0];
	CHECK_EQ(cycle.size(), 3);

	// Control: the endpoint-merge path on the same strokes finds nothing.
	Ref<CassieSketchGraph> plain;
	plain.instantiate();
	plain->add_stroke(s0, _up_normals(s0.size()));
	plain->add_stroke(s1, _up_normals(s1.size()));
	plain->add_stroke(s2, _up_normals(s2.size()));
	CHECK_EQ(plain->get_edge_count(), 3);
	CHECK_EQ(plain->get_node_count(), 6);
	CHECK_EQ(plain->find_cycles().size(), 0);
}

TEST_CASE("[Cassie][SketchGraph] add_stroke_intersecting keeps ids of edges it does not cross") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	const PackedVector3Array far_away = _segment(Vector3(5, 5, 0), Vector3(6, 5, 0));
	const PackedVector3Array base = _segment(Vector3(-1, 0, 0), Vector3(1, 0, 0));
	const PackedVector3Array cross = _segment(Vector3(0, -1, 0), Vector3(0, 1, 0));
	g->add_stroke_intersecting(far_away, _up_normals(far_away.size()), 0.02);
	g->add_stroke_intersecting(base, _up_normals(base.size()), 0.02);
	CHECK(g->get_edge(0).is_valid());
	CHECK(g->get_edge(1).is_valid());
	g->add_stroke_intersecting(cross, _up_normals(cross.size()), 0.02);
	CHECK_MESSAGE(g->get_edge(0).is_valid(), "the untouched edge keeps id 0");
	CHECK_MESSAGE(g->get_edge(1).is_null(), "the crossed edge is replaced by its halves");
	CHECK_EQ(g->get_edge_count(), 5);
	CHECK_EQ(g->get_node_count(), 7);
	CHECK_EQ(g->get_node(g->get_edge(2)->get_node_b_id())->get_degree(), 4);
}

TEST_CASE("[Cassie][SketchGraph] triangle cycle → sample_cycle_boundary → CassieTriangulator produces a valid mesh") {
	// End-to-end: the cycle→patch path. Three strokes form a triangle,
	// find_cycles returns one cycle, sample_cycle_boundary emits a CCW
	// boundary, and CassieTriangulator turns it into an ArrayMesh.
	Ref<CassieSketchGraph> g;
	g.instantiate();
	const Vector3 v0(0, 0, 0);
	const Vector3 v1(1, 0, 0);
	const Vector3 v2(real_t(0.5), real_t(0.866), 0);
	const PackedVector3Array e0 = _segment(v0, v1, 16);
	const PackedVector3Array e1 = _segment(v1, v2, 16);
	const PackedVector3Array e2 = _segment(v2, v0, 16);
	g->add_stroke(e0, _up_normals(e0.size()));
	g->add_stroke(e1, _up_normals(e1.size()));
	g->add_stroke(e2, _up_normals(e2.size()));

	const Array cycles = g->find_cycles();
	REQUIRE(cycles.size() >= 1);
	const PackedInt32Array cycle = cycles[0];
	REQUIRE_EQ(cycle.size(), 3);

	const real_t target_edge_length = real_t(0.15);
	const PackedVector3Array boundary =
			g->sample_cycle_boundary(cycle, target_edge_length);
	REQUIRE_MESSAGE(boundary.size() >= 9,
			vformat("triangle at edge_length=0.15 should yield >= 9 boundary pts; got %d",
					boundary.size()));

	// Boundary must be planar (all in z=0 within fp tolerance).
	for (int i = 0; i < boundary.size(); ++i) {
		CHECK_MESSAGE(Math::abs(boundary[i].z) < real_t(1e-5),
				vformat("boundary pt %d off plane: z=%f", i, double(boundary[i].z)));
	}

	Dictionary mesh = CassieTriangulator::triangulate(boundary, target_edge_length);
	REQUIRE_MESSAGE(bool(mesh.get("success", false)),
			"CassieTriangulator should produce a valid mesh from the triangle boundary");
	const PackedVector3Array verts = mesh["vertices"];
	const PackedInt32Array faces = mesh["faces"];
	CHECK_MESSAGE(verts.size() >= 3,
			vformat("mesh should have at least 3 vertices; got %d", verts.size()));
	CHECK_MESSAGE(faces.size() >= 3,
			vformat("mesh should have >= 3 face-indices; got %d", faces.size()));
	CHECK_MESSAGE(faces.size() % 3 == 0,
			vformat("face-index count should be a multiple of 3; got %d",
					faces.size()));
	// Each face index must be in range.
	for (int i = 0; i < faces.size(); ++i) {
		const int fi = faces[i];
		CHECK_MESSAGE(fi >= 0,
				vformat("face index %d is negative: %d", i, fi));
		CHECK_MESSAGE(fi < verts.size(),
				vformat("face index %d (%d) >= vertex count %d",
						i, fi, verts.size()));
	}
}

static PackedVector3Array _quarter_arc(int p_samples = 16) {
	PackedVector3Array pts;
	pts.resize(p_samples);
	for (int i = 0; i < p_samples; ++i) {
		const real_t a = real_t(Math::PI) * real_t(0.5) * real_t(i) / real_t(p_samples - 1);
		pts.write[i] = Vector3(Math::cos(a), Math::sin(a), 0);
	}
	return pts;
}

TEST_CASE("[Cassie][SketchGraph] parallel_transport turns with the tangent and leaves the plane normal alone") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	const int arc = g->add_stroke(_quarter_arc(), _up_normals(16));
	const int line = g->add_stroke(_segment(Vector3(3, 0, 0), Vector3(4, 0, 0)), _up_normals(8));
	REQUIRE(arc >= 0);
	REQUIRE(line >= 0);
	const Ref<CassieSketchGraphEdge> e = g->get_edge(arc);
	const int from = e->get_node_a_id();

	// The radial at the arc's start is the radial at its end, less the
	// half-sample the chord tangents lose at each end (about 3 degrees here).
	const Vector3 radial = e->parallel_transport(Vector3(1, 0, 0), from);
	CHECK(radial.dot(Vector3(0, 1, 0)) > real_t(0.99));
	CHECK(Math::is_equal_approx(radial.length(), real_t(1)));
	const Vector3 up = e->parallel_transport(Vector3(0, 0, 1), from);
	CHECK(up.is_equal_approx(Vector3(0, 0, 1)));

	// Transporting the other way undoes it.
	const Vector3 back = e->parallel_transport(radial, e->get_node_b_id());
	CHECK(back.is_equal_approx(Vector3(1, 0, 0)));

	// Control: a straight edge has nothing to turn by.
	const Ref<CassieSketchGraphEdge> l = g->get_edge(line);
	CHECK(l->parallel_transport(Vector3(0, 1, 0), l->get_node_a_id()).is_equal_approx(Vector3(0, 1, 0)));
}

// Straight-edged fixtures with a known face count. Every node's fitted
// normal is a vertex normal rather than a face normal, so these exercise
// the sharp branch (cube, residual 1/sqrt(3)) and the smooth branch (tet,
// residual 1/3) of the walk, and the grid exercises the arrangement.
struct WireFixture {
	const char *name;
	int faces;
	int face_edges;
	TypedArray<PackedVector3Array> strokes;
};

static TypedArray<PackedVector3Array> _wire(const Vector3 *p_v, const int (*p_pair)[2], int p_count) {
	TypedArray<PackedVector3Array> out;
	for (int i = 0; i < p_count; ++i) {
		out.push_back(_segment(p_v[p_pair[i][0]], p_v[p_pair[i][1]]));
	}
	return out;
}

static WireFixture _tet_fixture() {
	const Vector3 v[4] = { Vector3(1, 1, 1), Vector3(1, -1, -1), Vector3(-1, 1, -1), Vector3(-1, -1, 1) };
	const int pr[6][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 2 }, { 1, 3 }, { 2, 3 } };
	return WireFixture{ "tet", 4, 3, _wire(v, pr, 6) };
}

static WireFixture _cube_fixture() {
	const Vector3 v[8] = { Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0), Vector3(0, 1, 0), Vector3(0, 0, 1), Vector3(1, 0, 1), Vector3(1, 1, 1), Vector3(0, 1, 1) };
	const int pr[12][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
	return WireFixture{ "cube", 6, 4, _wire(v, pr, 12) };
}

// Three horizontal and three vertical lines: four cells, and the walk also
// closes the outer boundary once, as an 8-edge loop.
static WireFixture _grid_fixture() {
	TypedArray<PackedVector3Array> lines;
	for (int i = 0; i <= 2; ++i) {
		lines.push_back(_segment(Vector3(0, i, 0), Vector3(2, i, 0)));
		lines.push_back(_segment(Vector3(i, 0, 0), Vector3(i, 2, 0)));
	}
	return WireFixture{ "grid", 4, 4, lines };
}

static Ref<CassieSketchGraph> _build(const TypedArray<PackedVector3Array> &p_strokes) {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	g->build_from_polylines(p_strokes, real_t(0.02));
	return g;
}

static int _count_cycles_of_size(const Array &p_cycles, int p_size) {
	int n = 0;
	for (int i = 0; i < p_cycles.size(); ++i) {
		n += PackedInt32Array(p_cycles[i]).size() == p_size ? 1 : 0;
	}
	return n;
}

static TypedArray<PackedVector3Array> _transformed(const TypedArray<PackedVector3Array> &p_strokes, const Transform3D &p_xf) {
	TypedArray<PackedVector3Array> out;
	for (int i = 0; i < p_strokes.size(); ++i) {
		PackedVector3Array poly = p_strokes[i];
		for (int k = 0; k < poly.size(); ++k) {
			poly.write[k] = p_xf.xform(poly[k]);
		}
		out.push_back(poly);
	}
	return out;
}

// Every edge of a closed wireframe borders exactly two of its faces.
static int _edges_used_twice(const Array &p_cycles, int p_face_edges) {
	HashMap<int, int> uses;
	for (int i = 0; i < p_cycles.size(); ++i) {
		const PackedInt32Array c = p_cycles[i];
		if (c.size() != p_face_edges) {
			continue;
		}
		for (int k = 0; k < c.size(); ++k) {
			uses[c[k]] = uses.has(c[k]) ? uses[c[k]] + 1 : 1;
		}
	}
	int twice = 0;
	for (const KeyValue<int, int> &kv : uses) {
		twice += kv.value == 2 ? 1 : 0;
	}
	return twice;
}

TEST_CASE("[Cassie][SketchGraph] tetrahedron and cube close every face and nothing else") {
	const WireFixture fixtures[2] = { _tet_fixture(), _cube_fixture() };
	for (int f = 0; f < 2; ++f) {
		const WireFixture &w = fixtures[f];
		INFO(w.name);
		const Ref<CassieSketchGraph> g = _build(w.strokes);
		REQUIRE_EQ(g->get_edge_count(), w.strokes.size());
		const Array cycles = g->find_cycles();
		CHECK_EQ(cycles.size(), w.faces);
		CHECK_EQ(_count_cycles_of_size(cycles, w.face_edges), w.faces);
		CHECK_EQ(_edges_used_twice(cycles, w.face_edges), w.strokes.size());

		// Control: dropping one edge opens the two faces it bordered.
		TypedArray<PackedVector3Array> cut = w.strokes.duplicate();
		cut.remove_at(cut.size() - 1);
		const Array cut_cycles = _build(cut)->find_cycles();
		CHECK_EQ(_count_cycles_of_size(cut_cycles, w.face_edges), w.faces - 2);
	}
}

TEST_CASE("[Cassie][SketchGraph] planar grid closes its four cells") {
	const WireFixture w = _grid_fixture();
	const Ref<CassieSketchGraph> g = _build(w.strokes);
	REQUIRE_EQ(g->get_node_count(), 9);
	REQUIRE_EQ(g->get_edge_count(), 12);
	const Array cycles = g->find_cycles();
	CHECK_EQ(_count_cycles_of_size(cycles, 4), 4);
	CHECK_EQ(_count_cycles_of_size(cycles, 8), 1);
	CHECK_EQ(cycles.size(), 5);

	// Control: without the middle vertical line the rows are two cells and
	// the outer boundary is six edges.
	TypedArray<PackedVector3Array> cut = w.strokes.duplicate();
	cut.remove_at(3);
	const Array cut_cycles = _build(cut)->find_cycles();
	CHECK_EQ(_count_cycles_of_size(cut_cycles, 4), 2);
	CHECK_EQ(_count_cycles_of_size(cut_cycles, 6), 1);
	CHECK_EQ(cut_cycles.size(), 3);
}

// Properties: the cycle set is a function of the sketch's shape, so a rigid
// motion, a mirror, a uniform scale or a reordering of the strokes leaves
// the count and the size multiset alone. The control is a flattening, which
// changes the shape and must change the answer.
static String _size_multiset(const Array &p_cycles) {
	Vector<int> sizes;
	for (int i = 0; i < p_cycles.size(); ++i) {
		sizes.push_back(PackedInt32Array(p_cycles[i]).size());
	}
	sizes.sort();
	String out;
	for (int i = 0; i < sizes.size(); ++i) {
		out += itos(sizes[i]) + ",";
	}
	return out;
}

TEST_CASE("[Cassie][SketchGraph] cycle set is invariant under rigid motion, mirroring, scale and stroke order") {
	const WireFixture fixtures[3] = { _tet_fixture(), _cube_fixture(), _grid_fixture() };
	const Transform3D rigid(Basis(Vector3(0.3, -0.8, 0.5).normalized(), real_t(1.1)), Vector3(4, -2, 7));
	const Transform3D mirror(Basis().scaled(Vector3(-1, 1, 1)), Vector3());
	const Transform3D scale(Basis().scaled(Vector3(3, 3, 3)), Vector3());
	const Transform3D flatten(Basis().scaled(Vector3(1, 1, 0)), Vector3());
	for (int f = 0; f < 3; ++f) {
		const WireFixture &w = fixtures[f];
		INFO(w.name);
		const Array base = _build(w.strokes)->find_cycles();
		REQUIRE(base.size() > 0);
		const String expected = _size_multiset(base);
		CHECK_EQ(_size_multiset(_build(_transformed(w.strokes, rigid))->find_cycles()), expected);
		CHECK_EQ(_size_multiset(_build(_transformed(w.strokes, mirror))->find_cycles()), expected);
		CHECK_EQ(_size_multiset(_build(_transformed(w.strokes, scale))->find_cycles()), expected);

		TypedArray<PackedVector3Array> reordered;
		for (int i = w.strokes.size() - 1; i >= 0; --i) {
			reordered.push_back(w.strokes[i]);
		}
		CHECK_EQ(_size_multiset(_build(reordered)->find_cycles()), expected);

		if (f < 2) {
			CHECK_NE(_size_multiset(_build(_transformed(w.strokes, flatten))->find_cycles()), expected);
		}
	}
}

// Twelve of the hat capture's 234 patches are bordered by two strokes, so a
// two-edge lens is a cycle.
TEST_CASE("[Cassie][SketchGraph] a lens of two strokes between two nodes is a cycle") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	PackedVector3Array upper;
	PackedVector3Array lower;
	for (int i = 0; i < 12; ++i) {
		const real_t t = real_t(i) / real_t(11);
		const real_t bulge = Math::sin(t * real_t(Math::PI)) * real_t(0.3);
		upper.push_back(Vector3(t, bulge, 0));
		lower.push_back(Vector3(t, -bulge, 0));
	}
	g->add_stroke(upper, _up_normals(12));
	g->add_stroke(lower, _up_normals(12));
	REQUIRE_EQ(g->get_node_count(), 2);
	const Array cycles = g->find_cycles();
	CHECK_EQ(cycles.size(), 1);
	CHECK_EQ(_count_cycles_of_size(cycles, 2), 1);

	// Control: the same two strokes without the shared far endpoint.
	Ref<CassieSketchGraph> open;
	open.instantiate();
	lower.write[11] = Vector3(1.2, 0, 0);
	open->add_stroke(upper, _up_normals(12));
	open->add_stroke(lower, _up_normals(12));
	REQUIRE_EQ(open->get_node_count(), 3);
	CHECK_EQ(open->find_cycles().size(), 0);
}

TEST_CASE("[Cassie][SketchGraph] two strokes form no cycle") {
	Ref<CassieSketchGraph> g;
	g.instantiate();
	const Vector3 shared(0.5, 0.5, 0);
	const PackedVector3Array a = _segment(Vector3(0, 0, 0), shared);
	const PackedVector3Array b = _segment(shared, Vector3(1, 1, 0));
	g->add_stroke(a, _up_normals(a.size()));
	g->add_stroke(b, _up_normals(b.size()));
	CHECK_EQ(g->find_cycles().size(), 0);
}

} // namespace TestCassieSketchGraph
