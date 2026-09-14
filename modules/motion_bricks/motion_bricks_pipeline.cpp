/**************************************************************************/
/*  motion_bricks_pipeline.cpp                                            */
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

#include "motion_bricks_pipeline.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <motionbricks/motionbricks.h>

MotionBricksPipeline::MotionBricksPipeline() {}

MotionBricksPipeline::~MotionBricksPipeline() {
	unload();
}

bool MotionBricksPipeline::load(const Dictionary &p_opts) {
	if (model) {
		unload();
	}

	ERR_FAIL_COND_V_MSG(!p_opts.has("bundle_directory"), false, "motion_bricks: opts must have bundle_directory.");

	CharString s_bundle = String(p_opts["bundle_directory"]).utf8();

	mb_runtime_options *rt = nullptr;
	char err[512] = { 0 };
	mb_status rs = mb_runtime_options_create(&rt, err, sizeof(err));
	if (rs != MB_OK) {
		ERR_FAIL_V_MSG(false, String("motion_bricks: runtime options create failed: ") + err);
	}
	if (p_opts.has("device")) {
		mb_runtime_options_set_device(rt, (mb_device)(int)p_opts["device"], err, sizeof(err));
	}
	if (p_opts.has("threads")) {
		mb_runtime_options_set_threads(rt, (uint32_t)(int)p_opts["threads"], err, sizeof(err));
	}

	rs = mb_model_load(s_bundle.get_data(), rt, &model, err, sizeof(err));
	mb_runtime_options_free(rt);
	if (rs != MB_OK || model == nullptr) {
		model = nullptr;
		ERR_FAIL_V_MSG(false, String("motion_bricks: model load failed: ") + err);
	}
	return true;
}

bool MotionBricksPipeline::is_loaded() const {
	return model != nullptr;
}

void MotionBricksPipeline::unload() {
	if (model) {
		mb_model_free(model);
		model = nullptr;
	}
}

int MotionBricksPipeline::get_parameter_count() const {
	if (!model) {
		return 0;
	}
	uint64_t v = 0;
	mb_model_get_parameter_count(model, &v, nullptr, 0);
	return (int)v;
}

int MotionBricksPipeline::get_joint_count() const {
	if (!model) {
		return 0;
	}
	uint32_t v = 0;
	mb_model_get_joint_count(model, &v, nullptr, 0);
	return (int)v;
}

String MotionBricksPipeline::get_joint_name(int p_joint) const {
	if (!model || p_joint < 0) {
		return String();
	}
	char buf[128] = { 0 };
	uint64_t written = 0;
	mb_status rs = mb_model_get_joint_name(model, (uint32_t)p_joint, buf, sizeof(buf), &written, nullptr, 0);
	if (rs != MB_OK) {
		return String();
	}
	return String::utf8(buf);
}

int MotionBricksPipeline::get_joint_parent(int p_joint) const {
	if (!model || p_joint < 0) {
		return -1;
	}
	uint32_t v = 0;
	mb_status rs = mb_model_get_joint_parent(model, (uint32_t)p_joint, &v, nullptr, 0);
	if (rs != MB_OK) {
		return -1;
	}
	return (int)v;
}

void MotionBricksPipeline::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "opts"), &MotionBricksPipeline::load);
	ClassDB::bind_method(D_METHOD("is_loaded"), &MotionBricksPipeline::is_loaded);
	ClassDB::bind_method(D_METHOD("unload"), &MotionBricksPipeline::unload);
	ClassDB::bind_method(D_METHOD("get_parameter_count"), &MotionBricksPipeline::get_parameter_count);
	ClassDB::bind_method(D_METHOD("get_joint_count"), &MotionBricksPipeline::get_joint_count);
	ClassDB::bind_method(D_METHOD("get_joint_name", "joint"), &MotionBricksPipeline::get_joint_name);
	ClassDB::bind_method(D_METHOD("get_joint_parent", "joint"), &MotionBricksPipeline::get_joint_parent);
}
