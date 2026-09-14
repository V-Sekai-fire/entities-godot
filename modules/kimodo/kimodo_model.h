/**************************************************************************/
/*  kimodo_model.h                                                        */
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
#include "core/io/resource.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
class KimodoModel : public Resource {
	GDCLASS(KimodoModel, Resource);

protected:
	static void _bind_methods();

public:
	int get_abi_version() const;
	String get_status_string(int p_status) const;

	// Text-to-motion. Returns a Dictionary with `frames`, `joints`,
	// `local_rotations_xyzw` (PackedFloat32Array, frames * joints * 4),
	// and `root_positions` (PackedFloat32Array, frames * 3). Returns an
	// empty Dictionary on any error; the reason is pushed through
	// print_error.
	Dictionary generate_motion(const String &p_motion_gguf, const String &p_text_gguf, const String &p_text_adapter_gguf, const String &p_prompt, const Dictionary &p_opts) const;
};
