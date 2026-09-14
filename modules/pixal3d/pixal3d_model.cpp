/**************************************************************************/
/*  pixal3d_model.cpp                                                     */
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

#include "pixal3d_model.h"

#include "trellis2_capi.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

int Pixal3DModel::get_abi_version() const {
	return t2_abi_version();
}

// Look one gguf path out of `opts`, empty string when the key is absent.
// Callers ("" for "not loaded") pass the string on to t2_pipeline_load.
static const char *opt_str(const Dictionary &opts, const char *key, String &scratch) {
	if (!opts.has(key)) {
		scratch = "";
		return "";
	}
	scratch = opts[key];
	return scratch.utf8().get_data();
}

PackedByteArray Pixal3DModel::image_to_glb(const PackedByteArray &p_image_bytes, const Dictionary &p_opts) const {
	PackedByteArray empty;

	ERR_FAIL_COND_V_MSG(p_image_bytes.is_empty(), empty, "pixal3d: image_bytes is empty.");
	ERR_FAIL_COND_V_MSG(!p_opts.has("dino_gguf") || !p_opts.has("ss_flow_gguf") || !p_opts.has("ss_dec_gguf"),
			empty,
			"pixal3d: opts must name at least dino_gguf, ss_flow_gguf, ss_dec_gguf.");

	String s_dino, s_ss_flow, s_ss_dec, s_slat_flow, s_slat_hr, s_shape_dec, s_shape_enc, s_tex_dec, s_tex_flow, s_tex_flow_hr;
	const char *dino = opt_str(p_opts, "dino_gguf", s_dino);
	const char *ss_flow = opt_str(p_opts, "ss_flow_gguf", s_ss_flow);
	const char *ss_dec = opt_str(p_opts, "ss_dec_gguf", s_ss_dec);
	const char *slat_flow = opt_str(p_opts, "slat_flow_gguf", s_slat_flow);
	const char *slat_hr = opt_str(p_opts, "slat_hr_flow_gguf", s_slat_hr);
	const char *shape_dec = opt_str(p_opts, "shape_dec_gguf", s_shape_dec);
	const char *shape_enc = opt_str(p_opts, "shape_enc_gguf", s_shape_enc);
	const char *tex_dec = opt_str(p_opts, "tex_dec_gguf", s_tex_dec);
	const char *tex_flow = opt_str(p_opts, "tex_flow_gguf", s_tex_flow);
	const char *tex_flow_hr = opt_str(p_opts, "tex_flow_hr_gguf", s_tex_flow_hr);

	char err[512] = { 0 };
	t2_pipeline *pipe = t2_pipeline_load(dino, ss_flow, ss_dec, slat_flow, slat_hr, shape_dec,
			shape_enc, tex_dec, tex_flow, tex_flow_hr, 0, err, sizeof(err));
	ERR_FAIL_NULL_V_MSG(pipe, empty, String("pixal3d: pipeline load failed: ") + err);

	const int pipeline_type = p_opts.get("pipeline_type", T2_PIPE_AUTO);
	const int background_mode = p_opts.get("background_mode", T2_BACKGROUND_AUTO);
	const int seed = p_opts.get("seed", 0);
	const int steps = p_opts.get("steps", 25);
	const float guidance = p_opts.get("guidance", 3.0f);
	const int texture_steps = p_opts.get("texture_steps", 0);

	err[0] = 0;
	t2_mesh_result *mesh = t2_generate(pipe, p_image_bytes.ptr(), p_image_bytes.size(),
			pipeline_type, background_mode, (uint64_t)seed, steps, guidance, texture_steps,
			nullptr, nullptr, nullptr, nullptr, err, sizeof(err));
	if (!mesh) {
		t2_pipeline_free(pipe);
		ERR_FAIL_V_MSG(empty, String("pixal3d: generate failed: ") + err);
	}

	const int n_verts = t2_mesh_n_verts(mesh);
	const int n_tris = t2_mesh_n_tris(mesh);
	const float *verts = t2_mesh_verts(mesh);
	const int *tris = t2_mesh_tris(mesh);
	const float *pbr = t2_mesh_has_pbr(mesh) ? t2_mesh_pbr(mesh) : nullptr;
	const int texture_size = p_opts.get("texture_size", 1024);
	const int component_filter = p_opts.get("component_filter", -1);

	int glb_len = 0;
	err[0] = 0;
	uint8_t *glb = t2_bake_glb(verts, n_verts, tris, n_tris, pbr, texture_size, component_filter,
			&glb_len, err, sizeof(err));

	t2_mesh_free(mesh);
	t2_pipeline_free(pipe);

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

void Pixal3DModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_abi_version"), &Pixal3DModel::get_abi_version);
	ClassDB::bind_method(D_METHOD("image_to_glb", "image_bytes", "opts"), &Pixal3DModel::image_to_glb);
}
