/**************************************************************************/
/*  oit_effect.cpp                                                        */
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

#include "oit_effect.h"

OITEffect::OITEffect() {
	Vector<String> defines;
	defines.push_back("");

	voxelize_shader.initialize(defines);
	voxelize_shader_version = voxelize_shader.version_create();
	voxelize_pipeline = RD::get_singleton()->compute_pipeline_create(voxelize_shader.version_get_shader(voxelize_shader_version, 0));

	integrate_shader.initialize(defines);
	integrate_shader_version = integrate_shader.version_create();
	integrate_pipeline = RD::get_singleton()->compute_pipeline_create(integrate_shader.version_get_shader(integrate_shader_version, 0));
}

OITEffect::~OITEffect() {
	_free_buffers();
	if (voxelize_shader_version.is_valid()) {
		voxelize_shader.version_free(voxelize_shader_version);
	}
	if (integrate_shader_version.is_valid()) {
		integrate_shader.version_free(integrate_shader_version);
	}
}

void OITEffect::_free_buffers() {
	RenderingDevice *rd = RD::get_singleton();
	if (extinction_buffer.is_valid()) {
		rd->free_rid(extinction_buffer);
		extinction_buffer = RID();
	}
	if (transmittance_buffer.is_valid()) {
		rd->free_rid(transmittance_buffer);
		transmittance_buffer = RID();
	}
	if (transmittance_sampler.is_valid()) {
		rd->free_rid(transmittance_sampler);
		transmittance_sampler = RID();
	}
	froxel_dims = Vector3i();
	extinction_bytes = 0;
	voxelize_uniform_set = RID();
	integrate_uniform_set = RID();
}

RID OITEffect::_uniform_set(RID &r_cached, const Vector<RD::Uniform> &p_uniforms, RID p_shader) {
	RenderingDevice *rd = RD::get_singleton();
	if (r_cached.is_valid() && rd->uniform_set_is_valid(r_cached)) {
		return r_cached;
	}
	r_cached = rd->uniform_set_create(p_uniforms, p_shader, 0);
	return r_cached;
}

void OITEffect::configure(const Vector2i &p_screen_size, int p_slice_count, const Vector2i &p_tile_size) {
	ERR_FAIL_COND(p_screen_size.x <= 0 || p_screen_size.y <= 0);
	ERR_FAIL_COND(p_slice_count <= 0);
	ERR_FAIL_COND(p_tile_size.x <= 0 || p_tile_size.y <= 0);

	Vector3i dims;
	dims.x = MAX(1, (p_screen_size.x + p_tile_size.x - 1) / p_tile_size.x);
	dims.y = MAX(1, (p_screen_size.y + p_tile_size.y - 1) / p_tile_size.y);
	dims.z = p_slice_count;

	if (dims == froxel_dims && extinction_buffer.is_valid()) {
		return;
	}

	_free_buffers();
	froxel_dims = dims;

	RenderingDevice *rd = RD::get_singleton();

	extinction_bytes = uint32_t(dims.x) * uint32_t(dims.y) * uint32_t(dims.z) * sizeof(uint32_t);
	extinction_buffer = rd->storage_buffer_create(extinction_bytes);

	RD::TextureFormat transmittance_fmt;
	transmittance_fmt.width = dims.x;
	transmittance_fmt.height = dims.y;
	transmittance_fmt.depth = dims.z;
	transmittance_fmt.array_layers = 1;
	transmittance_fmt.mipmaps = 1;
	transmittance_fmt.texture_type = RD::TEXTURE_TYPE_3D;
	transmittance_fmt.format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
	transmittance_fmt.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
	transmittance_buffer = rd->texture_create(transmittance_fmt, RD::TextureView());

	RD::SamplerState sampler_state;
	sampler_state.mag_filter = RD::SAMPLER_FILTER_LINEAR;
	sampler_state.min_filter = RD::SAMPLER_FILTER_LINEAR;
	sampler_state.repeat_u = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	sampler_state.repeat_v = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	sampler_state.repeat_w = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	transmittance_sampler = rd->sampler_create(sampler_state);
}

void OITEffect::voxelize(RID p_splat_buffer, RID p_splat_count_buffer, RID p_params_buffer, uint32_t p_splat_count) {
	ERR_FAIL_COND(!extinction_buffer.is_valid());
	RenderingDevice *rd = RD::get_singleton();

	rd->buffer_clear(extinction_buffer, 0, extinction_bytes);
	if (p_splat_count == 0) {
		return;
	}

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 0;
		u.append_id(extinction_buffer);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.binding = 1;
		u.append_id(p_params_buffer);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 2;
		u.append_id(p_splat_buffer);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.binding = 3;
		u.append_id(p_splat_count_buffer);
		uniforms.push_back(u);
	}

	RID uniform_set = _uniform_set(voxelize_uniform_set, uniforms, voxelize_shader.version_get_shader(voxelize_shader_version, 0));

	RD::ComputeListID compute_list = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(compute_list, voxelize_pipeline);
	rd->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
	rd->compute_list_dispatch(compute_list, (p_splat_count + 63) / 64, 1, 1);
	rd->compute_list_end();
}

void OITEffect::integrate(RID p_params_buffer) {
	ERR_FAIL_COND(!extinction_buffer.is_valid());
	RenderingDevice *rd = RD::get_singleton();

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 0;
		u.append_id(extinction_buffer);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
		u.binding = 1;
		u.append_id(transmittance_buffer);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.binding = 2;
		u.append_id(p_params_buffer);
		uniforms.push_back(u);
	}

	RID uniform_set = _uniform_set(integrate_uniform_set, uniforms, integrate_shader.version_get_shader(integrate_shader_version, 0));

	RD::ComputeListID compute_list = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(compute_list, integrate_pipeline);
	rd->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
	rd->compute_list_dispatch(compute_list, froxel_dims.x, froxel_dims.y, 1);
	rd->compute_list_end();
}
