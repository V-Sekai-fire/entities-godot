/**************************************************************************/
/*  pixal3d_pipeline.h                                                    */
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
#include "pixal3d_latent.h"

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"

struct t2_pipeline;

class Pixal3DPipeline : public RefCounted {
	GDCLASS(Pixal3DPipeline, RefCounted);

protected:
	static void _bind_methods();

public:
	Pixal3DPipeline();
	~Pixal3DPipeline();

	// Load the trellis2 pipeline with the given GGUFs. `opts` names the
	// GGUF checkpoints (same keys as Pixal3DModel::image_to_glb). Returns
	// true on success; on failure pushes the reason through print_error
	// and leaves the pipeline unloaded.
	bool load(const Dictionary &p_opts);

	// True if the pipeline is loaded and ready to generate.
	bool is_loaded() const;

	// The capability bitmask reported by the loaded pipeline (T2_CAP_*).
	int caps() const;

	// The backend the pipeline is running on (e.g. "metal", "cpu").
	String backend() const;

	// Two-stage split. encode_image runs the image-encoder half (currently
	// stashes the image bytes plus a snapshot of the generation options
	// pending an ABI split); decode_latent runs the mesh-decoder half and
	// returns GLB bytes. Every stage passes latents (RFD 1053 latents rule);
	// the VAE-shaped decode happens once, in decode_latent, at final output.
	Ref<Pixal3DLatent> encode_image(const PackedByteArray &p_image_bytes, const Dictionary &p_opts) const;
	PackedByteArray decode_latent(const Ref<Pixal3DLatent> &p_latent, const Dictionary &p_gen_opts) const;

	// Free the loaded pipeline. Safe to call more than once.
	void unload();

private:
	t2_pipeline *pipeline = nullptr;
};
