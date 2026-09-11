/**************************************************************************/
/*  order_independent_transparency_renderer.h                             */
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
#include "core/templates/rid.h"

class OrderIndependentTransparencyRenderer : public RefCounted {
	GDCLASS(OrderIndependentTransparencyRenderer, RefCounted);

	RID transparent_shader_rid;
	RID transparent_pipeline_rid;
	RID composite_shader_rid;
	RID composite_pipeline_rid;
	RID composite_pipeline_msaa_rid;
	RID depth_sampler_rid;
	RID uniform_set_rid;

protected:
	static void _bind_methods();

public:
	Error build_transparent_pipeline();
	Error build_composite_pipeline(bool p_use_msaa);
	void update_uniform_sets(RID p_vertex_buf, RID p_instance_buf, RID p_camera_ubo, RID p_depth_tex);

	RID get_transparent_pipeline() const { return transparent_pipeline_rid; }
	RID get_composite_pipeline(bool p_msaa) const {
		return p_msaa ? composite_pipeline_msaa_rid : composite_pipeline_rid;
	}
	RID get_uniform_set() const { return uniform_set_rid; }

	void free_resources();

	OrderIndependentTransparencyRenderer();
	~OrderIndependentTransparencyRenderer();
};
