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

#include "core/math/vector3.h"
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
