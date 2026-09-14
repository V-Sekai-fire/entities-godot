/**************************************************************************/
/*  pixal3d_pipeline.cpp                                                  */
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

#include "pixal3d_pipeline.h"

#include "trellis2_capi.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <cstring>

static void opt_str(const Dictionary &opts, const char *key, CharString &out) {
	if (opts.has(key)) {
		out = String(opts[key]).utf8();
	} else {
		out = String("").utf8();
	}
}

Pixal3DPipeline::Pixal3DPipeline() {}

Pixal3DPipeline::~Pixal3DPipeline() {
	unload();
}

bool Pixal3DPipeline::load(const Dictionary &p_opts) {
	if (pipeline) {
		unload();
	}

	ERR_FAIL_COND_V_MSG(!p_opts.has("dino_gguf") || !p_opts.has("ss_flow_gguf") || !p_opts.has("ss_dec_gguf"),
			false,
			"pixal3d: opts must name at least dino_gguf, ss_flow_gguf, ss_dec_gguf.");

	CharString s_dino, s_ss_flow, s_ss_dec, s_slat_flow, s_slat_hr, s_shape_dec, s_shape_enc, s_tex_dec, s_tex_flow, s_tex_flow_hr;
	opt_str(p_opts, "dino_gguf", s_dino);
	opt_str(p_opts, "ss_flow_gguf", s_ss_flow);
	opt_str(p_opts, "ss_dec_gguf", s_ss_dec);
	opt_str(p_opts, "slat_flow_gguf", s_slat_flow);
	opt_str(p_opts, "slat_hr_flow_gguf", s_slat_hr);
	opt_str(p_opts, "shape_dec_gguf", s_shape_dec);
	opt_str(p_opts, "shape_enc_gguf", s_shape_enc);
	opt_str(p_opts, "tex_dec_gguf", s_tex_dec);
	opt_str(p_opts, "tex_flow_gguf", s_tex_flow);
	opt_str(p_opts, "tex_flow_hr_gguf", s_tex_flow_hr);

	char err[512] = { 0 };
	pipeline = t2_pipeline_load(s_dino.get_data(), s_ss_flow.get_data(), s_ss_dec.get_data(),
			s_slat_flow.get_data(), s_slat_hr.get_data(), s_shape_dec.get_data(),
			s_shape_enc.get_data(), s_tex_dec.get_data(), s_tex_flow.get_data(), s_tex_flow_hr.get_data(),
			0, err, sizeof(err));
	ERR_FAIL_NULL_V_MSG(pipeline, false, String("pixal3d: pipeline load failed: ") + err);
	return true;
}

bool Pixal3DPipeline::is_loaded() const {
	return pipeline != nullptr;
}

int Pixal3DPipeline::caps() const {
	if (!pipeline) {
		return 0;
	}
	return t2_pipeline_caps(pipeline);
}

String Pixal3DPipeline::backend() const {
	if (!pipeline) {
		return String();
	}
	const char *name = t2_pipeline_backend(pipeline);
	return name ? String::utf8(name) : String();
}

Ref<Pixal3DLatent> Pixal3DPipeline::encode_image(const PackedByteArray &p_image_bytes, const Dictionary &p_opts) const {
	Ref<Pixal3DLatent> empty;

	ERR_FAIL_NULL_V_MSG(pipeline, empty, "pixal3d: pipeline not loaded; call load() first.");
	ERR_FAIL_COND_V_MSG(p_image_bytes.is_empty(), empty, "pixal3d: image_bytes is empty.");

	Ref<Pixal3DLatent> latent;
	latent.instantiate();
	latent->image_bytes = p_image_bytes;
	latent->options_snapshot = p_opts.duplicate();
	return latent;
}

PackedByteArray Pixal3DPipeline::decode_latent(const Ref<Pixal3DLatent> &p_latent, const Dictionary &p_gen_opts) const {
	PackedByteArray empty;

	ERR_FAIL_NULL_V_MSG(pipeline, empty, "pixal3d: pipeline not loaded; call load() first.");
	ERR_FAIL_COND_V_MSG(p_latent.is_null(), empty, "pixal3d: latent is null.");
	ERR_FAIL_COND_V_MSG(p_latent->image_bytes.is_empty(), empty, "pixal3d: latent carries no image bytes.");

	Dictionary opts = p_latent->options_snapshot.duplicate();
	for (const KeyValue<Variant, Variant> &kv : p_gen_opts) {
		opts[kv.key] = kv.value;
	}

	const int pipeline_type = (int)opts.get("pipeline_type", (int)T2_PIPE_AUTO);
	const int background_mode = (int)opts.get("background_mode", (int)T2_BACKGROUND_AUTO);
	const int seed = (int)opts.get("seed", 0);
	const int steps = (int)opts.get("steps", 25);
	const float guidance = (float)(double)opts.get("guidance", 3.0);
	const int texture_steps = (int)opts.get("texture_steps", 0);

	char err[512] = { 0 };
	t2_mesh_result *mesh = t2_generate(pipeline, p_latent->image_bytes.ptr(), p_latent->image_bytes.size(),
			pipeline_type, background_mode, (uint64_t)seed, steps, guidance, texture_steps,
			nullptr, nullptr, nullptr, nullptr, err, sizeof(err));
	ERR_FAIL_NULL_V_MSG(mesh, empty, String("pixal3d: generate failed: ") + err);

	const int n_verts = t2_mesh_n_verts(mesh);
	const int n_tris = t2_mesh_n_tris(mesh);
	const float *verts = t2_mesh_verts(mesh);
	const int *tris = t2_mesh_tris(mesh);
	const float *pbr = t2_mesh_has_pbr(mesh) ? t2_mesh_pbr(mesh) : nullptr;
	const int texture_size = (int)opts.get("texture_size", 1024);
	const int component_filter = (int)opts.get("component_filter", -1);

	int glb_len = 0;
	err[0] = 0;
	uint8_t *glb = t2_bake_glb(verts, n_verts, tris, n_tris, pbr, texture_size, component_filter,
			&glb_len, err, sizeof(err));

	t2_mesh_free(mesh);

	if (!glb || glb_len <= 0) {
		if (glb) {
			t2_free_buffer(glb);
		}
		ERR_FAIL_V_MSG(empty, String("pixal3d: bake_glb failed: ") + err);
	}

	PackedByteArray out;
	out.resize(glb_len);
	memcpy(out.ptrw(), glb, glb_len);
	t2_free_buffer(glb);
	return out;
}

void Pixal3DPipeline::unload() {
	if (pipeline) {
		t2_pipeline_free(pipeline);
		pipeline = nullptr;
	}
}

void Pixal3DPipeline::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "opts"), &Pixal3DPipeline::load);
	ClassDB::bind_method(D_METHOD("is_loaded"), &Pixal3DPipeline::is_loaded);
	ClassDB::bind_method(D_METHOD("caps"), &Pixal3DPipeline::caps);
	ClassDB::bind_method(D_METHOD("backend"), &Pixal3DPipeline::backend);
	ClassDB::bind_method(D_METHOD("encode_image", "image_bytes", "opts"), &Pixal3DPipeline::encode_image);
	ClassDB::bind_method(D_METHOD("decode_latent", "latent", "gen_opts"), &Pixal3DPipeline::decode_latent);
	ClassDB::bind_method(D_METHOD("unload"), &Pixal3DPipeline::unload);
}
