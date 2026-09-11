/**************************************************************************/
/*  order_independent_transparency_buffer_pool.cpp                        */
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

#include "order_independent_transparency_buffer_pool.h"

#include "core/error/error_macros.h"

Error OrderIndependentTransparencyBufferPool::allocate(const Size2i &p_viewport, int p_rt_format) {
	if (p_viewport.x <= 0 || p_viewport.y <= 0) {
		return ERR_INVALID_PARAMETER;
	}
	current_size = p_viewport;
	current_format = p_rt_format;
	last_alloc_bytes = 0;
	return OK;
}

Error OrderIndependentTransparencyBufferPool::resize_if_needed(const Size2i &p_viewport) {
	if (p_viewport == current_size && framebuffer_rid.is_valid()) {
		return OK;
	}
	free_resources();
	return allocate(p_viewport, current_format);
}

void OrderIndependentTransparencyBufferPool::free_resources() {
	accum_texture_rid = RID();
	revealage_texture_rid = RID();
	framebuffer_rid = RID();
	current_size = Size2i();
	last_alloc_bytes = 0;
}

OrderIndependentTransparencyBufferPool::OrderIndependentTransparencyBufferPool() {}

OrderIndependentTransparencyBufferPool::~OrderIndependentTransparencyBufferPool() {
	free_resources();
}

void OrderIndependentTransparencyBufferPool::_bind_methods() {
}
