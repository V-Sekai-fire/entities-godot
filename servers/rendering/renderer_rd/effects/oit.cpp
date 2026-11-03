/**************************************************************************/
/*  oit.cpp                                                               */
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

#include "oit.h"

#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"

using namespace RendererRD;

OITEffect::OITEffect() {
	Vector<String> defines;
	defines.push_back("");

	voxelize_shader.initialize(defines);
	voxelize_shader_version = voxelize_shader.version_create();
	voxelize_pipeline = RD::get_singleton()->compute_pipeline_create(voxelize_shader.version_get_shader(voxelize_shader_version, 0));

	integrate_shader.initialize(defines);
	integrate_shader_version = integrate_shader.version_create();
	integrate_pipeline = RD::get_singleton()->compute_pipeline_create(integrate_shader.version_get_shader(integrate_shader_version, 0));

	Vector<String> resolve_defines;
	resolve_defines.push_back("");
	resolve_defines.push_back("\n#define USE_MULTIVIEW\n");
	resolve_defines.push_back("\n#define USE_MULTISAMPLE\n");
	resolve_defines.push_back("\n#define USE_MULTIVIEW\n#define USE_MULTISAMPLE\n");
	resolve_shader.initialize(resolve_defines);
	resolve_shader_version = resolve_shader.version_create();

	// Premultiplied over: the resolve carries 1 - T in alpha so the background keeps T.
	RD::PipelineColorBlendState::Attachment blend_attachment;
	blend_attachment.enable_blend = true;
	blend_attachment.color_blend_op = RD::BLEND_OP_ADD;
	blend_attachment.src_color_blend_factor = RD::BLEND_FACTOR_ONE;
	blend_attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment.alpha_blend_op = RD::BLEND_OP_ADD;
	blend_attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
	blend_attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// The Forward+ color pass format keeps unused specular and motion vector slots after the color.
	RD::PipelineColorBlendState blend_state;
	blend_state.attachments = { blend_attachment, RD::PipelineColorBlendState::Attachment(), RD::PipelineColorBlendState::Attachment() };
	for (int i = 0; i < RESOLVE_VARIANT_MAX; i++) {
		resolve_pipelines[i].setup(resolve_shader.version_get_shader(resolve_shader_version, i), RD::RENDER_PRIMITIVE_TRIANGLES, RD::PipelineRasterizationState(), RD::PipelineMultisampleState(), RD::PipelineDepthStencilState(), blend_state, 0);
	}
}

OITEffect::~OITEffect() {
	_free_buffers();
	_free_accumulation();
	for (int i = 0; i < RESOLVE_VARIANT_MAX; i++) {
		resolve_pipelines[i].clear();
	}
	if (resolve_shader_version.is_valid()) {
		resolve_shader.version_free(resolve_shader_version);
	}
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
	if (rd->framebuffer_is_valid(splat_framebuffer)) {
		rd->free_rid(splat_framebuffer);
	}
	splat_framebuffer = RID();
	if (splat_placeholder.is_valid()) {
		rd->free_rid(splat_placeholder);
		splat_placeholder = RID();
	}
	froxel_dims = Vector3i();
	view_count = 0;
	extinction_bytes = 0;
	voxelize_uniform_set = RID();
	integrate_uniform_set = RID();
}

void OITEffect::_free_accumulation() {
	RenderingDevice *rd = RD::get_singleton();
	if (rd->framebuffer_is_valid(accumulation_framebuffer)) {
		rd->free_rid(accumulation_framebuffer);
	}
	accumulation_framebuffer = RID();
	if (accumulated_color.is_valid()) {
		rd->free_rid(accumulated_color);
		accumulated_color = RID();
	}
	if (accumulated_extinction.is_valid()) {
		rd->free_rid(accumulated_extinction);
		accumulated_extinction = RID();
	}
	accumulation_size = Vector2i();
	accumulation_view_count = 0;
	accumulation_depth = RID();
}

RID OITEffect::_uniform_set(RID &r_cached, const Vector<RD::Uniform> &p_uniforms, RID p_shader) {
	RenderingDevice *rd = RD::get_singleton();
	if (r_cached.is_valid() && rd->uniform_set_is_valid(r_cached)) {
		return r_cached;
	}
	r_cached = rd->uniform_set_create(p_uniforms, p_shader, 0);
	return r_cached;
}

void OITEffect::configure(const Vector2i &p_screen_size, int p_slice_count, const Vector2i &p_tile_size, uint32_t p_view_count) {
	ERR_FAIL_COND(p_screen_size.x <= 0 || p_screen_size.y <= 0);
	ERR_FAIL_COND(p_slice_count <= 0);
	ERR_FAIL_COND(p_tile_size.x <= 0 || p_tile_size.y <= 0);
	ERR_FAIL_COND(p_view_count == 0);

	Vector3i dims;
	dims.x = MAX(1, (p_screen_size.x + p_tile_size.x - 1) / p_tile_size.x);
	dims.y = MAX(1, (p_screen_size.y + p_tile_size.y - 1) / p_tile_size.y);
	dims.z = p_slice_count;

	if (dims == froxel_dims && p_view_count == view_count && extinction_buffer.is_valid()) {
		return;
	}

	_free_buffers();
	froxel_dims = dims;
	view_count = p_view_count;

	RenderingDevice *rd = RD::get_singleton();

	uint32_t packed_x = uint32_t(dims.x) * p_view_count;
	extinction_bytes = packed_x * uint32_t(dims.y) * uint32_t(dims.z) * sizeof(uint32_t);
	extinction_buffer = rd->storage_buffer_create(extinction_bytes);

	RD::TextureFormat transmittance_fmt;
	transmittance_fmt.width = packed_x;
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

	// The raster splat draws at froxel resolution so each surface lands once per covered column.
	// A layered depth attachment carries the view count, which an empty framebuffer cannot; the splat pipeline never tests it.
	RD::TextureFormat placeholder_fmt;
	placeholder_fmt.width = dims.x;
	placeholder_fmt.height = dims.y;
	placeholder_fmt.array_layers = p_view_count;
	placeholder_fmt.texture_type = RD::TEXTURE_TYPE_2D_ARRAY;
	placeholder_fmt.usage_bits = RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	placeholder_fmt.format = rd->texture_is_format_supported_for_usage(RD::DATA_FORMAT_D16_UNORM, placeholder_fmt.usage_bits) ? RD::DATA_FORMAT_D16_UNORM : RD::DATA_FORMAT_D32_SFLOAT;
	splat_placeholder = rd->texture_create(placeholder_fmt, RD::TextureView());
	splat_framebuffer = rd->framebuffer_create(Vector<RID>({ splat_placeholder }), RD::INVALID_ID, p_view_count);
}

void OITEffect::clear_extinction() {
	ERR_FAIL_COND(!extinction_buffer.is_valid());
	RD::get_singleton()->buffer_clear(extinction_buffer, 0, extinction_bytes);
}

void OITEffect::voxelize(RID p_splat_buffer, RID p_splat_count_buffer, RID p_params_buffer, uint32_t p_splat_count) {
	ERR_FAIL_COND(!extinction_buffer.is_valid());
	RenderingDevice *rd = RD::get_singleton();

	clear_extinction();
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
	rd->compute_list_dispatch(compute_list, froxel_dims.x * view_count, froxel_dims.y, 1);
	rd->compute_list_end();
}

void OITEffect::configure_accumulation(const Vector2i &p_size, RID p_depth_texture, uint32_t p_view_count, RD::TextureSamples p_samples) {
	ERR_FAIL_COND(p_size.x <= 0 || p_size.y <= 0);
	ERR_FAIL_COND(!p_depth_texture.is_valid());
	ERR_FAIL_COND(p_view_count == 0);

	RenderingDevice *rd = RD::get_singleton();
	if (p_size == accumulation_size && p_depth_texture == accumulation_depth && p_view_count == accumulation_view_count && p_samples == accumulation_samples && rd->framebuffer_is_valid(accumulation_framebuffer)) {
		return;
	}

	_free_accumulation();
	accumulation_size = p_size;
	accumulation_depth = p_depth_texture;
	accumulation_view_count = p_view_count;
	accumulation_samples = p_samples;

	RD::TextureFormat fmt;
	fmt.width = p_size.x;
	fmt.height = p_size.y;
	fmt.array_layers = p_view_count;
	fmt.texture_type = p_view_count > 1 ? RD::TEXTURE_TYPE_2D_ARRAY : RD::TEXTURE_TYPE_2D;
	fmt.samples = p_samples;
	fmt.usage_bits = RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
	fmt.format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
	accumulated_color = rd->texture_create(fmt, RD::TextureView());
	fmt.format = RD::DATA_FORMAT_R16_SFLOAT;
	accumulated_extinction = rd->texture_create(fmt, RD::TextureView());

	Vector<RID> attachments;
	attachments.push_back(accumulated_color);
	attachments.push_back(accumulated_extinction);
	attachments.push_back(p_depth_texture);
	accumulation_framebuffer = rd->framebuffer_create(attachments, RD::INVALID_ID, p_view_count);
}

void OITEffect::resolve(RD::DrawListID p_draw_list, RD::FramebufferFormatID p_framebuffer_format) {
	ERR_FAIL_COND(!accumulation_framebuffer.is_valid());
	RenderingDevice *rd = RD::get_singleton();
	UniformSetCacheRD *uniform_set_cache = UniformSetCacheRD::get_singleton();
	ERR_FAIL_NULL(uniform_set_cache);

	RID sampler = RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
	RD::Uniform u_color(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 0, Vector<RID>({ sampler, accumulated_color }));
	RD::Uniform u_extinction(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 1, Vector<RID>({ sampler, accumulated_extinction }));

	bool multisample = accumulation_samples != RD::TEXTURE_SAMPLES_1;
	int variant = multisample ? (accumulation_view_count > 1 ? RESOLVE_VARIANT_MULTIVIEW_MULTISAMPLE : RESOLVE_VARIANT_MULTISAMPLE) : (accumulation_view_count > 1 ? RESOLVE_VARIANT_MULTIVIEW : RESOLVE_VARIANT_MONO);
	RID shader = resolve_shader.version_get_shader(resolve_shader_version, variant);
	rd->draw_list_bind_render_pipeline(p_draw_list, resolve_pipelines[variant].get_render_pipeline(RD::INVALID_ID, p_framebuffer_format));
	rd->draw_list_bind_uniform_set(p_draw_list, uniform_set_cache->get_cache(shader, 0, u_color, u_extinction), 0);
	if (multisample) {
		uint32_t push_constant[4] = { 1u << uint32_t(accumulation_samples), 0, 0, 0 };
		rd->draw_list_set_push_constant(p_draw_list, push_constant, sizeof(push_constant));
	}
	rd->draw_list_draw(p_draw_list, false, 1u, 3u);
}
