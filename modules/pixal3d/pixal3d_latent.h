/**************************************************************************/
/*  pixal3d_latent.h                                                      */
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

// Opaque intermediate produced by Pixal3DPipeline.encode_image and consumed
// by Pixal3DPipeline.decode_latent. Carries whatever the pipeline hands over
// between the image-encoder half and the mesh-decoder half.
//
// The current pipeline runs image -> mesh in one call, so this class is a
// forward-compatible handle that stashes what a split call would need: the
// input image bytes plus a snapshot of the generation knobs seen at encode
// time. When the trellis2 C ABI grows explicit intermediate entry points,
// this class holds the SS_FLOW or SLAT_FLOW output instead, without any
// caller change.
class Pixal3DLatent : public RefCounted {
	GDCLASS(Pixal3DLatent, RefCounted);

protected:
	static void _bind_methods();

public:
	PackedByteArray image_bytes;
	Dictionary options_snapshot;

	PackedByteArray get_image_bytes() const;
	Dictionary get_options_snapshot() const;
};
