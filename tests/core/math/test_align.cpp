/**************************************************************************/
/*  test_align.cpp                                                        */
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

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_align)

#include "core/math/basis.h"
#include "core/math/transform_3d.h"

namespace TestAlign {

static Basis rot(const Vector3 &p_axis, real_t p_angle) {
	return Basis(p_axis.normalized(), p_angle);
}

TEST_CASE("[Align] Three pairs recover the rotation") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(1, 0, 0));
	sources.push_back(Vector3(0, 1, 0));
	sources.push_back(Vector3(0, 0, 1));
	Basis rots[4] = {
		rot(Vector3(0, 1, 0), Math::deg_to_rad((real_t)37.0)),
		rot(Vector3(1, 1, 0), Math::deg_to_rad((real_t)120.0)),
		rot(Vector3(1, 2, 3), Math::deg_to_rad((real_t)200.0)),
		rot(Vector3(-2, 1, 4), Math::deg_to_rad((real_t)-95.0)),
	};
	for (int r = 0; r < 4; r++) {
		Vector<Vector3> targets;
		for (int i = 0; i < 3; i++) {
			targets.push_back(rots[r].xform(sources[i]));
		}
		Basis rotation = Basis::align(targets, sources);
		CHECK(rotation.is_rotation());
		for (int i = 0; i < 3; i++) {
			CHECK(rotation.xform(sources[i]).distance_to(targets[i]) < (real_t)1e-3);
		}
	}
}

TEST_CASE("[Align] Single pair maps the source onto the target") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(0.3, -0.7, 0.5));
	Vector<Vector3> targets;
	targets.push_back(rot(Vector3(1, 0, 0), Math::deg_to_rad((real_t)80.0)).xform(sources[0]));
	Basis rotation = Basis::align(targets, sources);
	CHECK(rotation.is_rotation());
	CHECK(rotation.xform(sources[0]).normalized().distance_to(targets[0].normalized()) < (real_t)1e-4);
}

TEST_CASE("[Align] Antipodal single pair is a valid 180 degree rotation") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(0, 0, 1));
	Vector<Vector3> targets;
	targets.push_back(-sources[0]);
	Basis rotation = Basis::align(targets, sources);
	CHECK(rotation.is_rotation());
	CHECK(rotation.xform(sources[0]).normalized().distance_to(targets[0].normalized()) < (real_t)1e-3);
}

TEST_CASE("[Align] Rank-deficient covariance still yields a rotation") {
	// Collinear sources make the covariance rank 1; the fallback must still rotate.
	Basis twist = rot(Vector3(1, 0, 0), Math::deg_to_rad((real_t)40.0));
	Vector<Vector3> sources;
	Vector<Vector3> targets;
	for (int i = 1; i <= 3; i++) {
		Vector3 source = Vector3(0, 0, 1) * (real_t)i;
		sources.push_back(source);
		targets.push_back(twist.xform(source));
	}
	Basis rotation = Basis::align(targets, sources);
	CHECK(rotation.is_rotation());
}

TEST_CASE("[Align] Transform recovers a pure translation") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(1, 0, 0));
	sources.push_back(Vector3(0, 1, 0));
	sources.push_back(Vector3(0, 0, 1));
	Vector3 offset(2.5, -1.0, 4.0);
	Vector<Vector3> targets;
	for (int i = 0; i < sources.size(); i++) {
		targets.push_back(sources[i] + offset);
	}
	Transform3D fit = Transform3D::align(targets, sources);
	CHECK(fit.basis.is_rotation());
	CHECK(fit.origin.distance_to(offset) < (real_t)1e-4);
	for (int i = 0; i < sources.size(); i++) {
		CHECK(fit.xform(sources[i]).distance_to(targets[i]) < (real_t)1e-3);
	}
}

TEST_CASE("[Align] Transform recovers a pure rotation") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(1, 0, 0));
	sources.push_back(Vector3(0, 1, 0));
	sources.push_back(Vector3(0, 0, 1));
	Basis rotation = rot(Vector3(-2, 1, 4), Math::deg_to_rad((real_t)73.0));
	Vector<Vector3> targets;
	for (int i = 0; i < sources.size(); i++) {
		targets.push_back(rotation.xform(sources[i]));
	}
	Transform3D fit = Transform3D::align(targets, sources);
	CHECK(fit.basis.is_rotation());
	CHECK(fit.origin.length() < (real_t)1e-3);
	for (int i = 0; i < sources.size(); i++) {
		CHECK(fit.xform(sources[i]).distance_to(targets[i]) < (real_t)1e-3);
	}
}

TEST_CASE("[Align] Transform recovers a rotation and a translation") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(1, 0, 0));
	sources.push_back(Vector3(0, 1, 0));
	sources.push_back(Vector3(0, 0, 1));
	sources.push_back(Vector3(1, 2, 3));
	Basis rotation = rot(Vector3(1, 2, 3), Math::deg_to_rad((real_t)115.0));
	Vector3 offset(-3.0, 0.5, 2.0);
	Vector<Vector3> targets;
	for (int i = 0; i < sources.size(); i++) {
		targets.push_back(rotation.xform(sources[i]) + offset);
	}
	Transform3D fit = Transform3D::align(targets, sources);
	CHECK(fit.basis.is_rotation());
	for (int i = 0; i < sources.size(); i++) {
		CHECK(fit.xform(sources[i]).distance_to(targets[i]) < (real_t)1e-3);
	}
}

TEST_CASE("[Align] Transform of a single pair is that translation") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(0.4, -0.2, 0.7));
	Vector<Vector3> targets;
	targets.push_back(Vector3(1.0, 1.0, 1.0));
	Transform3D fit = Transform3D::align(targets, sources);
	CHECK(fit.basis.is_rotation());
	CHECK(fit.xform(sources[0]).distance_to(targets[0]) < (real_t)1e-4);
}

TEST_CASE("[Align] Align is deterministic across repeated calls") {
	Vector<Vector3> sources;
	sources.push_back(Vector3(1, 0, 0));
	sources.push_back(Vector3(0, 1, 0));
	sources.push_back(Vector3(0, 0, 1));
	sources.push_back(Vector3(1, 2, 3));
	Basis rotation = rot(Vector3(1, 2, 3), Math::deg_to_rad((real_t)200.0));
	Vector3 offset(-3.0, 0.5, 2.0);
	Vector<Vector3> targets;
	for (int i = 0; i < sources.size(); i++) {
		targets.push_back(rotation.xform(sources[i]) + offset);
	}
	// Same input must give a bit-identical result, not merely an approximate one.
	Basis basis_first = Basis::align(targets, sources);
	Basis basis_second = Basis::align(targets, sources);
	CHECK(basis_first == basis_second);
	Transform3D xform_first = Transform3D::align(targets, sources);
	Transform3D xform_second = Transform3D::align(targets, sources);
	CHECK(xform_first == xform_second);
}

} // namespace TestAlign
