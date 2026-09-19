/**************************************************************************/
/*  skin_tokens_pipeline.cpp                                              */
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

#include "skin_tokens_pipeline.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <skintokens/skintokens.h>

SkinTokensPipeline::SkinTokensPipeline() {}

SkinTokensPipeline::~SkinTokensPipeline() {
	unload();
}

bool SkinTokensPipeline::load(const Dictionary &p_opts) {
	if (model) {
		unload();
	}

	ERR_FAIL_COND_V_MSG(!p_opts.has("bundle_path"), false, "skin_tokens: opts must have bundle_path.");

	CharString s_bundle = String(p_opts["bundle_path"]).utf8();

	st_runtime_options rt = st_default_runtime_options();
	rt.device = (st_device)(int)p_opts.get("device", (int)ST_DEVICE_AUTO);

	char err[512] = { 0 };
	st_status s = st_model_load(s_bundle.get_data(), &rt, &model, err, sizeof(err));
	if (s != ST_OK || model == nullptr) {
		model = nullptr;
		ERR_FAIL_V_MSG(false, String("skin_tokens: model load failed: ") + err);
	}
	return true;
}

bool SkinTokensPipeline::is_loaded() const {
	return model != nullptr;
}

String SkinTokensPipeline::backend() const {
	if (!model) {
		return String();
	}
	const char *b = st_model_backend_name(model);
	return b ? String::utf8(b) : String();
}

String SkinTokensPipeline::last_error() const {
	if (!model) {
		return String();
	}
	const char *e = st_model_last_error(model);
	return e ? String::utf8(e) : String();
}

void SkinTokensPipeline::unload() {
	if (model) {
		st_model_free(model);
		model = nullptr;
	}
}

int SkinTokensPipeline::rig_file(const String &p_mesh_path, const String &p_output_path, const Dictionary &p_opts) const {
	ERR_FAIL_NULL_V_MSG(model, ST_INVALID_ARGUMENT, "skin_tokens: pipeline not loaded; call load() first.");
	ERR_FAIL_COND_V_MSG(p_mesh_path.is_empty(), ST_INVALID_ARGUMENT, "skin_tokens: mesh_path is empty.");
	ERR_FAIL_COND_V_MSG(p_output_path.is_empty(), ST_INVALID_ARGUMENT, "skin_tokens: output_path is empty.");

	CharString s_mesh = p_mesh_path.utf8();
	CharString s_output = p_output_path.utf8();

	st_generation_options gen = st_default_generation_options();
	if (p_opts.has("seed")) {
		gen.seed = (uint64_t)(int)p_opts["seed"];
	}
	if (p_opts.has("top_k")) {
		gen.top_k = (uint32_t)(int)p_opts["top_k"];
	}
	if (p_opts.has("top_p")) {
		gen.top_p = (float)(double)p_opts["top_p"];
	}
	if (p_opts.has("temperature")) {
		gen.temperature = (float)(double)p_opts["temperature"];
	}
	if (p_opts.has("repetition_penalty")) {
		gen.repetition_penalty = (float)(double)p_opts["repetition_penalty"];
	}
	if (p_opts.has("beams")) {
		gen.beams = (uint32_t)(int)p_opts["beams"];
	}
	if (p_opts.has("max_tokens")) {
		gen.max_tokens = (size_t)(int)p_opts["max_tokens"];
	}
	if (p_opts.has("geometric_only")) {
		gen.geometric_only = (int)p_opts["geometric_only"];
	}

	int learned = 0;
	char err[512] = { 0 };
	st_status rs = st_rig_file(model, s_mesh.get_data(), s_output.get_data(), &gen, &learned, err, sizeof(err));
	if (rs != ST_OK) {
		String last = String::utf8(st_model_last_error(model));
		ERR_FAIL_V_MSG((int)rs, String("skin_tokens: rig_file failed (") + err + "): " + last);
	}
	return (int)rs;
}

void SkinTokensPipeline::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "opts"), &SkinTokensPipeline::load);
	ClassDB::bind_method(D_METHOD("is_loaded"), &SkinTokensPipeline::is_loaded);
	ClassDB::bind_method(D_METHOD("backend"), &SkinTokensPipeline::backend);
	ClassDB::bind_method(D_METHOD("last_error"), &SkinTokensPipeline::last_error);
	ClassDB::bind_method(D_METHOD("rig_file", "mesh_path", "output_path", "opts"), &SkinTokensPipeline::rig_file);
	ClassDB::bind_method(D_METHOD("unload"), &SkinTokensPipeline::unload);
}
