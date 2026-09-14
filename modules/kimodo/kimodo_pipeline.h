/**************************************************************************/
/*  kimodo_pipeline.h                                                     */
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
#include "kimodo_latent.h"

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"

struct kimodo_model;

class KimodoPipeline : public RefCounted {
	GDCLASS(KimodoPipeline, RefCounted);

protected:
	static void _bind_methods();

public:
	KimodoPipeline();
	~KimodoPipeline();

	// Load the kimodo model. `opts` has required key `motion_gguf` and
	// optional keys `text_gguf` (required for text prompts, unused for
	// precomputed-embedding decode), `text_adapter_gguf`, `threads`,
	// `device`. Returns true on success.
	bool load(const Dictionary &p_opts);
	bool is_loaded() const;
	String last_error() const;
	void unload();

	// Full text-to-motion: prompt -> internal text encoder -> denoiser.
	Dictionary generate_from_prompt(const String &p_prompt, const Dictionary &p_opts) const;

	// Denoiser only: takes a KimodoLatent whose embedding is exactly the
	// [1, 1, 4096] F32 LLM2Vec latent kimodo_generate_embedding expects.
	// This is the honest latent-stage decode.
	Dictionary generate_from_embedding(const Ref<KimodoLatent> &p_latent, const Dictionary &p_opts) const;

private:
	kimodo_model *model = nullptr;
	Dictionary _generate_dict(void *motion) const;
};
