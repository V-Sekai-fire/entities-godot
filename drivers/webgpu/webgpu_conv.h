/**************************************************************************/
/*  webgpu_conv.h                                                         */
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

#include "drivers/webgpu/webgpu_platform.h"
#include "servers/rendering/rendering_device.h"

WGPUBufferUsage webgpu_buffer_usage_from_rd(BitField<RDD::BufferUsageBits> p_buffer_usage);
WGPUTextureFormat webgpu_texture_format_from_rd(RDD::DataFormat p_data_format);
WGPUFilterMode webgpu_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter);
WGPUMipmapFilterMode webgpu_mipmap_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter);
WGPUAddressMode webgpu_address_mode_from_rd(RDD::SamplerRepeatMode p_sampler_repeat_mode);
WGPUCompareFunction webgpu_compare_mode_from_rd(RDD::CompareOperator p_compare_operator);
WGPUVertexFormat webgpu_vertex_format_from_rd(RDD::DataFormat p_data_format);
WGPULoadOp webgpu_load_op_from_rd(RDD::AttachmentLoadOp p_load_op);
WGPUStoreOp webgpu_store_op_from_rd(RDD::AttachmentStoreOp p_store_op);
WGPUTextureViewDimension webgpu_texture_view_dimension_from_rd(RDD::TextureType p_texture_type);
WGPUShaderStage webgpu_shader_stage_from_rd(RDD::ShaderStage p_shader_stage);
WGPUTextureAspect webgpu_texture_aspect_from_rd(RDD::TextureAspect p_texture_aspect);
WGPUTextureAspect webgpu_texture_aspect_from_rd(BitField<RDD::TextureAspectBits> p_texture_aspect);
WGPUTextureAspect webgpu_texture_aspect_from_rd_format(RDD::DataFormat p_data_format);
WGPUBlendOperation webgpu_blend_operation_from_rd(RDD::BlendOperation p_blend_operation);
WGPUBlendFactor webgpu_blend_factor_from_rd(RDD::BlendFactor p_blend_factor);
WGPUStencilOperation webgpu_stencil_operation_from_rd(RDD::StencilOperation p_stencil_operation);

// HACK: `wgpu` does not support swizzle: https://github.com/gfx-rs/wgpu/issues/1028
// Currently, our fork has it patched in for Vulkan-only.
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
WGPUComponentSwizzle webgpu_component_swizzle_from_rd(RDD::TextureSwizzle p_texture_swizzle);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
WGPUNativeTextureComponentSwizzle webgpu_component_swizzle_from_rd(RDD::TextureSwizzle p_texture_swizzle);
#endif

WGPUTextureSampleType webgpu_texture_sample_type_from_shader_uniform(RDD::ShaderUniform::TextureSampleType p_texture_sample_type);

RDD::DataFormat rd_texture_format_from_webgpu(WGPUTextureFormat p_format);
uint64_t rd_limit_from_webgpu(RDD::Limit p_selected_limit, WGPULimits p_limits);

struct FormatBlockDimension {
	uint32_t block_dim_x;
	uint32_t block_dim_y;
};

bool webgpu_texture_format_is_depth_stencil(WGPUTextureFormat p_format);
bool webgpu_texture_format_has_depth_aspect(WGPUTextureFormat p_format);
bool webgpu_texture_format_has_stencil_aspect(WGPUTextureFormat p_format);
WGPUTextureFormat webgpu_texture_format_downgrade_depth_only(WGPUTextureFormat p_format);
uint32_t webgpu_texture_format_block_copy_size(WGPUTextureFormat format, WGPUTextureAspect aspect);
FormatBlockDimension webgpu_texture_format_block_dimensions(WGPUTextureFormat format);

static const WGPUTextureFormat WEBGPU_CORE_SUPPORTED_FORMATS[] = {
	WGPUTextureFormat_R8Unorm,
	WGPUTextureFormat_R8Snorm,
	WGPUTextureFormat_R8Uint,
	WGPUTextureFormat_R8Sint,

	WGPUTextureFormat_R16Uint,
	WGPUTextureFormat_R16Sint,
	WGPUTextureFormat_R16Float,
	WGPUTextureFormat_RG8Unorm,
	WGPUTextureFormat_RG8Snorm,
	WGPUTextureFormat_RG8Uint,
	WGPUTextureFormat_RG8Sint,

	WGPUTextureFormat_R32Uint,
	WGPUTextureFormat_R32Sint,
	WGPUTextureFormat_R32Float,

	WGPUTextureFormat_RG16Uint,
	WGPUTextureFormat_RG16Sint,
	WGPUTextureFormat_RG16Float,
	WGPUTextureFormat_RGBA8Unorm,
	WGPUTextureFormat_RGBA8UnormSrgb,
	WGPUTextureFormat_RGBA8Snorm,
	WGPUTextureFormat_RGBA8Uint,
	WGPUTextureFormat_RGBA8Sint,
	WGPUTextureFormat_BGRA8Unorm,
	WGPUTextureFormat_BGRA8UnormSrgb,
	WGPUTextureFormat_RGB9E5Ufloat,
	WGPUTextureFormat_RGBA8Uint,
	WGPUTextureFormat_RGBA8Sint,
	WGPUTextureFormat_BGRA8Unorm,
	WGPUTextureFormat_BGRA8UnormSrgb,
	WGPUTextureFormat_RGB9E5Ufloat,
	WGPUTextureFormat_RGB10A2Uint,
	WGPUTextureFormat_RGB10A2Unorm,
	WGPUTextureFormat_RG11B10Ufloat,

	WGPUTextureFormat_RG32Uint,
	WGPUTextureFormat_RG32Sint,
	WGPUTextureFormat_RG32Float,

	WGPUTextureFormat_RGBA16Uint,
	WGPUTextureFormat_RGBA16Sint,
	WGPUTextureFormat_RGBA16Float,

	WGPUTextureFormat_RGBA32Uint,
	WGPUTextureFormat_RGBA32Sint,
	WGPUTextureFormat_RGBA32Float,

	WGPUTextureFormat_Stencil8,
	WGPUTextureFormat_Depth16Unorm,
	WGPUTextureFormat_Depth24Plus,
	WGPUTextureFormat_Depth24PlusStencil8,
	WGPUTextureFormat_Depth32Float,
	WGPUTextureFormat_Depth32FloatStencil8,

};
