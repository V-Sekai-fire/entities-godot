/**************************************************************************/
/*  skin_tokens_pipeline.h                                                */
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
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"

struct st_model;

class SkinTokensPipeline : public RefCounted {
	GDCLASS(SkinTokensPipeline, RefCounted);

protected:
	static void _bind_methods();

public:
	SkinTokensPipeline();
	~SkinTokensPipeline();

	// Load the skin-tokens bundle. `opts` has required key
	// `bundle_path`; optional keys `threads`, `device` (0 auto, 1 CPU,
	// 2 Vulkan). Returns true on success; on failure pushes the reason
	// through print_error and leaves the pipeline unloaded.
	bool load(const Dictionary &p_opts);

	bool is_loaded() const;
	String backend() const;
	String last_error() const;

	// Rig a mesh at `mesh_path`, write the rigged GLB to `output_path`.
	// Reuses the loaded weights across calls, so a batch of parts pays
	// st_model_load once. Returns the st_status enum: 0 on success;
	// anything else is a failure whose reason went through print_error.
	int rig_file(const String &p_mesh_path, const String &p_output_path, const Dictionary &p_opts) const;

	void unload();

private:
	st_model *model = nullptr;
};
