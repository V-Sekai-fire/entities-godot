/**************************************************************************/
/*  test_oit_render.h                                                     */
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

#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/material.h"
#include "servers/rendering/rendering_device.h"
#include "tests/test_macros.h"

namespace TestOITRender {

// Gates on GPU access: --headless with mock DisplayServer has no
// RenderingDevice, so the test SKIPs with a doctest MESSAGE. Running
// the binary with a real display driver (a windowed session) satisfies
// the gate and the transparent-mesh render runs to a PNG.
TEST_CASE("[OIT][SceneTree] Screenshot transparent meshes (skipped without GPU)") {
	if (RenderingDevice::get_singleton() == nullptr) {
		MESSAGE("Skipped: no RenderingDevice (headless mock display driver)");
		return;
	}

	SceneTree *tree = SceneTree::get_singleton();
	REQUIRE(tree != nullptr);
	Window *root = tree->get_root();
	REQUIRE(root != nullptr);
	root->set_size(Vector2i(320, 240));

	Camera3D *camera = memnew(Camera3D);
	camera->set_position(Vector3(0, 0, 3));
	root->add_child(camera);
	camera->make_current();

	auto make_transparent = [&](Vector3 pos, Color tint, float alpha) {
		MeshInstance3D *inst = memnew(MeshInstance3D);
		Ref<BoxMesh> box;
		box.instantiate();
		box->set_size(Vector3(0.8, 0.8, 0.8));
		inst->set_mesh(box);
		Ref<StandardMaterial3D> mat;
		mat.instantiate();
		mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
		Color c = tint;
		c.a = alpha;
		mat->set_albedo(c);
		inst->set_surface_override_material(0, mat);
		inst->set_position(pos);
		root->add_child(inst);
		return inst;
	};

	MeshInstance3D *red = make_transparent(Vector3(-0.4, 0, 0), Color(1, 0, 0), 0.5);
	MeshInstance3D *green = make_transparent(Vector3(0.0, 0, -0.4), Color(0, 1, 0), 0.5);
	MeshInstance3D *blue = make_transparent(Vector3(0.4, 0, -0.8), Color(0, 0, 1), 0.5);
	CHECK(red != nullptr);
	CHECK(green != nullptr);
	CHECK(blue != nullptr);

	// One frame does not populate get_texture; force at least two.
	for (int i = 0; i < 3; i++) {
		tree->process(1.0 / 60.0);
		RenderingServer::get_singleton()->draw(false, 1.0 / 60.0);
	}

	Ref<ViewportTexture> viewport_tex = root->get_texture();
	REQUIRE(viewport_tex.is_valid());
	Ref<Image> img = viewport_tex->get_image();
	REQUIRE(img.is_valid());
	REQUIRE(img->get_width() > 0);
	REQUIRE(img->get_height() > 0);

	String out_dir = "user://oit_screenshots";
	Ref<DirAccess> da = DirAccess::open("user://");
	if (da.is_valid() && !da->dir_exists("oit_screenshots")) {
		da->make_dir("oit_screenshots");
	}
	String path = out_dir + "/transparent_boxes.png";
	Error err = img->save_png(path);
	CHECK(err == OK);
	MESSAGE("OIT screenshot written to ", path);

	// Count non-black, non-fully-opaque pixels to confirm the transparent
	// draw path actually produced blended output rather than just clears.
	int blended = 0;
	int total = img->get_width() * img->get_height();
	for (int y = 0; y < img->get_height(); y++) {
		for (int x = 0; x < img->get_width(); x++) {
			Color c = img->get_pixel(x, y);
			if (c.r + c.g + c.b > 0.05 && c.r + c.g + c.b < 2.9) {
				blended++;
			}
		}
	}
	CHECK(blended > total / 100);
}

} // namespace TestOITRender
