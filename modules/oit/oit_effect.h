/**************************************************************************/
/*  oit_effect.h                                                          */
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

#include "core/math/projection.h"
#include "core/math/vector2i.h"
#include "core/math/vector3i.h"
#include "servers/rendering/rendering_device.h"

#include "modules/oit/shaders/oit_integrate.glsl.gen.h"
#include "modules/oit/shaders/oit_voxelize.glsl.gen.h"

class OITEffect {
	OitVoxelizeShaderRD voxelize_shader;
	RID voxelize_shader_version;
	RID voxelize_pipeline;

	OitIntegrateShaderRD integrate_shader;
	RID integrate_shader_version;
	RID integrate_pipeline;

	Vector3i froxel_dims;
	uint32_t extinction_bytes = 0;
	RID extinction_buffer;
	RID transmittance_buffer;
	RID transmittance_sampler;
	RID splat_framebuffer;
	RID voxelize_uniform_set;
	RID integrate_uniform_set;

	void _free_buffers();
	RID _uniform_set(RID &r_cached, const Vector<RD::Uniform> &p_uniforms, RID p_shader);

public:
	void configure(const Vector2i &p_screen_size, int p_slice_count, const Vector2i &p_tile_size);
	void clear_extinction();
	void voxelize(RID p_splat_buffer, RID p_splat_count_buffer, RID p_params_buffer, uint32_t p_splat_count);
	void integrate(RID p_params_buffer);

	RID get_extinction_buffer() const { return extinction_buffer; }
	RID get_splat_framebuffer() const { return splat_framebuffer; }
	RID get_transmittance_texture() const { return transmittance_buffer; }
	RID get_transmittance_sampler() const { return transmittance_sampler; }
	Vector3i get_froxel_dims() const { return froxel_dims; }
	bool is_configured() const { return extinction_buffer.is_valid(); }

	OITEffect();
	~OITEffect();
};
