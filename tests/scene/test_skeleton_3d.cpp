/**************************************************************************/
/*  test_skeleton_3d.cpp                                                  */
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

TEST_FORCE_LINK(test_skeleton_3d)

#ifndef _3D_DISABLED

#include "scene/3d/skeleton_3d.h"

namespace TestSkeleton3D {

TEST_CASE("[Skeleton3D] Test per-bone meta") {
	Skeleton3D *skeleton = memnew(Skeleton3D);
	skeleton->add_bone("root");
	skeleton->set_bone_rest(0, Transform3D());

	// Adding meta to bone.
	skeleton->set_bone_meta(0, "key1", "value1");
	skeleton->set_bone_meta(0, "key2", 12345);
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key1") == "value1", "Bone meta missing.");
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key2") == Variant(12345), "Bone meta missing.");

	// Rename bone and check if meta persists.
	skeleton->set_bone_name(0, "renamed_root");
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key1") == "value1", "Bone meta missing.");
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key2") == Variant(12345), "Bone meta missing.");

	// Retrieve list of keys.
	List<StringName> keys;
	skeleton->get_bone_meta_list(0, &keys);
	CHECK_MESSAGE(keys.size() == 2, "Wrong number of bone meta keys.");
	CHECK_MESSAGE(keys.find("key1"), "key1 not found in bone meta list");
	CHECK_MESSAGE(keys.find("key2"), "key2 not found in bone meta list");

	// Removing meta.
	skeleton->set_bone_meta(0, "key1", Variant());
	skeleton->set_bone_meta(0, "key2", Variant());
	CHECK_MESSAGE(!skeleton->has_bone_meta(0, "key1"), "Bone meta key1 should be deleted.");
	CHECK_MESSAGE(!skeleton->has_bone_meta(0, "key2"), "Bone meta key2 should be deleted.");
	List<StringName> should_be_empty_keys;
	skeleton->get_bone_meta_list(0, &should_be_empty_keys);
	CHECK_MESSAGE(should_be_empty_keys.size() == 0, "Wrong number of bone meta keys.");

	// Deleting non-existing key should succeed.
	skeleton->set_bone_meta(0, "non-existing-key", Variant());
	memdelete(skeleton);
}

TEST_CASE("[Skeleton3D] Global poses and rests with many root bones") {
	Skeleton3D *skeleton = memnew(Skeleton3D);
	const int roots = 64;
	for (int i = 0; i < roots; i++) {
		int r = skeleton->get_bone_count();
		skeleton->add_bone(vformat("root%d", i));
		skeleton->add_bone(vformat("child%d", i));
		skeleton->set_bone_parent(r + 1, r);
		skeleton->set_bone_rest(r, Transform3D(Basis(), Vector3(i, 0, 0)));
		skeleton->set_bone_rest(r + 1, Transform3D(Basis(), Vector3(0, 1, 0)));
		skeleton->set_bone_pose(r, Transform3D(Basis(Vector3(0, 1, 0), 0.1 * i), Vector3(i, 2, 0)));
		skeleton->set_bone_pose(r + 1, Transform3D(Basis(Vector3(1, 0, 0), 0.2), Vector3(0, 0.5, 0)));
	}
	for (int i = 0; i < roots; i++) {
		int r = 2 * i;
		Transform3D root_pose = skeleton->get_bone_pose(r);
		CHECK(skeleton->get_bone_global_pose(r).is_equal_approx(root_pose));
		CHECK(skeleton->get_bone_global_pose(r + 1).is_equal_approx(root_pose * skeleton->get_bone_pose(r + 1)));
	}

	for (int i = 0; i < roots; i++) {
		skeleton->set_bone_rest(2 * i, Transform3D(Basis(), Vector3(i, 3, 0)));
	}
	skeleton->set_bone_pose(2 * (roots - 1), Transform3D(Basis(), Vector3(-1, -1, -1)));
	for (int i = 0; i < roots; i++) {
		int r = 2 * i;
		CHECK(skeleton->get_bone_global_rest(r + 1).is_equal_approx(Transform3D(Basis(), Vector3(i, 4, 0))));
		CHECK(skeleton->get_bone_global_pose(r + 1).is_equal_approx(skeleton->get_bone_pose(r) * skeleton->get_bone_pose(r + 1)));
	}
	memdelete(skeleton);
}

} // namespace TestSkeleton3D

#endif // _3D_DISABLED
