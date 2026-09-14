/**************************************************************************/
/*  kimodo_pipeline.cpp                                                   */
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

#include "kimodo_pipeline.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <kimodo/kimodo_capi.h>

#include <cstring>

KimodoPipeline::KimodoPipeline() {}

KimodoPipeline::~KimodoPipeline() {
	unload();
}

bool KimodoPipeline::load(const Dictionary &p_opts) {
	if (model) {
		unload();
	}

	ERR_FAIL_COND_V_MSG(!p_opts.has("motion_gguf"), false, "kimodo: opts must have motion_gguf.");

	CharString s_motion = String(p_opts["motion_gguf"]).utf8();
	CharString s_text = p_opts.has("text_gguf") ? String(p_opts["text_gguf"]).utf8() : String("").utf8();
	CharString s_adapter = p_opts.has("text_adapter_gguf") ? String(p_opts["text_adapter_gguf"]).utf8() : String("").utf8();

	kimodo_runtime_options rt = {};
	rt.size = sizeof(rt);
	rt.threads = (uint32_t)(int)p_opts.get("threads", 0);
	rt.device = (kimodo_device)(int)p_opts.get("device", 0);
	rt.backend_dir = nullptr;

	char err[512] = { 0 };
	model = kimodo_model_load(s_motion.get_data(),
			p_opts.has("text_gguf") ? s_text.get_data() : nullptr,
			p_opts.has("text_adapter_gguf") ? s_adapter.get_data() : nullptr,
			&rt, err, sizeof(err));
	ERR_FAIL_NULL_V_MSG(model, false, String("kimodo: model load failed: ") + err);
	return true;
}

bool KimodoPipeline::is_loaded() const {
	return model != nullptr;
}

String KimodoPipeline::last_error() const {
	if (!model) {
		return String();
	}
	const char *e = kimodo_model_last_error(model);
	return e ? String::utf8(e) : String();
}

void KimodoPipeline::unload() {
	if (model) {
		kimodo_model_free(model);
		model = nullptr;
	}
}

Dictionary KimodoPipeline::_generate_dict(void *p_motion_ptr) const {
	kimodo_motion *motion = (kimodo_motion *)p_motion_ptr;
	const int frames = kimodo_motion_frames(motion);
	const int joints = kimodo_motion_joints(motion);
	const float *rots = kimodo_motion_local_rotations_xyzw(motion);
	const float *root = kimodo_motion_root_positions(motion);

	PackedFloat32Array rots_arr;
	rots_arr.resize(frames * joints * 4);
	memcpy(rots_arr.ptrw(), rots, sizeof(float) * frames * joints * 4);

	PackedFloat32Array root_arr;
	root_arr.resize(frames * 3);
	memcpy(root_arr.ptrw(), root, sizeof(float) * frames * 3);

	Dictionary out;
	out["frames"] = frames;
	out["joints"] = joints;
	out["local_rotations_xyzw"] = rots_arr;
	out["root_positions"] = root_arr;
	return out;
}

Dictionary KimodoPipeline::generate_from_prompt(const String &p_prompt, const Dictionary &p_opts) const {
	Dictionary empty;
	ERR_FAIL_NULL_V_MSG(model, empty, "kimodo: pipeline not loaded; call load() first.");
	ERR_FAIL_COND_V_MSG(p_prompt.is_empty(), empty, "kimodo: prompt is empty.");

	kimodo_generation_options gen = {};
	gen.size = sizeof(gen);
	gen.seed = (uint64_t)(int)p_opts.get("seed", 0);
	gen.frames = (uint32_t)(int)p_opts.get("frames", 120);
	gen.diffusion_steps = (uint32_t)(int)p_opts.get("diffusion_steps", 30);
	gen.text_cfg_weight = (float)(double)p_opts.get("text_cfg_weight", 5.0);
	gen.constraint_cfg_weight = (float)(double)p_opts.get("constraint_cfg_weight", 1.0);

	CharString s_prompt = p_prompt.utf8();
	char err[512] = { 0 };
	kimodo_motion *motion = kimodo_generate(model, s_prompt.get_data(), &gen, err, sizeof(err));
	ERR_FAIL_NULL_V_MSG(motion, empty, String("kimodo: generate failed: ") + err + " (" + last_error() + ")");

	Dictionary out = _generate_dict(motion);
	kimodo_motion_free(motion);
	return out;
}

Dictionary KimodoPipeline::generate_from_embedding(const Ref<KimodoLatent> &p_latent, const Dictionary &p_opts) const {
	Dictionary empty;
	ERR_FAIL_NULL_V_MSG(model, empty, "kimodo: pipeline not loaded; call load() first.");
	ERR_FAIL_COND_V_MSG(p_latent.is_null(), empty, "kimodo: latent is null.");
	ERR_FAIL_COND_V_MSG(p_latent->embedding.size() != 4096, empty, "kimodo: latent.embedding must be exactly 4096 F32 values.");

	kimodo_embedding emb = {};
	emb.data = p_latent->embedding.ptr();
	emb.values = (uint32_t)p_latent->embedding.size();

	kimodo_generation_options gen = {};
	gen.size = sizeof(gen);
	gen.seed = (uint64_t)(int)p_opts.get("seed", 0);
	gen.frames = (uint32_t)(int)p_opts.get("frames", 120);
	gen.diffusion_steps = (uint32_t)(int)p_opts.get("diffusion_steps", 30);
	gen.text_cfg_weight = (float)(double)p_opts.get("text_cfg_weight", 5.0);
	gen.constraint_cfg_weight = (float)(double)p_opts.get("constraint_cfg_weight", 1.0);

	char err[512] = { 0 };
	kimodo_motion *motion = kimodo_generate_embedding(model, &emb, &gen, err, sizeof(err));
	ERR_FAIL_NULL_V_MSG(motion, empty, String("kimodo: generate_embedding failed: ") + err + " (" + last_error() + ")");

	Dictionary out = _generate_dict(motion);
	kimodo_motion_free(motion);
	return out;
}

void KimodoPipeline::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "opts"), &KimodoPipeline::load);
	ClassDB::bind_method(D_METHOD("is_loaded"), &KimodoPipeline::is_loaded);
	ClassDB::bind_method(D_METHOD("last_error"), &KimodoPipeline::last_error);
	ClassDB::bind_method(D_METHOD("unload"), &KimodoPipeline::unload);
	ClassDB::bind_method(D_METHOD("generate_from_prompt", "prompt", "opts"), &KimodoPipeline::generate_from_prompt);
	ClassDB::bind_method(D_METHOD("generate_from_embedding", "latent", "opts"), &KimodoPipeline::generate_from_embedding);
}
