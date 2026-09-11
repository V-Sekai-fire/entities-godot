/**************************************************************************/
/*  order_independent_transparency_buffer_pool.h                          */
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

#include "core/math/vector2i.h"
#include "core/object/ref_counted.h"
#include "core/templates/rid.h"

class OrderIndependentTransparencyBufferPool : public RefCounted {
	GDCLASS(OrderIndependentTransparencyBufferPool, RefCounted);

	Size2i current_size;
	int current_format = 1;

	RID accum_texture_rid;
	RID revealage_texture_rid;
	RID framebuffer_rid;

	uint64_t last_alloc_bytes = 0;

protected:
	static void _bind_methods();

public:
	Error allocate(const Size2i &p_viewport, int p_rt_format);
	Error resize_if_needed(const Size2i &p_viewport);
	void free_resources();

	RID get_accum_texture() const { return accum_texture_rid; }
	RID get_revealage_texture() const { return revealage_texture_rid; }
	RID get_framebuffer() const { return framebuffer_rid; }
	Size2i get_size() const { return current_size; }
	uint64_t get_last_alloc_bytes() const { return last_alloc_bytes; }

	OrderIndependentTransparencyBufferPool();
	~OrderIndependentTransparencyBufferPool();
};
