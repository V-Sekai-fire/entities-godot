/**************************************************************************/
/*  rendering_device_driver_webgpu.cpp                                    */
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

#include "rendering_device_driver_webgpu.h"

#include "core/error/error_macros.h"
#include "core/os/memory.h"
#include "core/string/print_string.h"
#include "core/templates/local_vector.h"
#include "drivers/webgpu/rendering_context_driver_webgpu.h"
#include "drivers/webgpu/rendering_shader_container_webgpu.h"
#include "drivers/webgpu/webgpu_conv.h"
#include "drivers/webgpu/webgpu_platform.h"

#include <cstdint>
#include <cstring>

#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/proxying.h>
#include <emscripten/threading.h>

// WebGPU objects live in the browser's main thread; workers have no device. Godot compiles
// shaders and pipelines on worker threads, so those calls run on the main thread instead.
#define WEBGPU_ON_MAIN_THREAD(m_type, m_call) \
	if (!emscripten_is_main_runtime_thread()) { \
		m_type result = {}; \
		auto call = [&]() { result = m_call; }; \
		emscripten_proxy_sync(emscripten_proxy_get_system_queue(), emscripten_main_runtime_thread_id(), [](void *p_call) { (*(decltype(call) *)p_call)(); }, &call); \
		return result; \
	}
#else
#define WEBGPU_ON_MAIN_THREAD(m_type, m_call)
#endif

#define WGPU_LOG_LEVEL WGPULogLevel_Trace

static constexpr uint32_t VERTEX_DYN_BITS = 2;
static constexpr uint64_t VERTEX_DYN_MASK = (uint64_t(1) << VERTEX_DYN_BITS) - 1ull;
static constexpr uint32_t UNIFORM_DYN_BITS = 4;
static constexpr uint32_t UNIFORM_DYN_MASK = (1u << UNIFORM_DYN_BITS) - 1u;

static void handle_request_device(WGPURequestDeviceStatus p_status,
		WGPUDevice p_device, WGPUStringView p_message,
		void *userdata, void *_) {
	if (p_status != WGPURequestDeviceStatus_Success) {
		print_error(vformat("[WEBGPU] requestDevice failed: status=%d msg=%s",
				(int)p_status, String::utf8(p_message.data, p_message.length)));
	} else if (p_device == nullptr) {
		print_error("[WEBGPU] requestDevice reported success but device is null.");
	} else {
		print_verbose("[WEBGPU] requestDevice ok");
	}
	*(WGPUDevice *)userdata = p_device;
}

static void handle_uncaptured_error(const WGPUDevice *_device, WGPUErrorType p_type,
		WGPUStringView p_message, void *_userdata1, void *_userdata2) {
	String message = String::utf8(p_message.data, p_message.length);
	ERR_PRINT(vformat("[WEBGPU] error: %s", message));
}

static void handle_device_lost(const WGPUDevice *_device, WGPUDeviceLostReason p_reason,
		WGPUStringView p_message, void *_userdata1, void *_userdata2) {
	if (p_reason == WGPUDeviceLostReason_Destroyed) {
		return;
	}
	String message = String::utf8(p_message.data, p_message.length);
	ERR_PRINT(vformat("[WEBGPU] Device lost: %s", message));
}

Error RenderingDeviceDriverWebGpu::initialize(uint32_t p_device_index, uint32_t p_frame_count) {
#ifdef WGPU_LOG_LEVEL
#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
	wgpuSetLogCallback([](WGPULogLevel p_level, WGPUStringView p_message, void *userdata) {
		if (p_level <= WGPU_LOG_LEVEL) {
			String message = String::utf8(p_message.data, p_message.length);
			print_line("[WEBGPU]", message);
		}
	},
			nullptr);
#endif
#endif

	adapter = context_driver->adapter_acquire(p_device_index);
	ERR_FAIL_NULL_V_MSG(adapter, FAILED, "No WebGPU adapter for this device.");
	context_device = context_driver->device_get(p_device_index);
	frame_count = MAX(p_frame_count, 1u);

	// Only request features the adapter reports it can grant. On browsers whose
	// WebGPU is stable (Firefox in this window) the Dawn-draft features
	// (texture-formats-tier1/2, subgroups, texture-component-swizzle) are not
	// implemented and a hard requirement would fail requestDevice.
	WGPUFeatureName wanted_features[] = {
		WGPUFeatureName_Depth32FloatStencil8,
		WGPUFeatureName_Float32Filterable,
		WGPUFeatureName_TextureCompressionBC,

#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
		WGPUFeatureName_TextureFormatsTier1,
		WGPUFeatureName_TextureFormatsTier2,
		WGPUFeatureName_Subgroups,
		WGPUFeatureName_TextureComponentSwizzle,
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
		// `wgpu` needs to add support for "texture-formats-tier1", "texture-formats-tier2", and "subgroups", see:
		// - https://github.com/gfx-rs/wgpu/issues/5555
		// - https://github.com/gfx-rs/wgpu/issues/8122
		(WGPUFeatureName)WGPUNativeFeature_TextureFormat16bitNorm,
		(WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures,
		(WGPUFeatureName)WGPUNativeFeature_Subgroup,
#endif
	};
	WGPUFeatureName required_features[sizeof(wanted_features) / sizeof(WGPUFeatureName)];
	uint32_t required_features_count = 0;
	for (uint32_t i = 0; i < sizeof(wanted_features) / sizeof(WGPUFeatureName); i++) {
		if (wgpuAdapterHasFeature(adapter, wanted_features[i])) {
			required_features[required_features_count++] = wanted_features[i];
		}
	}
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	texture_swizzle_supported = wgpuAdapterHasFeature(adapter, WGPUFeatureName_TextureComponentSwizzle);
	texture_formats_tier1_supported = wgpuAdapterHasFeature(adapter, WGPUFeatureName_TextureFormatsTier1);
	subgroups_supported = wgpuAdapterHasFeature(adapter, WGPUFeatureName_Subgroups);
#else
	texture_swizzle_supported = true;
	texture_formats_tier1_supported = true;
	subgroups_supported = true;
#endif
	// Features listed for reference only; enabling them needs matching runtime paths.
	//   WGPUNativeFeature_Immediates — currently emulated via push constants.
	//   Binding-array features (TextureBindingArray, StorageResourceBindingArray,
	//   BufferBindingArray, and the two NonUniformIndexing variants) — currently
	//   worked around via binding-array splitting.
	//   VertexWritableStorage, MultiDrawIndirect, MultiDrawIndirectCount — unused.

	// Clamp requested limits to what the adapter reports. Firefox's stable
	// WebGPU exposes lower ceilings than Chromium's Dawn; requesting more
	// than the adapter grants fails requestDevice.
	WGPULimits adapter_limits = WGPU_LIMITS_INIT;
	wgpuAdapterGetLimits(adapter, &adapter_limits);
	auto pick_limit = [](uint32_t wanted, uint32_t supported) -> uint32_t {
		return wanted <= supported ? wanted : supported;
	};
	WGPULimits required_limits = WGPU_LIMITS_INIT;
	required_limits.maxBindGroups = pick_limit(WEBGPU_MAX_BIND_GROUPS, adapter_limits.maxBindGroups);
	required_limits.maxImmediateSize = pick_limit(64, adapter_limits.maxImmediateSize);
	required_limits.maxSampledTexturesPerShaderStage = pick_limit(48, adapter_limits.maxSampledTexturesPerShaderStage);
	required_limits.maxStorageBuffersPerShaderStage = pick_limit(16, adapter_limits.maxStorageBuffersPerShaderStage);
	required_limits.maxStorageTexturesPerShaderStage = pick_limit(16, adapter_limits.maxStorageTexturesPerShaderStage);

	WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
	device_desc.requiredFeatureCount = required_features_count;
	device_desc.requiredFeatures = required_features;
	device_desc.requiredLimits = &required_limits;
	device_desc.deviceLostCallbackInfo = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;
	device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
	device_desc.deviceLostCallbackInfo.callback = handle_device_lost;
	device_desc.uncapturedErrorCallbackInfo = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;
	device_desc.uncapturedErrorCallbackInfo.callback = handle_uncaptured_error;
	WGPURequestDeviceCallbackInfo device_callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
	// WaitAnyOnly is the mode WaitAny actually drives. AllowProcessEvents
	// requires the caller to periodically pump wgpuInstanceProcessEvents,
	// which the emdawnwebgpu path never does; the request-device callback
	// then never fires under the WaitAny below and the device stays null.
	device_callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
	device_callback_info.callback = handle_request_device;
	device_callback_info.userdata1 = &this->device;
	WGPUFuture device_future = wgpuAdapterRequestDevice(adapter, &device_desc, device_callback_info);
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
	wait_info.future = device_future;
	wait_info.completed = false;
	WGPUWaitStatus wait_status = wgpuInstanceWaitAny(context_driver->instance_get(), 1, &wait_info, UINT64_MAX);
	ERR_FAIL_COND_V_MSG(wait_status != WGPUWaitStatus_Success, FAILED,
			vformat("Failed to wait on WebGPU device request: wait_status=%d completed=%d", (int)wait_status, (int)wait_info.completed));
	ERR_FAIL_COND_V_MSG(!wait_info.completed, FAILED,
			"WebGPU device-request future returned before the callback fired.");
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
	(void)device_future;
	wgpuInstanceProcessEvents(context_driver->instance_get());
#endif

	ERR_FAIL_NULL_V_MSG(this->device, FAILED, "WebGPU device pointer is null after successful WaitAny.");

#ifdef WGPU_LOG_LEVEL
#ifdef WEBGPU_BACKEND_DAWN_DESKTOP
	wgpuDeviceSetLoggingCallback(device, (WGPULoggingCallbackInfo){
												 .callback = [](WGPULoggingType p_type, WGPUStringView p_message, void *, void *) {
													 String message = String::utf8(p_message.data, p_message.length);
													 print_line("[DAWN]", message);
												 },
										 });
#endif
#endif

	queue = wgpuDeviceGetQueue(device);
	ERR_FAIL_COND_V(!this->queue, FAILED);

	capabilities = (Capabilities){
		// TODO: This information is not accurate, see modules/glslang/register_types.cpp:78.
		.device_family = DEVICE_WEBGPU,
		.version_major = 28,
		.version_minor = 0,
	};
	multiview_capabilities = (MultiviewCapabilities){
		.is_supported = false,
	};
	// Defaults: no fragment density map, no fragment shading rate.
	fdm_capabilities = FragmentDensityMapCapabilities();
	fsr_capabilities = FragmentShadingRateCapabilities();

	// Initialize push constant emulation buffer.
	WGPUBufferDescriptor push_constant_buffer_desc = WGPU_BUFFER_DESCRIPTOR_INIT;
	push_constant_buffer_desc.nextInChain = nullptr;
	push_constant_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
	push_constant_buffer_desc.size = WEBGPU_MAX_IMMEDIATE_SIZE * WEBGPU_PUSH_CONSTANT_EMULATION_BUFFER_ENTRIES;
	push_constant_buffer_desc.mappedAtCreation = false;
	push_constant_emulation_buffer = wgpuDeviceCreateBuffer(device, &push_constant_buffer_desc);

	return OK;
}

/*****************/
/**** BUFFERS ****/
/*****************/

RenderingDeviceDriverWebGpu::BufferID RenderingDeviceDriverWebGpu::buffer_create(uint64_t p_size, BitField<BufferUsageBits> p_usage, MemoryAllocationType p_allocation_type, uint64_t p_frames_drawn) {
	WGPUBufferUsage usage = webgpu_buffer_usage_from_rd(p_usage);
	uint32_t map_mode = 0;
	const bool is_dynamic = p_usage.has_flag(BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT);

	if (p_allocation_type == MemoryAllocationType::MEMORY_ALLOCATION_TYPE_GPU) {
		usage = (WGPUBufferUsage)((int)usage & ~(WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite));
	} else {
		if (usage & WGPUBufferUsage_MapRead) {
			map_mode |= WGPUMapMode_Read;
		}
		if (usage & WGPUBufferUsage_MapWrite) {
			map_mode |= WGPUMapMode_Write;
		}
	}

	if (is_dynamic) {
		usage = (WGPUBufferUsage)((int)usage & ~(WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite));
		usage = (WGPUBufferUsage)((int)usage | WGPUBufferUsage_CopyDst);
		map_mode = 0;
	}

	// Upload buffers are never mapped on the GPU side; see BufferInfo::upload_shadow.
	const bool is_upload = !is_dynamic && (map_mode & WGPUMapMode_Write);
	if (is_upload) {
		usage = (WGPUBufferUsage)(((int)usage & ~WGPUBufferUsage_MapWrite) | WGPUBufferUsage_CopyDst);
	}
	const bool maps_at_creation = false;

	// Each frame's slice of a dynamic buffer starts on a dynamic offset, which WebGPU requires to be
	// a multiple of minUniformBufferOffsetAlignment (256 at most).
	const uint64_t slice_size = is_dynamic ? STEPIFY(p_size, 256) : p_size;
	const uint64_t alloc_size = is_dynamic ? slice_size * frame_count : slice_size;

	WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
	desc.usage = usage;
	desc.size = STEPIFY(alloc_size, 256);
	desc.mappedAtCreation = maps_at_creation;
	WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);

	if (is_dynamic) {
		BufferDynamicInfo *dyn = memnew(BufferDynamicInfo);
		dyn->size = slice_size;
		dyn->buffer = buffer;
		dyn->map_mode = (WGPUMapMode)0;
		dyn->is_transfer_first_map = false;
		dyn->frame_idx = 0;
		dyn->persistent_size = slice_size * frame_count;
		dyn->persistent_ptr = (uint8_t *)memalloc(dyn->persistent_size);
		memset(dyn->persistent_ptr, 0, dyn->persistent_size);
		// Treat the buffer as "never mapped" so the first advance is unconditional.
		dyn->last_frame_mapped = p_frames_drawn - 1u;
		return BufferID(static_cast<BufferInfo *>(dyn));
	}

	BufferInfo *buffer_info = memnew(BufferInfo);
	buffer_info->size = slice_size;
	buffer_info->buffer = buffer;
	buffer_info->usage = usage;
	buffer_info->map_mode = (WGPUMapMode)map_mode;
	buffer_info->is_transfer_first_map = maps_at_creation;
	buffer_info->is_mapped = maps_at_creation;
	if (is_upload) {
		buffer_info->upload_shadow = (uint8_t *)memalloc(STEPIFY(slice_size, 4));
		memset(buffer_info->upload_shadow, 0, STEPIFY(slice_size, 4));
	}

	return BufferID(buffer_info);
}

bool RenderingDeviceDriverWebGpu::buffer_set_texel_format(BufferID p_buffer, DataFormat p_format) {
	// TODO
	return true;
}

void RenderingDeviceDriverWebGpu::buffer_free(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	wgpuBufferRelease(buffer_info->buffer);
	if (buffer_info->is_dynamic()) {
		BufferDynamicInfo *dyn = static_cast<BufferDynamicInfo *>(buffer_info);
		dirty_dynamic_buffers.erase(dyn);
		if (dyn->persistent_ptr) {
			memfree(dyn->persistent_ptr);
		}
		memdelete(dyn);
	} else {
		if (buffer_info->upload_shadow) {
			memfree(buffer_info->upload_shadow);
		}
		memdelete(buffer_info);
	}
}

uint64_t RenderingDeviceDriverWebGpu::buffer_get_allocation_size(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	return buffer_info->size;
}

static void handle_buffer_map(WGPUMapAsyncStatus status, WGPUStringView _message, void *_userdata1, void *_userdata2) {
	ERR_FAIL_COND_V_MSG(
			status != WGPUMapAsyncStatus_Success, (void)0,
			vformat("Failed to map buffer"));
}

uint8_t *RenderingDeviceDriverWebGpu::buffer_map(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	if (buffer_info->upload_shadow) {
		return buffer_info->upload_shadow;
	}

	uint64_t offset = 0;
	// WebGPU maps whole multiples of 4 bytes; the buffer is allocated in multiples of 256.
	uint64_t size = STEPIFY(buffer_info->size, 4);

	if (!buffer_info->is_transfer_first_map) {
		if (buffer_info->map_mode == WGPUMapMode_Write) {
			// TODO TODO TODO
			wgpuBufferRelease(buffer_info->buffer);
			WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
			desc.usage = buffer_info->usage;
			desc.size = STEPIFY(size, 256);
			desc.mappedAtCreation = true;
			buffer_info->buffer = wgpuDeviceCreateBuffer(device, &desc);
		} else {
			WGPUBufferMapCallbackInfo buffer_map_callback_info = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
			buffer_map_callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
			buffer_map_callback_info.callback = handle_buffer_map;
			WGPUFuture future = wgpuBufferMapAsync(
					buffer_info->buffer, buffer_info->map_mode, offset, size, buffer_map_callback_info);
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
			WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
			wait_info.future = future;
			wait_info.completed = false;
			wgpuInstanceWaitAny(context_driver->instance_get(), 1, &wait_info, UINT64_MAX);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
			(void)future;
			wgpuDevicePoll(device, true, nullptr);
#endif
		}
	} else {
		buffer_info->is_transfer_first_map = false;
	}
	buffer_info->is_mapped = true;
	void *data = (buffer_info->map_mode & WGPUMapMode_Write)
			? wgpuBufferGetMappedRange(buffer_info->buffer, offset, size)
			: (void *)wgpuBufferGetConstMappedRange(buffer_info->buffer, offset, size);
	return (uint8_t *)data;
}

void RenderingDeviceDriverWebGpu::buffer_unmap(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	if (buffer_info->upload_shadow) {
		return;
	}
	if (!buffer_info->is_mapped) {
		return;
	}
	buffer_info->is_transfer_first_map = false;
	buffer_info->is_mapped = false;

	wgpuBufferUnmap(buffer_info->buffer);
}

uint8_t *RenderingDeviceDriverWebGpu::buffer_persistent_map_advance(BufferID p_buffer, uint64_t p_frames_drawn) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	ERR_FAIL_COND_V_MSG(!buffer_info->is_dynamic(), nullptr,
			"Buffer must have BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT. Use buffer_map() instead.");
	BufferDynamicInfo *dyn = static_cast<BufferDynamicInfo *>(buffer_info);
	ERR_FAIL_COND_V_MSG(dyn->last_frame_mapped == p_frames_drawn, nullptr,
			"Buffers with BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT must only be mapped once per frame.");
	dyn->last_frame_mapped = p_frames_drawn;
	dyn->frame_idx = (dyn->frame_idx + 1u) % frame_count;
	dirty_dynamic_buffers.insert(dyn);
	return dyn->persistent_ptr + dyn->frame_idx * dyn->size;
}

uint64_t RenderingDeviceDriverWebGpu::buffer_get_dynamic_offsets(Span<BufferID> p_buffers) {
	// Pretty much what Vulkan does.
	uint64_t mask = 0u;
	uint64_t shift = 0u;
	for (const BufferID &b : p_buffers) {
		const BufferInfo *bi = (const BufferInfo *)b.id;
		if (!bi->is_dynamic()) {
			continue;
		}
		mask |= (uint64_t(bi->frame_idx) & VERTEX_DYN_MASK) << shift;
		shift += VERTEX_DYN_BITS;
	}
	return mask;
}

void RenderingDeviceDriverWebGpu::_upload_shadow_range(BufferInfo *p_buffer, uint64_t p_offset, uint64_t p_size) {
	// wgpuQueueWriteBuffer runs on the queue before the command buffer holding the copy is
	// submitted, so the copy reads what Godot wrote. Offset and size must be multiples of 4.
	uint64_t begin = p_offset & ~uint64_t(3);
	uint64_t end = MIN(STEPIFY(p_offset + p_size, 4), STEPIFY(p_buffer->size, 4));
	if (end > begin) {
		wgpuQueueWriteBuffer(queue, p_buffer->buffer, begin, p_buffer->upload_shadow + begin, end - begin);
	}
}

void RenderingDeviceDriverWebGpu::_flush_pending_dynamic_buffers() {
	for (BufferDynamicInfo *dyn : dirty_dynamic_buffers) {
		const uint64_t offset = dyn->frame_idx * dyn->size;
		wgpuQueueWriteBuffer(queue, dyn->buffer, offset, dyn->persistent_ptr + offset, dyn->size);
	}
	dirty_dynamic_buffers.clear();
}

uint64_t RenderingDeviceDriverWebGpu::buffer_get_device_address(BufferID p_buffer) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> buffer_get_device_address");
}

/*****************/
/**** TEXTURE ****/
/*****************/

WGPUTextureView RenderingDeviceDriverWebGpu::TextureInfo::get_view_with_format() const {
	if (webgpu_texture_format_is_depth_stencil(texture_view_desc.format)) {
		return this->get_depth_only_view();
	} else {
		return this->get_default_view();
	}
}

Vector<WGPUTextureView> RenderingDeviceDriverWebGpu::_texture_views_with_aspect_create(WGPUTexture p_texture, const WGPUTextureViewDescriptor &p_texture_view_descriptor) {
	if (webgpu_texture_format_is_depth_stencil(p_texture_view_descriptor.format)) {
		WGPUTextureViewDescriptor depth_only_descriptor = p_texture_view_descriptor;
		depth_only_descriptor.aspect = WGPUTextureAspect_DepthOnly;
		depth_only_descriptor.format = webgpu_texture_format_downgrade_depth_only(p_texture_view_descriptor.format);
		return {
			wgpuTextureCreateView(p_texture, &p_texture_view_descriptor),
			wgpuTextureCreateView(p_texture, &depth_only_descriptor)
		};
	} else {
		return {
			wgpuTextureCreateView(p_texture, &p_texture_view_descriptor)
		};
	}
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create(const TextureFormat &p_format, const TextureView &p_view) {
	WGPUFlags usage_bits = WGPUTextureUsage_None;
	if (p_format.usage_bits & TEXTURE_USAGE_SAMPLING_BIT) {
		usage_bits |= WGPUTextureUsage_TextureBinding;
	}
	if ((p_format.usage_bits & TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ||
			(p_format.usage_bits & TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) ||
			(p_format.usage_bits & TEXTURE_USAGE_INPUT_ATTACHMENT_BIT)) {
		usage_bits |= WGPUTextureUsage_RenderAttachment;
	}
	if ((p_format.usage_bits & TEXTURE_USAGE_CAN_UPDATE_BIT) || (p_format.usage_bits & TEXTURE_USAGE_CAN_COPY_TO_BIT)) {
		usage_bits |= WGPUTextureUsage_CopyDst;
	}
	if (p_format.usage_bits & TEXTURE_USAGE_CAN_COPY_FROM_BIT) {
		usage_bits |= WGPUTextureUsage_CopySrc;
	}
	if (p_format.usage_bits & TEXTURE_USAGE_STORAGE_BIT) {
		// Read-only storage images are read as sampled textures (tint's read_only_storage_as_sampled).
		usage_bits |= WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding;
	}

	WGPUTextureFormat texture_format = webgpu_texture_format_from_rd(p_format.format);
	WGPUTextureFormat view_format = webgpu_texture_format_from_rd(p_view.format);
	WGPUTextureUsage usage = (WGPUTextureUsage)usage_bits;
	WGPUTextureAspect aspect = webgpu_texture_aspect_from_rd_format(p_format.format);

	WGPUExtent3D size;
	size.width = p_format.width;
	size.height = p_format.height;
	size.depthOrArrayLayers = p_format.array_layers;
	WGPUTextureDimension dimension;
	bool uses_depth_or_array_layers = false;

	if (p_format.texture_type == TEXTURE_TYPE_1D) {
		dimension = WGPUTextureDimension_1D;
	} else if (p_format.texture_type == TEXTURE_TYPE_2D) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	} else if (p_format.texture_type == TEXTURE_TYPE_3D) {
		size.depthOrArrayLayers = p_format.depth;
		dimension = WGPUTextureDimension_3D;
		uses_depth_or_array_layers = true;
	} else if (p_format.texture_type == TEXTURE_TYPE_1D_ARRAY) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_1D;
	} else if (p_format.texture_type == TEXTURE_TYPE_2D_ARRAY) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	} else if (p_format.texture_type == TEXTURE_TYPE_CUBE) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	} else if (p_format.texture_type == TEXTURE_TYPE_CUBE_ARRAY) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	}

	if (p_format.samples > TextureSamples::TEXTURE_SAMPLES_1) {
		usage_bits |= WGPUTextureUsage_RenderAttachment;
	}

	uint32_t mip_level_count = p_format.mipmaps ? p_format.mipmaps : 1;

	// TODO: Assert that p_format.samples follows this behavior.
	uint32_t sample_count = pow(2, (uint32_t)p_format.samples);

	Vector<WGPUTextureFormat> view_formats;

	for (uint32_t i = 0; i < p_format.shareable_formats.size(); i++) {
		DataFormat format = p_format.shareable_formats[i];
		view_formats.push_back(webgpu_texture_format_from_rd(format));
	}
	view_formats.push_back(view_format);

	WGPUTextureDescriptor texture_desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
	texture_desc.usage = usage;
	texture_desc.dimension = dimension;
	texture_desc.size = size;
	texture_desc.format = texture_format;
	texture_desc.mipLevelCount = mip_level_count;
	texture_desc.sampleCount = sample_count;
	texture_desc.viewFormatCount = (size_t)view_formats.size();
	texture_desc.viewFormats = view_formats.ptr();

	WGPUTexture texture = wgpuDeviceCreateTexture(device, &texture_desc);

#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	WGPUTextureComponentSwizzleDescriptor texture_view_desc_extras = WGPU_TEXTURE_COMPONENT_SWIZZLE_DESCRIPTOR_INIT;
	texture_view_desc_extras.chain = {};
	texture_view_desc_extras.chain.next = nullptr;
	texture_view_desc_extras.chain.sType = (WGPUSType)WGPUSType_TextureComponentSwizzleDescriptor;
	texture_view_desc_extras.swizzle = WGPU_TEXTURE_COMPONENT_SWIZZLE_INIT;
	texture_view_desc_extras.swizzle.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r);
	texture_view_desc_extras.swizzle.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g);
	texture_view_desc_extras.swizzle.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b);
	texture_view_desc_extras.swizzle.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
	WGPUTextureViewDescriptorExtras texture_view_desc_extras = (WGPUTextureViewDescriptorExtras){
		.chain = (WGPUChainedStruct){ .next = nullptr, .sType = (WGPUSType)WGPUSType_TextureViewDescriptorExtras },
		.swizzle = (WGPUNativeTextureViewSwizzle){
				.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r),
				.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g),
				.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b),
				.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a),
		},
	};
#endif

	WGPUTextureViewDimension view_dimension = webgpu_texture_view_dimension_from_rd(p_format.texture_type);

	WGPUTextureViewDescriptor texture_view_desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
	texture_view_desc.nextInChain = texture_swizzle_supported ? (WGPUChainedStruct *)&texture_view_desc_extras : nullptr;
	texture_view_desc.format = view_format;
	texture_view_desc.dimension = view_dimension;
	texture_view_desc.mipLevelCount = texture_desc.mipLevelCount;
	texture_view_desc.arrayLayerCount = uses_depth_or_array_layers ? 1 : texture_desc.size.depthOrArrayLayers;
	texture_view_desc.aspect = aspect;

	Vector<WGPUTextureView> views = _texture_views_with_aspect_create(texture, texture_view_desc);

	TextureInfo *texture_info = memnew(TextureInfo);
	texture_info->texture = texture;
	texture_info->views = views;
	texture_info->rd_texture_format = p_format.format;
	texture_info->texture_desc = texture_desc;
	texture_info->texture_view_desc = texture_view_desc;
	texture_info->is_original_texture = true;
	texture_info->uses_depth_or_array_layers = uses_depth_or_array_layers;

	return TextureID(texture_info);
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_from_extension(uint64_t p_native_texture, TextureType p_type, DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil, uint32_t p_mipmaps) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> texture_create_from_extension");
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_shared(TextureID p_original_texture, const TextureView &p_view) {
	TextureInfo *texture_info = (TextureInfo *)p_original_texture.id;

	// HACK: We need to account for the fact that some texture formats may not support the usages of the
	// The vulkan driver does a check then unflags certain usages, but we don't have that ability.
	// Note that on wgpu vulkan, this will fail old versions of the the api (1.0 with minimal extensions).
	WGPUTextureUsage texture_view_usage = texture_info->texture_desc.usage;
	if (p_view.format == DATA_FORMAT_R8G8B8A8_SRGB) {
		if (texture_info->texture_desc.usage & WGPUTextureUsage_StorageBinding) {
			texture_view_usage &= (~WGPUTextureUsage_StorageBinding);
		}
	}

#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	WGPUTextureComponentSwizzleDescriptor texture_view_desc_extras = WGPU_TEXTURE_COMPONENT_SWIZZLE_DESCRIPTOR_INIT;
	texture_view_desc_extras.chain = {};
	texture_view_desc_extras.chain.next = nullptr;
	texture_view_desc_extras.chain.sType = (WGPUSType)WGPUSType_TextureComponentSwizzleDescriptor;
	texture_view_desc_extras.swizzle = WGPU_TEXTURE_COMPONENT_SWIZZLE_INIT;
	texture_view_desc_extras.swizzle.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r);
	texture_view_desc_extras.swizzle.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g);
	texture_view_desc_extras.swizzle.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b);
	texture_view_desc_extras.swizzle.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
	WGPUTextureViewDescriptorExtras texture_view_desc_extras = (WGPUTextureViewDescriptorExtras){
		.chain = (WGPUChainedStruct){ .next = nullptr, .sType = (WGPUSType)WGPUSType_TextureViewDescriptorExtras },
		.swizzle = (WGPUNativeTextureViewSwizzle){
				.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r),
				.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g),
				.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b),
				.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a),
		},
	};
#endif

	WGPUTextureViewDescriptor texture_view_desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
	texture_view_desc.nextInChain = texture_swizzle_supported ? (WGPUChainedStruct *)&texture_view_desc_extras : nullptr;
	texture_view_desc.format = webgpu_texture_format_from_rd(p_view.format);
	texture_view_desc.dimension = texture_info->texture_view_desc.dimension;
	texture_view_desc.baseMipLevel = texture_info->texture_view_desc.baseMipLevel;
	texture_view_desc.mipLevelCount = texture_info->texture_view_desc.mipLevelCount;
	texture_view_desc.baseArrayLayer = texture_info->texture_view_desc.baseArrayLayer;
	texture_view_desc.arrayLayerCount = texture_info->texture_view_desc.arrayLayerCount;
	texture_view_desc.aspect = texture_info->texture_view_desc.aspect;
	texture_view_desc.usage = texture_view_usage;

	Vector<WGPUTextureView> views = _texture_views_with_aspect_create(texture_info->texture, texture_view_desc);

	TextureInfo *new_texture_info = memnew(TextureInfo);
	*new_texture_info = *texture_info;
	new_texture_info->views = views;
	new_texture_info->storage_view = nullptr;
	new_texture_info->is_original_texture = false;
	new_texture_info->texture_view_desc = texture_view_desc;

	return TextureID(new_texture_info);
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_shared_from_slice(TextureID p_original_texture, const TextureView &p_view, TextureSliceType p_slice_type, uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) {
	TextureInfo *texture_info = (TextureInfo *)p_original_texture.id;

#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	WGPUTextureComponentSwizzleDescriptor texture_view_desc_extras = WGPU_TEXTURE_COMPONENT_SWIZZLE_DESCRIPTOR_INIT;
	texture_view_desc_extras.chain = {};
	texture_view_desc_extras.chain.next = nullptr;
	texture_view_desc_extras.chain.sType = (WGPUSType)WGPUSType_TextureComponentSwizzleDescriptor;
	texture_view_desc_extras.swizzle = WGPU_TEXTURE_COMPONENT_SWIZZLE_INIT;
	texture_view_desc_extras.swizzle.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r);
	texture_view_desc_extras.swizzle.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g);
	texture_view_desc_extras.swizzle.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b);
	texture_view_desc_extras.swizzle.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
	WGPUTextureViewDescriptorExtras texture_view_desc_extras = (WGPUTextureViewDescriptorExtras){
		.chain = (WGPUChainedStruct){ .next = nullptr, .sType = (WGPUSType)WGPUSType_TextureViewDescriptorExtras },
		.swizzle = (WGPUNativeTextureViewSwizzle){
				.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r),
				.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g),
				.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b),
				.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a),
		},
	};
#endif

	WGPUTextureFormat view_format = webgpu_texture_format_from_rd(p_view.format);
	WGPUTextureAspect aspect = webgpu_texture_aspect_from_rd_format(p_view.format);
	WGPUTextureViewDescriptor texture_view_desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
	texture_view_desc.nextInChain = texture_swizzle_supported ? (WGPUChainedStruct *)&texture_view_desc_extras : nullptr;
	texture_view_desc.format = view_format;
	texture_view_desc.dimension = texture_info->texture_view_desc.dimension;
	texture_view_desc.baseMipLevel = p_mipmap;
	texture_view_desc.mipLevelCount = p_mipmaps;
	texture_view_desc.baseArrayLayer = p_layer;
	texture_view_desc.arrayLayerCount = p_layers;
	texture_view_desc.aspect = aspect;
	texture_view_desc.usage = texture_info->texture_view_desc.usage;

	switch (p_slice_type) {
		case RenderingDeviceCommons::TEXTURE_SLICE_2D:
			texture_view_desc.dimension = WGPUTextureViewDimension_2D;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_CUBEMAP:
			texture_view_desc.dimension = WGPUTextureViewDimension_Cube;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_3D:
			texture_view_desc.dimension = WGPUTextureViewDimension_3D;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_2D_ARRAY:
			texture_view_desc.dimension = WGPUTextureViewDimension_2DArray;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_MAX:
			return TextureID();
	}

	Vector<WGPUTextureView> views = _texture_views_with_aspect_create(texture_info->texture, texture_view_desc);

	TextureInfo *new_texture_info = memnew(TextureInfo);
	*new_texture_info = *texture_info;
	new_texture_info->views = views;
	new_texture_info->storage_view = nullptr;
	new_texture_info->is_original_texture = false;
	new_texture_info->texture_view_desc = texture_view_desc;

	return TextureID(new_texture_info);
}

void RenderingDeviceDriverWebGpu::texture_free(TextureID p_texture) {
	TextureInfo *texture_info = (TextureInfo *)p_texture.id;
	if (texture_info->is_original_texture) {
		wgpuTextureRelease(texture_info->texture);
	}
	for (int i = 0; i < texture_info->views.size(); i++) {
		wgpuTextureViewRelease(texture_info->views[i]);
	}
	if (texture_info->storage_view) {
		wgpuTextureViewRelease(texture_info->storage_view);
	}
	memdelete(texture_info);
}

uint64_t RenderingDeviceDriverWebGpu::texture_get_allocation_size(TextureID p_texture) {
	// TODO
	return 1;
}

void RenderingDeviceDriverWebGpu::texture_get_copyable_layout(
		TextureID p_texture,
		const TextureSubresource &p_subresource,
		TextureCopyableLayout *r_layout) {
	TextureInfo *texture_info = (TextureInfo *)p_texture.id;

	FormatBlockDimension block_dimensions = webgpu_texture_format_block_dimensions(texture_info->texture_desc.format);
	uint32_t bytes_per_block =
			webgpu_texture_format_block_copy_size(
					texture_info->texture_desc.format,
					webgpu_texture_aspect_from_rd(
							(TextureAspectBits)(1 << p_subresource.aspect)));

	uint32_t block_width = block_dimensions.block_dim_x;
	uint32_t block_height = block_dimensions.block_dim_y;

	uint32_t width = texture_info->texture_desc.size.width;
	uint32_t height = texture_info->texture_desc.size.height;
	uint32_t depth = texture_info->texture_desc.size.depthOrArrayLayers;

	uint32_t blocks_per_row =
			(width + block_width - 1) / block_width;

	uint32_t blocks_per_column =
			(height + block_height - 1) / block_height;

	r_layout->row_pitch =
			STEPIFY(blocks_per_row * bytes_per_block, 256);
	r_layout->size =
			r_layout->row_pitch * blocks_per_column * depth;
}

Vector<uint8_t> RenderingDeviceDriverWebGpu::texture_get_data(TextureID p_texture, uint32_t p_layer) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> texture_get_data");
}

// WebGPU spec 26.1.1 "Plain color formats": formats usable as write-only and read-only storage
// textures without any optional feature ("core-features-and-limits" is always on for core devices).
static bool webgpu_storage_format_is_core(WGPUTextureFormat p_format) {
	switch (p_format) {
		case WGPUTextureFormat_RGBA8Unorm:
		case WGPUTextureFormat_RGBA8Snorm:
		case WGPUTextureFormat_RGBA8Uint:
		case WGPUTextureFormat_RGBA8Sint:
		case WGPUTextureFormat_RGBA16Uint:
		case WGPUTextureFormat_RGBA16Sint:
		case WGPUTextureFormat_RGBA16Float:
		case WGPUTextureFormat_R32Uint:
		case WGPUTextureFormat_R32Sint:
		case WGPUTextureFormat_R32Float:
		case WGPUTextureFormat_RG32Uint:
		case WGPUTextureFormat_RG32Sint:
		case WGPUTextureFormat_RG32Float:
		case WGPUTextureFormat_RGBA32Uint:
		case WGPUTextureFormat_RGBA32Sint:
		case WGPUTextureFormat_RGBA32Float:
			return true;
		default:
			return false;
	}
}

// The same table: formats that become storage formats with "texture-formats-tier1".
static bool webgpu_storage_format_is_tier1(WGPUTextureFormat p_format) {
	switch (p_format) {
		case WGPUTextureFormat_R8Unorm:
		case WGPUTextureFormat_R8Uint:
		case WGPUTextureFormat_R8Sint:
		case WGPUTextureFormat_RG8Unorm:
		case WGPUTextureFormat_RG8Uint:
		case WGPUTextureFormat_RG8Sint:
		case WGPUTextureFormat_R16Uint:
		case WGPUTextureFormat_R16Sint:
		case WGPUTextureFormat_R16Float:
		case WGPUTextureFormat_RG16Uint:
		case WGPUTextureFormat_RG16Sint:
		case WGPUTextureFormat_RG16Float:
		case WGPUTextureFormat_RGB10A2Uint:
		case WGPUTextureFormat_RGB10A2Unorm:
		case WGPUTextureFormat_RG11B10Ufloat:
			return true;
		default:
			return false;
	}
}

BitField<RenderingDeviceDriver::TextureUsageBits> RenderingDeviceDriverWebGpu::texture_get_usages_supported_by_format(DataFormat p_format, bool p_cpu_readable) {
	const WGPUTextureFormat wgpu_format = webgpu_texture_format_from_rd(p_format);
	for (WGPUTextureFormat format : WEBGPU_CORE_SUPPORTED_FORMATS) {
		if (wgpu_format == format) {
			// Only the storage column is checked here; the other usages are still reported as supported.
			BitField<RDD::TextureUsageBits> supported = INT64_MAX;
			const bool storage = webgpu_storage_format_is_core(wgpu_format) ||
					(texture_formats_tier1_supported && webgpu_storage_format_is_tier1(wgpu_format));
			if (!storage) {
				supported.clear_flag(TEXTURE_USAGE_STORAGE_BIT);
				supported.clear_flag(TEXTURE_USAGE_STORAGE_ATOMIC_BIT);
			}
			return supported;
		}
	}

	return 0;
}

bool RenderingDeviceDriverWebGpu::texture_can_make_shared_with_format(TextureID p_texture, DataFormat p_format, bool &r_raw_reinterpretation) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> texture_can_make_shared_with_format");
	return true;
}

/*****************/
/**** SAMPLER ****/
/*****************/

RenderingDeviceDriver::SamplerID RenderingDeviceDriverWebGpu::sampler_create(const SamplerState &p_state) {
	// STUB: Samplers with anisotropy enabled cannot support nearest filtering.
	// See https://gpuweb.github.io/gpuweb/#sampler-creation
	WGPUSamplerDescriptor sampler_desc = WGPU_SAMPLER_DESCRIPTOR_INIT;
	sampler_desc.addressModeU = webgpu_address_mode_from_rd(p_state.repeat_u);
	sampler_desc.addressModeV = webgpu_address_mode_from_rd(p_state.repeat_v);
	sampler_desc.addressModeW = webgpu_address_mode_from_rd(p_state.repeat_w);
	sampler_desc.magFilter = p_state.use_anisotropy ? WGPUFilterMode_Linear : webgpu_filter_mode_from_rd(p_state.mag_filter);
	sampler_desc.minFilter = p_state.use_anisotropy ? WGPUFilterMode_Linear : webgpu_filter_mode_from_rd(p_state.min_filter);
	sampler_desc.mipmapFilter = p_state.use_anisotropy ? WGPUMipmapFilterMode_Linear : webgpu_mipmap_filter_mode_from_rd(p_state.mip_filter);
	// NOTE: `min_lod` cannot be negative.
	// See https://www.w3.org/TR/webgpu/#sampler-creation
	sampler_desc.lodMinClamp = p_state.min_lod < 0.0f ? 0.0f : p_state.min_lod;
	sampler_desc.lodMaxClamp = p_state.max_lod;
	sampler_desc.compare = p_state.enable_compare ? webgpu_compare_mode_from_rd(p_state.compare_op) : WGPUCompareFunction_Undefined;
	sampler_desc.maxAnisotropy = p_state.use_anisotropy ? (uint16_t)p_state.anisotropy_max : (uint16_t)1;

	WGPUSampler sampler = wgpuDeviceCreateSampler(device, &sampler_desc);
	sampler_descs[sampler] = sampler_desc;
	return SamplerID(sampler);
}

void RenderingDeviceDriverWebGpu::sampler_free(SamplerID p_sampler) {
	WGPUSampler sampler = (WGPUSampler)p_sampler.id;
	WGPUSampler *nearest = nearest_samplers.getptr(sampler);
	if (nearest) {
		wgpuSamplerRelease(*nearest);
		nearest_samplers.erase(sampler);
	}
	sampler_descs.erase(sampler);
	wgpuSamplerRelease(sampler);
}

WGPUSampler RenderingDeviceDriverWebGpu::_sampler_get_non_filtering(WGPUSampler p_sampler) {
	const WGPUSamplerDescriptor *desc = sampler_descs.getptr(p_sampler);
	if (!desc || desc->compare != WGPUCompareFunction_Undefined ||
			(desc->magFilter != WGPUFilterMode_Linear && desc->minFilter != WGPUFilterMode_Linear && desc->mipmapFilter != WGPUMipmapFilterMode_Linear)) {
		return p_sampler;
	}
	WGPUSampler *nearest = nearest_samplers.getptr(p_sampler);
	if (nearest) {
		return *nearest;
	}
	WGPUSamplerDescriptor nearest_desc = *desc;
	nearest_desc.magFilter = WGPUFilterMode_Nearest;
	nearest_desc.minFilter = WGPUFilterMode_Nearest;
	nearest_desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
	nearest_desc.maxAnisotropy = 1;
	WGPUSampler created = wgpuDeviceCreateSampler(device, &nearest_desc);
	nearest_samplers[p_sampler] = created;
	return created;
}

bool RenderingDeviceDriverWebGpu::sampler_is_format_supported_for_filter(DataFormat _p_format, SamplerFilter p_filter) {
	// "descriptor.magFilter, descriptor.minFilter, and descriptor.mipmapFilter must be "linear"."
	return p_filter == SamplerFilter::SAMPLER_FILTER_LINEAR;
}

/**********************/
/**** VERTEX ARRAY ****/
/**********************/

// NOTE: The attributes in `p_vertex_attribs` must be in order.
RenderingDeviceDriverWebGpu::VertexFormatID RenderingDeviceDriverWebGpu::vertex_format_create(Span<VertexAttribute> p_vertex_attribs, const VertexAttributeBindingsMap &p_vertex_bindings) {
	VertexFormatInfo *vertex_format_info = memnew(VertexFormatInfo);

	uint32_t slot_count = 0;
	for (const KeyValue<uint32_t, VertexAttributeBinding> &kv : p_vertex_bindings) {
		slot_count = MAX(slot_count, kv.key + 1);
	}

	HashMap<uint32_t, Vector<WGPUVertexAttribute>> attrs_by_binding;
	for (uint32_t i = 0; i < p_vertex_attribs.size(); i++) {
		const VertexAttribute &attrib = p_vertex_attribs[i];
		uint32_t binding = (attrib.binding == UINT32_MAX) ? i : attrib.binding;
		WGPUVertexAttribute vertex_attribute = WGPU_VERTEX_ATTRIBUTE_INIT;
		vertex_attribute.format = webgpu_vertex_format_from_rd(attrib.format);
		vertex_attribute.offset = attrib.offset;
		vertex_attribute.shaderLocation = attrib.location;
		attrs_by_binding[binding].push_back(vertex_attribute);
	}

	HashMap<uint32_t, uint32_t> binding_attr_offsets;
	for (const KeyValue<uint32_t, VertexAttributeBinding> &kv : p_vertex_bindings) {
		uint32_t binding = kv.key;
		Vector<WGPUVertexAttribute> *attrs = attrs_by_binding.getptr(binding);
		binding_attr_offsets[binding] = vertex_format_info->vertex_attributes.size();
		if (attrs) {
			vertex_format_info->vertex_attributes.append_array(*attrs);
		}
	}

	vertex_format_info->layouts.resize_initialized(slot_count);
	const WGPUVertexAttribute *attr_base = vertex_format_info->vertex_attributes.ptr();
	for (const KeyValue<uint32_t, VertexAttributeBinding> &kv : p_vertex_bindings) {
		uint32_t binding = kv.key;
		const VertexAttributeBinding &binding_info = kv.value;
		Vector<WGPUVertexAttribute> *attrs = attrs_by_binding.getptr(binding);
		size_t attr_count = attrs ? attrs->size() : 0;
		WGPUVertexStepMode step_mode = binding_info.frequency == VERTEX_FREQUENCY_VERTEX
				? WGPUVertexStepMode_Vertex
				: WGPUVertexStepMode_Instance;
		vertex_format_info->layouts.write[binding] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
		vertex_format_info->layouts.write[binding].stepMode = step_mode;
		vertex_format_info->layouts.write[binding].arrayStride = binding_info.stride;
		vertex_format_info->layouts.write[binding].attributeCount = attr_count;
		vertex_format_info->layouts.write[binding].attributes = attr_count > 0 ? attr_base + binding_attr_offsets[binding] : nullptr;
	}

	return VertexFormatID(vertex_format_info);
}

void RenderingDeviceDriverWebGpu::vertex_format_free(VertexFormatID p_vertex_format) {
	VertexFormatInfo *vertex_format_info = (VertexFormatInfo *)p_vertex_format.id;
	memdelete(vertex_format_info);
}

/******************/
/**** BARRIERS ****/
/******************/

void RenderingDeviceDriverWebGpu::command_pipeline_barrier(
		CommandBufferID p_cmd_buffer,
		BitField<PipelineStageBits> p_src_stages,
		BitField<PipelineStageBits> p_dst_stages,
		VectorView<MemoryAccessBarrier> p_memory_barriers,
		VectorView<BufferBarrier> p_buffer_barriers,
		VectorView<TextureBarrier> p_texture_barriers,
		VectorView<AccelerationStructureBarrier> p_acceleration_structure_barriers) {
	// Empty.
}

/****************/
/**** FENCES ****/
/****************/

RenderingDeviceDriver::FenceID RenderingDeviceDriverWebGpu::fence_create() {
	// The usage of fences in godot to sync frames is already handled by WebGpu.
	return FenceID(1);
}

Error RenderingDeviceDriverWebGpu::fence_wait(FenceID _p_fence) {
	return OK;
}

void RenderingDeviceDriverWebGpu::fence_free(FenceID p_fence) {
	// Empty.
}

/********************/
/**** SEMAPHORES ****/
/********************/

RenderingDeviceDriver::SemaphoreID RenderingDeviceDriverWebGpu::semaphore_create() {
	// The usage of fences in godot to sync frames is already handled by WebGpu.
	return SemaphoreID(1);
}

void RenderingDeviceDriverWebGpu::semaphore_free(SemaphoreID _p_semaphore) {
	// Empty.
}

/******************/
/**** COMMANDS ****/
/******************/

// ----- QUEUE FAMILY -----

RenderingDeviceDriver::CommandQueueFamilyID RenderingDeviceDriverWebGpu::command_queue_family_get(BitField<CommandQueueFamilyBits> _p_cmd_queue_family_bits, RenderingContextDriver::SurfaceID _p_surface) {
	// WebGpu has no concept of queue families, so this value is unused.
	return CommandQueueFamilyID(1);
}

// ----- QUEUE -----

RenderingDeviceDriver::CommandQueueID RenderingDeviceDriverWebGpu::command_queue_create(CommandQueueFamilyID _p_cmd_queue_family, bool _p_identify_as_main_queue) {
	// WebGpu has only one queue, so this value is unused.
	return CommandQueueID(1);
}

Error RenderingDeviceDriverWebGpu::command_queue_execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains) {
	Vector<WGPUCommandBuffer> commands = Vector<WGPUCommandBuffer>();

	for (uint32_t i = 0; i < p_cmd_buffers.size(); i++) {
		CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffers[i].id;

		DEV_ASSERT(command_buffer_info != nullptr);
		DEV_ASSERT(command_buffer_info->encoder != nullptr);

		WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_buffer_info->encoder, nullptr);
		commands.push_back(command_buffer);

		wgpuCommandEncoderRelease(command_buffer_info->encoder);
		command_buffer_info->encoder = nullptr;
	}

	_flush_pending_dynamic_buffers();
	wgpuQueueSubmit(queue, commands.size(), commands.ptr());

	// Flush push constant emulation buffer for the next command buffer.
	push_constant_emulation_head = 0;

	for (uint32_t i = 0; i < commands.size(); i++) {
		WGPUCommandBuffer command_buffer = commands[i];
		wgpuCommandBufferRelease(command_buffer);
	}

	// TODO: IMPL
	// Q: Will we get multiple surfaces?
	// Only needed for desktop.
#if !defined(WEBGPU_BACKEND_EMDAWN)
	for (uint32_t i = 0; i < p_swap_chains.size(); i++) {
		SwapChainInfo *swapchain = (SwapChainInfo *)p_swap_chains[i].id;
		RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain->surface;

		wgpuSurfacePresent(surface->surface);
	}
#endif

	return OK;
}

void RenderingDeviceDriverWebGpu::command_queue_free(CommandQueueID _p_cmd_queue) {
	// Empty.
}

// ----- POOL -----

RenderingDeviceDriver::CommandPoolID RenderingDeviceDriverWebGpu::command_pool_create(CommandQueueFamilyID _p_cmd_queue_family, CommandBufferType _p_cmd_buffer_type) {
	TightLocalVector<CommandBufferInfo *> *command_pool = memnew(TightLocalVector<CommandBufferInfo *>);
	return CommandPoolID(command_pool);
}

bool RenderingDeviceDriverWebGpu::command_pool_reset(CommandPoolID p_cmd_pool) {
	// TODO: impl
	return true;
}

void RenderingDeviceDriverWebGpu::command_pool_free(CommandPoolID p_cmd_pool) {
	TightLocalVector<CommandBufferInfo *> *command_pool = (TightLocalVector<CommandBufferInfo *> *)p_cmd_pool.id;
	for (uint32_t i = 0; i < command_pool->size(); i++) {
		memfree(command_pool->ptr()[i]);
	}
	memfree(command_pool);
}

// ----- BUFFER -----

uint32_t RenderingDeviceDriverWebGpu::_push_constant_emulation_alloc() {
	const uint32_t capacity = WEBGPU_MAX_IMMEDIATE_SIZE * WEBGPU_PUSH_CONSTANT_EMULATION_BUFFER_ENTRIES;
	if (push_constant_emulation_head + WEBGPU_MAX_IMMEDIATE_SIZE > capacity) {
		ERR_PRINT_ONCE(vformat("WebGpu push constant emulation buffer filled, rendering will be incorrect."));
		return capacity - WEBGPU_MAX_IMMEDIATE_SIZE;
	}
	uint32_t offset = push_constant_emulation_head;
	push_constant_emulation_head += WEBGPU_MAX_IMMEDIATE_SIZE;
	return offset;
}

void RenderingDeviceDriverWebGpu::_flush_active_command_pass(CommandBufferInfo &p_command_buffer_info) {
	// State tracking relating to push constant emulation
	struct PushConstantBindState {
		ShaderInfo *shader = nullptr;
		uint32_t current_offset = 0;
		uint32_t bound_offset = UINT32_MAX;

		WGPUBindGroup group = nullptr;
		WGPUBindGroupLayout group_layout = nullptr;
		Vector<uint32_t> group_offsets;

		bool has_push_constant() const {
			return shader != nullptr && shader->push_constant_size > 0 && shader->push_constant_set_index >= 0;
		}
		bool is_standalone() const {
			return shader != nullptr && shader->standalone_push_constant_group != nullptr;
		}

		uint32_t get_set_index() const {
			return (uint32_t)shader->push_constant_set_index;
		}

		WGPUBindGroupLayout get_layout(const ShaderInfo *p_shader) const {
			if (!p_shader || p_shader->push_constant_set_index < 0) {
				return nullptr;
			}
			uint32_t idx = (uint32_t)p_shader->push_constant_set_index;
			return idx < (uint32_t)p_shader->bind_group_layouts.size() ? p_shader->bind_group_layouts[idx] : nullptr;
		}
		void set_shader(ShaderInfo *p_shader) {
			if (get_layout(p_shader) != group_layout) {
				group = nullptr;
				group_layout = nullptr;
				group_offsets.clear();
			}
			shader = p_shader;
			bound_offset = UINT32_MAX;
			if (is_standalone()) {
				group = shader->standalone_push_constant_group;
				group_layout = shader->standalone_push_constant_layout;
				group_offsets.clear();
			}
		}

		Vector<uint32_t> get_offsets_for(uint32_t p_group_index, const Vector<uint32_t> &p_base) const {
			Vector<uint32_t> offsets = p_base;
			if (has_push_constant() && !is_standalone() && p_group_index == get_set_index()) {
				offsets.push_back(current_offset);
			}
			return offsets;
		}

		Vector<uint32_t> get_mock_offsets(uint32_t p_group_index) const {
			Vector<uint32_t> offsets;
			if (shader && p_group_index < (uint32_t)shader->set_dynamic_offset_counts.size()) {
				for (uint32_t i = 0; i < shader->set_dynamic_offset_counts[p_group_index]; i++) {
					offsets.push_back(0);
				}
			}
			return get_offsets_for(p_group_index, offsets);
		}

		void on_set_bind_group(uint32_t p_group_index, WGPUBindGroup p_group, const Vector<uint32_t> &p_offsets) {
			if (!has_push_constant() || is_standalone() || p_group_index != get_set_index()) {
				return;
			}
			group = p_group;
			group_layout = get_layout(shader);
			group_offsets = p_offsets;
			bound_offset = current_offset;
		}

		// Returns the group to re-bind before a draw, or nullptr when what is bound is already right.
		WGPUBindGroup take_rebind(uint32_t &r_set_index) {
			if (!has_push_constant() || bound_offset == current_offset || group == nullptr) {
				return nullptr;
			}
			if (group_offsets.is_empty()) {
				group_offsets.push_back(current_offset);
			} else {
				group_offsets.write[group_offsets.size() - 1] = current_offset;
			}
			bound_offset = current_offset;
			r_set_index = get_set_index();
			return group;
		}

		void rebind_render(WGPURenderPassEncoder p_encoder) {
			uint32_t set_idx = 0;
			WGPUBindGroup rebind = take_rebind(set_idx);
			if (rebind) {
				wgpuRenderPassEncoderSetBindGroup(p_encoder, set_idx, rebind, (size_t)group_offsets.size(), group_offsets.ptr());
			}
		}

		void rebind_compute(WGPUComputePassEncoder p_encoder) {
			uint32_t set_idx = 0;
			WGPUBindGroup rebind = take_rebind(set_idx);
			if (rebind) {
				wgpuComputePassEncoderSetBindGroup(p_encoder, set_idx, rebind, (size_t)group_offsets.size(), group_offsets.ptr());
			}
		}
	};

	WGPUComputePassEncoder compute_encoder = nullptr;
	if (p_command_buffer_info.has_compute_commands) {
		WGPUComputePassDescriptor compute_pass_descriptor = (WGPUComputePassDescriptor){};
		compute_encoder = wgpuCommandEncoderBeginComputePass(p_command_buffer_info.encoder, &compute_pass_descriptor);
		p_command_buffer_info.has_compute_commands = false;
	}

	// `wgpu` skips binding most bind groups if one is missing.
	// To remedy this, we need to bind missing bind groups after setting the pipeline but before an "effective" command.
	// Additionally, we need to preserve previously bound groups on compatible bind group layouts across pipelines.

	PipelineInfo *current_pipeline = nullptr;
	HashMap<uint32_t, Vector<Pair<uint32_t, WGPUBindGroup>>> render_mock_bind_groups;
	HashMap<uint32_t, Vector<Pair<uint32_t, WGPUBindGroup>>> compute_mock_bind_groups;
	HashMap<uint32_t, WGPUBindGroupLayout> bound_layouts;

	for (uint32_t i = 0; i < p_command_buffer_info.commands.size(); i++) {
		const PassEncoderCommand &command = p_command_buffer_info.commands[i];

		if (command.type == PassEncoderCommand::CommandType::RENDER_SET_PIPELINE || command.type == PassEncoderCommand::CommandType::COMPUTE_SET_PIPELINE) {
			// Check if this pipeline has a compatible set of bind group layouts.
			// We want to keep the pipeline if so.
			bool clear = false;
			for (KeyValue<uint32_t, WGPUBindGroupLayout> &kv : bound_layouts) {
				ShaderInfo *shader_info = (ShaderInfo *)command.set_pipeline.pipeline_info->shader_id.id;
				if (kv.key >= shader_info->bind_group_layouts.size() || shader_info->bind_group_layouts[kv.key] != kv.value) {
					clear = true;
					break;
				}
			}

			if (clear) {
				bound_layouts.clear();
			}

			current_pipeline = command.set_pipeline.pipeline_info;
		}

		if (current_pipeline) {
			if (command.type == PassEncoderCommand::CommandType::RENDER_SET_BIND_GROUP || command.type == PassEncoderCommand::CommandType::COMPUTE_SET_BIND_GROUP) {
				PassEncoderCommand::SetBindGroup data = command.set_bind_group;

				if (ShaderID(data.shader_info) == current_pipeline->shader_id) {
					bound_layouts.insert(data.group_index, data.shader_info->bind_group_layouts[data.group_index]);
				}
			}
			if (command.is_draw_call() || command.is_dispatch_call()) {
				ShaderInfo *shader_info = (ShaderInfo *)current_pipeline->shader_id.id;
				if (shader_info) {
					HashMap<uint32_t, Vector<Pair<uint32_t, WGPUBindGroup>>> *mock_bind_groups = nullptr;
					if (command.is_draw_call()) {
						mock_bind_groups = &render_mock_bind_groups;
					} else if (command.is_dispatch_call()) {
						mock_bind_groups = &compute_mock_bind_groups;
					}

					mock_bind_groups->insert(i, Vector<Pair<uint32_t, WGPUBindGroup>>());
					Vector<Pair<uint32_t, WGPUBindGroup>> &groups = (*mock_bind_groups)[i];

					for (uint32_t set_idx = 0; set_idx < shader_info->bind_group_layout_descs.size(); set_idx++) {
						const WGPUBindGroupLayoutDescriptor &desc = shader_info->bind_group_layout_descs[set_idx];
						if (!bound_layouts.has(set_idx)) {
							WGPUBindGroup mock_group = this->_mock_bind_group_create_or_get(
									desc,
									shader_info->bind_group_layouts[set_idx],
									shader_info->set_binding_remaps[set_idx],
									shader_info->used_original_bindings_map[set_idx],
									shader_info->_set_index_has_push_constant_emulation(set_idx) ? shader_info->push_constant_size : 0);
							groups.push_back(Pair(set_idx, mock_group));
						}
					}
				}
			}
		}
	}

	// Fill up our push constant emulation buffer in our render order (compute then render).
	for (uint32_t i = 0; i < p_command_buffer_info.commands.size(); i++) {
		PassEncoderCommand &command = p_command_buffer_info.commands.write[i];
		const Vector<uint8_t> *payload = nullptr;
		uint32_t payload_offset = 0;

		if (command.type == PassEncoderCommand::CommandType::COMPUTE_SET_IMMEDIATES) {
			command.compute_set_immediates.emulation_offset = _push_constant_emulation_alloc();
			payload = &command.compute_push_constants;
			payload_offset = command.compute_set_immediates.emulation_offset + command.compute_set_immediates.offset;
		} else if (command.type == PassEncoderCommand::CommandType::RENDER_SET_IMMEDIATES) {
			command.render_set_immediates.emulation_offset = _push_constant_emulation_alloc();
			payload = &command.render_push_constants;
			payload_offset = command.render_set_immediates.emulation_offset + command.render_set_immediates.offset;
		}
		if (payload && payload->size() > 0) {
			wgpuQueueWriteBuffer(
					queue,
					push_constant_emulation_buffer,
					payload_offset,
					payload->ptr(),
					payload->size());
		}
	}

	if (compute_encoder) {
		PushConstantBindState pc_state;

		for (uint32_t i = 0; i < p_command_buffer_info.commands.size(); i++) {
			const PassEncoderCommand &command = p_command_buffer_info.commands.write[i];

			if (compute_mock_bind_groups.has(i)) {
				const Vector<Pair<uint32_t, WGPUBindGroup>> &mock_bindings = compute_mock_bind_groups.get(i);
				for (uint32_t mb_idx = 0; mb_idx < mock_bindings.size(); mb_idx++) {
					uint32_t set_idx = mock_bindings[mb_idx].first;
					WGPUBindGroup bind_group = mock_bindings[mb_idx].second;
					Vector<uint32_t> offsets = pc_state.get_mock_offsets(set_idx);
					pc_state.on_set_bind_group(set_idx, bind_group, offsets);
					wgpuComputePassEncoderSetBindGroup(compute_encoder, set_idx, bind_group,
							(size_t)offsets.size(),
							offsets.is_empty() ? nullptr : offsets.ptr());
				}
			}

			switch (command.type) {
				case PassEncoderCommand::CommandType::COMPUTE_SET_PIPELINE: {
					PassEncoderCommand::SetPipeline data = command.set_pipeline;
					wgpuComputePassEncoderSetPipeline(compute_encoder, data.pipeline_info->compute_pipeline);
					pc_state.set_shader((ShaderInfo *)data.pipeline_info->shader_id.id);
				} break;
				case PassEncoderCommand::CommandType::COMPUTE_SET_BIND_GROUP: {
					PassEncoderCommand::SetBindGroup data = command.set_bind_group;

					Vector<uint32_t> offsets = pc_state.get_offsets_for(data.group_index, command.dynamic_offsets);
					pc_state.on_set_bind_group(data.group_index, data.bind_group, offsets);

					wgpuComputePassEncoderSetBindGroup(compute_encoder, data.group_index, data.bind_group,
							(size_t)offsets.size(),
							offsets.is_empty() ? nullptr : offsets.ptr());
				} break;
				case PassEncoderCommand::CommandType::COMPUTE_SET_IMMEDIATES: {
					pc_state.current_offset = command.compute_set_immediates.emulation_offset;
				} break;
				case PassEncoderCommand::CommandType::COMPUTE_DISPATCH_WORKGROUPS: {
					pc_state.rebind_compute(compute_encoder);
					PassEncoderCommand::ComputeDispatchWorkgroups data = command.compute_dispatch_workgroups;
					wgpuComputePassEncoderDispatchWorkgroups(compute_encoder, data.workgroup_count_x, data.workgroup_count_y, data.workgroup_count_z);
				} break;
				case PassEncoderCommand::CommandType::COMPUTE_DISPATCH_WORKGROUPS_INDIRECT: {
					pc_state.rebind_compute(compute_encoder);
					PassEncoderCommand::ComputeDispatchWorkgroupsIndirect data = command.compute_dispatch_workgroups_indirect;
					wgpuComputePassEncoderDispatchWorkgroupsIndirect(compute_encoder, data.indirect_buffer, data.indirect_offset);
				} break;
				default:
					break;
			}
		}

		wgpuComputePassEncoderEnd(compute_encoder);
		wgpuComputePassEncoderRelease(compute_encoder);
	}

	WGPURenderPassEncoder render_encoder = nullptr;
	if (p_command_buffer_info.is_render_pass_active) {
		WGPURenderPassDescriptor render_pass_descriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
		render_pass_descriptor.colorAttachmentCount = (uint32_t)p_command_buffer_info.active_render_pass_info.color_attachments.size();
		render_pass_descriptor.colorAttachments = p_command_buffer_info.active_render_pass_info.color_attachments.ptr();
		render_pass_descriptor.depthStencilAttachment = p_command_buffer_info.active_render_pass_info.depth_stencil_attachment.second ? &p_command_buffer_info.active_render_pass_info.depth_stencil_attachment.first : nullptr;
		render_encoder = wgpuCommandEncoderBeginRenderPass(p_command_buffer_info.encoder, &render_pass_descriptor);
		p_command_buffer_info.is_render_pass_active = false;
	}

	if (render_encoder) {
		PushConstantBindState pc_state;

		for (uint32_t i = 0; i < p_command_buffer_info.commands.size(); i++) {
			PassEncoderCommand &command = p_command_buffer_info.commands.write[i];

			if (render_mock_bind_groups.has(i)) {
				const Vector<Pair<uint32_t, WGPUBindGroup>> &mock_bindings = render_mock_bind_groups.get(i);
				for (uint32_t mb_idx = 0; mb_idx < mock_bindings.size(); mb_idx++) {
					uint32_t set_idx = mock_bindings[mb_idx].first;

					WGPUBindGroup bind_group = mock_bindings[mb_idx].second;
					Vector<uint32_t> offsets = pc_state.get_mock_offsets(set_idx);
					pc_state.on_set_bind_group(set_idx, bind_group, offsets);
					wgpuRenderPassEncoderSetBindGroup(render_encoder, set_idx, bind_group,
							(size_t)offsets.size(),
							offsets.is_empty() ? nullptr : offsets.ptr());
				}
			}

			if (command.is_draw_call()) {
				pc_state.rebind_render(render_encoder);
			}

			switch (command.type) {
				case PassEncoderCommand::CommandType::RENDER_SET_VIEWPORT: {
					PassEncoderCommand::RenderSetViewport data = command.render_set_viewport;
					wgpuRenderPassEncoderSetViewport(render_encoder, data.x, data.y, data.width, data.height, data.min_depth, data.max_depth);
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_SCISSOR_RECT: {
					PassEncoderCommand::RenderSetScissorRect data = command.render_set_scissor_rect;
					wgpuRenderPassEncoderSetScissorRect(render_encoder, data.x, data.y, data.width, data.height);
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_PIPELINE: {
					PassEncoderCommand::SetPipeline data = command.set_pipeline;
					wgpuRenderPassEncoderSetPipeline(render_encoder, data.pipeline_info->render_pipeline);
					pc_state.set_shader((ShaderInfo *)data.pipeline_info->shader_id.id);
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_BIND_GROUP: {
					PassEncoderCommand::SetBindGroup data = command.set_bind_group;

					Vector<uint32_t> offsets = pc_state.get_offsets_for(data.group_index, command.dynamic_offsets);
					pc_state.on_set_bind_group(data.group_index, data.bind_group, offsets);

					wgpuRenderPassEncoderSetBindGroup(render_encoder, data.group_index, data.bind_group,
							(size_t)offsets.size(),
							offsets.is_empty() ? nullptr : offsets.ptr());
				} break;
				case PassEncoderCommand::CommandType::RENDER_DRAW: {
					PassEncoderCommand::RenderDraw data = command.render_draw;
					wgpuRenderPassEncoderDraw(render_encoder, data.vertex_count, data.instance_count, data.first_vertex, data.first_instance);
				} break;
				case PassEncoderCommand::CommandType::RENDER_DRAW_INDEXED: {
					PassEncoderCommand::RenderDrawIndexed data = command.render_draw_indexed;
					wgpuRenderPassEncoderDrawIndexed(render_encoder, data.index_count, data.instance_count, data.first_index, data.base_vertex, data.first_instance);
				} break;
				case PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDIRECT: {
#ifdef WEBGPU_BACKEND_DAWN_DESKTOP
					PassEncoderCommand::RenderMultiDrawIndirect data = command.render_multi_draw_indirect;
					wgpuRenderPassEncoderMultiDrawIndirect(render_encoder, data.indirect_buffer, data.indirect_offset, data.count, nullptr, 0);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
					PassEncoderCommand::RenderMultiDrawIndirect data = command.render_multi_draw_indirect;
					wgpuRenderPassEncoderMultiDrawIndirect(render_encoder, data.indirect_buffer, data.indirect_offset, data.count);
#else
					CRASH_NOW_MSG("wgpuRenderPassEncoderMultiDrawIndirect unsupported");
#endif
				} break;
				case PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDIRECT_COUNT: {
#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
					PassEncoderCommand::RenderMultiDrawIndirectCount data = command.render_multi_draw_indirect_count;
					wgpuRenderPassEncoderMultiDrawIndirectCount(render_encoder, data.indirect_buffer, data.indirect_offset, data.count_buffer, data.count_offset, data.max_count);
#else
					CRASH_NOW_MSG("wgpuRenderPassEncoderMultiDrawIndirectCount unsupported");
#endif
				} break;
				case PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDEXED_INDIRECT: {
#ifdef WEBGPU_BACKEND_DAWN_DESKTOP
					PassEncoderCommand::RenderMultiDrawIndexedIndirect data = command.render_multi_draw_indexed_indirect;
					wgpuRenderPassEncoderMultiDrawIndexedIndirect(render_encoder, data.indirect_buffer, data.indirect_offset, data.count, nullptr, 0);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
					PassEncoderCommand::RenderMultiDrawIndexedIndirect data = command.render_multi_draw_indexed_indirect;
					wgpuRenderPassEncoderMultiDrawIndexedIndirect(render_encoder, data.indirect_buffer, data.indirect_offset, data.count);
#else
					CRASH_NOW_MSG("wgpuRenderPassEncoderMultiDrawIndexedIndirect unsupported");
#endif
				} break;
				case PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDEXED_INDIRECT_COUNT: {
#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
					PassEncoderCommand::RenderMultiDrawIndexedIndirectCount data = command.render_multi_draw_indexed_indirect_count;
					wgpuRenderPassEncoderMultiDrawIndexedIndirectCount(render_encoder, data.indirect_buffer, data.indirect_offset, data.count_buffer, data.count_offset, data.max_count);
#else
					CRASH_NOW_MSG("wgpuRenderPassEncoderMultiDrawIndexedIndirectCount unsupported");
#endif
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_VERTEX_BUFFER: {
					PassEncoderCommand::RenderSetVertexBuffer data = command.render_set_vertex_buffer;
					wgpuRenderPassEncoderSetVertexBuffer(render_encoder, data.slot, data.buffer, data.offset, data.size);
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_INDEX_BUFFER: {
					PassEncoderCommand::RenderSetIndexBuffer data = command.render_set_index_buffer;
					wgpuRenderPassEncoderSetIndexBuffer(render_encoder, data.buffer, data.format, data.offset, data.size);
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_BLEND_CONSTANTS: {
					const PassEncoderCommand::RenderSetBlendConstant &data = command.render_set_blend_constant;
					wgpuRenderPassEncoderSetBlendConstant(render_encoder, &data.color);
				} break;
				case PassEncoderCommand::CommandType::RENDER_SET_IMMEDIATES: {
					pc_state.current_offset = command.render_set_immediates.emulation_offset;
				} break;
				default:
					break;
			}
		}

		wgpuRenderPassEncoderEnd(render_encoder);
		wgpuRenderPassEncoderRelease(render_encoder);
	}

	p_command_buffer_info.commands.clear();
}

RenderingDeviceDriver::CommandBufferID RenderingDeviceDriverWebGpu::command_buffer_create(CommandPoolID p_cmd_pool) {
	TightLocalVector<CommandBufferInfo *> *command_pool = (TightLocalVector<CommandBufferInfo *> *)p_cmd_pool.id;
	CommandBufferInfo *command_buffer_info = memnew(CommandBufferInfo);

	command_buffer_info->encoder = nullptr;
	command_buffer_info->is_render_pass_active = false;

	command_pool->push_back(command_buffer_info);

	return CommandBufferID(command_buffer_info);
}

bool RenderingDeviceDriverWebGpu::command_buffer_begin(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer.id != 0);

	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	WGPUCommandEncoderDescriptor desc = (WGPUCommandEncoderDescriptor){};
	command_buffer_info->encoder = wgpuDeviceCreateCommandEncoder(device, &desc);

	return true;
}

bool RenderingDeviceDriverWebGpu::command_buffer_begin_secondary(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> command_buffer_begin_secondary");
}

void RenderingDeviceDriverWebGpu::command_buffer_end(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer.id != 0);

	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;

	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	_flush_active_command_pass(*command_buffer_info);
}

void RenderingDeviceDriverWebGpu::command_buffer_execute_secondary(CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> command_buffer_execute_secondary");
}

/********************/
/**** SWAP CHAIN ****/
/********************/

RenderingDeviceDriver::SwapChainID RenderingDeviceDriverWebGpu::swap_chain_create(RenderingContextDriver::SurfaceID p_surface) {
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)p_surface;

	RenderPassInfo *render_pass_info = memnew(RenderPassInfo);
	render_pass_info->depth_attachment_index = UINT32_MAX;

	surface->configure(this->adapter, this->device);

	render_pass_info->attachments = Vector<RenderPassAttachmentInfo>({ (RenderPassAttachmentInfo){
			.format = surface->format,
			.sample_count = 1,
			.load_op = WGPULoadOp_Clear,
			.store_op = WGPUStoreOp_Store,
			.stencil_load_op = WGPULoadOp_Undefined,
			.stencil_store_op = WGPUStoreOp_Undefined,
	} });
	// TODO: The multiview feature is currently disabled, so I will ignore this.
	render_pass_info->view_count = 1;

	SwapChainInfo *swapchain_info = memnew(SwapChainInfo);
	swapchain_info->surface = p_surface;
	swapchain_info->render_pass = RenderPassID(render_pass_info);

	return SwapChainID(swapchain_info);
}

Error RenderingDeviceDriverWebGpu::swap_chain_resize(CommandQueueID _p_cmd_queue, SwapChainID p_swap_chain, uint32_t _p_desired_framebuffer_count) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;

	surface->configure(this->adapter, this->device);
	context_driver->surface_set_needs_resize(swapchain_info->surface, false);

	return OK;
}

RenderingDeviceDriver::FramebufferID RenderingDeviceDriverWebGpu::swap_chain_acquire_framebuffer(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool &r_resize_required) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	if (context_driver->surface_get_needs_resize(swapchain_info->surface)) {
		r_resize_required = true;
		return FramebufferID();
	}

	FramebufferInfo *framebuffer_info = memnew(FramebufferInfo);
	framebuffer_info->maybe_swapchain = p_swap_chain;

	return FramebufferID(framebuffer_info);
}

RenderingDeviceDriver::RenderPassID RenderingDeviceDriverWebGpu::swap_chain_get_render_pass(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	return swapchain_info->render_pass;
}

RenderingDeviceDriver::DataFormat RenderingDeviceDriverWebGpu::swap_chain_get_format(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;

	return surface->rd_format;
}

RenderingDeviceDriver::ColorSpace RenderingDeviceDriverWebGpu::swap_chain_get_color_space(SwapChainID p_swap_chain) {
	// TODO: Look into this.
	return ColorSpace::COLOR_SPACE_REC709_NONLINEAR_SRGB;
	;
}

bool RenderingDeviceDriverWebGpu::swap_chain_get_hdr_output_supported(SwapChainID p_swap_chain) {
	return false;
}

void RenderingDeviceDriverWebGpu::swap_chain_free(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;

	memdelete((RenderPassInfo *)swapchain_info->render_pass.id);
	memdelete(swapchain_info);
}

/*********************/
/**** FRAMEBUFFER ****/
/*********************/

RenderingDeviceDriver::FramebufferID RenderingDeviceDriverWebGpu::framebuffer_create(RenderPassID p_render_pass, VectorView<TextureID> p_attachments, uint32_t _p_width, uint32_t _p_height) {
	FramebufferInfo *framebuffer_info = memnew(FramebufferInfo);
	framebuffer_info->maybe_swapchain = SwapChainID();

	Vector<TextureID> attachments = Vector<TextureID>();
	for (uint32_t i = 0; i < p_attachments.size(); i++) {
		attachments.push_back(p_attachments[i]);
	}
	const RenderPassInfo *render_pass_info = (const RenderPassInfo *)p_render_pass.id;
	if (render_pass_info && render_pass_info->empty_attachment && attachments.is_empty()) {
		const RenderPassAttachmentInfo &attachment = render_pass_info->attachments[0];
		TextureFormat format = {};
		format.format = rd_texture_format_from_webgpu(attachment.format);
		format.width = MAX(_p_width, 1u);
		format.height = MAX(_p_height, 1u);
		format.usage_bits = TextureUsageBits::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		format.samples = TEXTURE_SAMPLES_4; // Matches _empty_render_pass_attachment_create().
		TextureView view = {};
		view.format = format.format;
		attachments.push_back(texture_create(format, view));
		framebuffer_info->owned_attachment = attachments[0];
	}
	framebuffer_info->attachments = attachments;
	// The render area is the attachments' size, which can be smaller than the size asked for.
	if (!attachments.is_empty()) {
		const TextureInfo *texture_info = (const TextureInfo *)attachments[0].id;
		const uint32_t mip = texture_info->texture_view_desc.baseMipLevel;
		framebuffer_info->size = Size2i(MAX(texture_info->texture_desc.size.width >> mip, 1u), MAX(texture_info->texture_desc.size.height >> mip, 1u));
	}

	return FramebufferID(framebuffer_info);
}

void RenderingDeviceDriverWebGpu::framebuffer_free(FramebufferID p_framebuffer) {
	FramebufferInfo *framebuffer_info = (FramebufferInfo *)p_framebuffer.id;
	if (framebuffer_info->owned_attachment) {
		texture_free(framebuffer_info->owned_attachment);
	}
	memdelete(framebuffer_info);
}

/****************/
/**** SHADER ****/
/****************/

RenderingDeviceDriver::ShaderID RenderingDeviceDriverWebGpu::shader_create_from_container(const Ref<RenderingShaderContainer> &p_shader_container, const Vector<ImmutableSampler> &p_immutable_samplers) {
	WEBGPU_ON_MAIN_THREAD(ShaderID, shader_create_from_container(p_shader_container, p_immutable_samplers));
	Ref<RenderingShaderContainerWebGpu> container = p_shader_container;
	ERR_FAIL_COND_V(container.is_null(), ShaderID());

	ShaderReflection refl = container->get_shader_reflection();

	ShaderInfo *shader_info = memnew(ShaderInfo);
	*shader_info = {};

	shader_info->shader_name = String::utf8(container->shader_name.ptr(), container->shader_name.length());

	Vector<Vector<WGPUBindGroupLayoutEntry>> &bind_group_layout_entries = shader_info->bind_group_layout_entries;
	bind_group_layout_entries.resize_initialized(refl.uniform_sets.size());

	// Used to index `webgpu_uniform_data` which is a flattened
	uint32_t global_idx = 0;
	for (uint32_t set_idx = 0; set_idx < refl.uniform_sets.size(); set_idx++) {
		const Vector<ShaderUniform> &uniforms = refl.uniform_sets[set_idx];

		// Build corrections and other useful information
		HashMap<uint32_t, Vector<uint32_t>> binding_remaps;
		uint32_t saved_global_idx = global_idx;
		for (uint32_t binding_idx = 0; binding_idx < (uint32_t)uniforms.size(); binding_idx++) {
			const ShaderUniform &refl_uniform = uniforms[binding_idx];
			const RenderingShaderContainerWebGpu::UniformData &u = container->webgpu_uniform_data[global_idx++];
			binding_remaps.insert(refl_uniform.binding, u.corrections);
		}
		global_idx = saved_global_idx;

		// Build binding hints
		HashMap<uint32_t, WebGpuBindingHint> binding_hints;
		for (uint32_t binding_idx = 0; binding_idx < (uint32_t)uniforms.size(); binding_idx++) {
			const ShaderUniform &refl_uniform = uniforms[binding_idx];
			const RenderingShaderContainerWebGpu::UniformData &u = container->webgpu_uniform_data[global_idx++];
			for (int k = 0; k < u.corrections.size(); k++) {
				binding_hints.insert(refl_uniform.binding + 1 + k, u.binding_hints[k]);
			}
		}
		global_idx = saved_global_idx;

		// Run unified correction index splitting logic.
		Vector<Pair<uint32_t, UniformType>> index_bindings_input;
		for (uint32_t uniform_idx = 0; uniform_idx < uniforms.size(); uniform_idx++) {
			const ShaderUniform &uniform = uniforms.ptr()[uniform_idx];
			index_bindings_input.push_back({ uniform.binding, uniform.type });
		}
		// We do not supply `p_binding_mask` because we prune bind group layout entries right after.
		Vector<RemappedBinding> corrected_bindings = _remap_binding_indices(index_bindings_input, binding_remaps, nullptr, BindingIndexType::CORRECTED);

		// Build bind group layouts
		HashMap<uint32_t, UniformType> used_original_bindings_map;

		// Keep track of texture / sampler index pairs
		bool is_texture_turn = true;

		for (uint32_t corrected_binding_idx = 0; corrected_binding_idx < (uint32_t)corrected_bindings.size(); corrected_binding_idx++) {
			const RemappedBinding &corrected_binding = corrected_bindings[corrected_binding_idx];

			const ShaderUniform &refl_uniform = uniforms[corrected_binding.input_idx];
			const RenderingShaderContainerWebGpu::UniformData &u = container->webgpu_uniform_data[saved_global_idx + corrected_binding.input_idx];

			ShaderUniform info = refl_uniform;
			info.image_format = (DataFormat)u.image_format;
			info.image_access = (ShaderUniform::ImageAccess)u.image_access;
			info.texture_image_type = (TextureType)u.texture_image_type;
			info.texture_sample_type = (ShaderUniform::TextureSampleType)u.texture_sample_type;
			info.texture_is_multisample = u.texture_is_multisample != 0;

			binding_hints.insert(corrected_binding.corrected_binding_idx, u.base_hint);

			// Visible only to the stages whose code reads the uniform: WebGPU counts each binding
			// against the per-stage limits of every stage it is visible to, and Godot declares many
			// bindings in stages that never read them. Uniform sets bound with other variants get a
			// bind group per layout (_select_bind_group_from_uniform_set).
			WGPUShaderStage shader_stage = (WGPUShaderStage)0;
			for (uint32_t k = 0; k < SHADER_STAGE_MAX; k++) {
				if (info.stages.has_flag((ShaderStage)(1 << k)) && (u.stages_read & (1u << k))) {
					shader_stage |= webgpu_shader_stage_from_rd((ShaderStage)k);
				}
			}

			WGPUBindGroupLayoutEntry layout_entry = {};
			layout_entry.binding = corrected_binding.corrected_binding_idx;
			layout_entry.visibility = shader_stage;

			// Used to build `used_original_bindings`.
			int set_original_entry_count = bind_group_layout_entries[set_idx].size();

			switch (corrected_binding.original_type) {
				case UNIFORM_TYPE_SAMPLER: {
					bool pruned = u.base_hint.type == WebGpuBindingHintType::UNUSED;

					WGPUSamplerBindingType sampler_binding_type =
							!pruned ? (WGPUSamplerBindingType)u.base_hint.sampler.sampler_type : WGPUSamplerBindingType_Filtering;
					layout_entry.sampler = WGPU_SAMPLER_BINDING_LAYOUT_INIT;
					layout_entry.sampler.type = sampler_binding_type;
					if (corrected_binding.remap.second) {
						switch (corrected_binding.remap.first) {
							case WEBGPU_BINDING_REMAP_SPLIT_DREF_REGULAR:
								layout_entry.sampler.type = WGPUSamplerBindingType_Filtering;
								break;
							case WEBGPU_BINDING_REMAP_SPLIT_DREF_COMPARISON:
								layout_entry.sampler.type = WGPUSamplerBindingType_Comparison;
								break;
							case WEBGPU_BINDING_REMAP_SPLIT_COMBINED:
								memdelete(shader_info);
								ERR_FAIL_V_MSG(ShaderID(), "Expected sampler, got combined image sampler");
							default:
								break;
						}
					}
					if (!pruned) {
						bind_group_layout_entries.write[set_idx].push_back(layout_entry);
					}
				} break;
				case UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
					bool pruned = u.base_hint.type == WebGpuBindingHintType::UNUSED;
					if (is_texture_turn) {
						WGPUTextureSampleType sampleType =
								!pruned ? (WGPUTextureSampleType)u.base_hint.texture.sample_type : webgpu_texture_sample_type_from_shader_uniform(info.texture_sample_type);
						bool multisampled = !pruned ? u.base_hint.texture.multisampled != 0 : info.texture_is_multisample;
						if (info.texture_is_multisample && sampleType == WGPUTextureSampleType_Float) {
							sampleType = WGPUTextureSampleType_UnfilterableFloat;
						}
						layout_entry.texture = WGPU_TEXTURE_BINDING_LAYOUT_INIT;
						layout_entry.texture.sampleType = sampleType;
						layout_entry.texture.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type);
						layout_entry.texture.multisampled = multisampled;
						if (!pruned) {
							bind_group_layout_entries.write[set_idx].push_back(layout_entry);
						}
					} else {
						layout_entry.texture.sampleType = WGPUTextureSampleType_BindingNotUsed;
						const bool sampler_hint = u.binding_hints.size() == 1 && u.binding_hints[0].type == WebGpuBindingHintType::SAMPLER;
						layout_entry.sampler = WGPU_SAMPLER_BINDING_LAYOUT_INIT;
						layout_entry.sampler.type = sampler_hint ? (WGPUSamplerBindingType)u.binding_hints[0].sampler.sampler_type : WGPUSamplerBindingType_Filtering;
						if (!pruned) {
							bind_group_layout_entries.write[set_idx].push_back(layout_entry);
						}
					}
					is_texture_turn = !is_texture_turn;
				} break;
				case UNIFORM_TYPE_TEXTURE: {
					bool pruned = u.base_hint.type == WebGpuBindingHintType::UNUSED;
					WGPUTextureSampleType sampleType =
							!pruned ? (WGPUTextureSampleType)u.base_hint.texture.sample_type : webgpu_texture_sample_type_from_shader_uniform(info.texture_sample_type);

					bool multisampled = !pruned ? u.base_hint.texture.multisampled != 0 : info.texture_is_multisample;
					if (info.texture_is_multisample && sampleType == WGPUTextureSampleType_Float) {
						sampleType = WGPUTextureSampleType_UnfilterableFloat;
					}

					// Dref textures are always depth or unfilterable-float rather than float.
					// TODO: We can try to memoize these.
					bool is_dref_split = false;
					for (int i = 0; i < u.corrections.size(); i++) {
						if (u.corrections[i] == WEBGPU_BINDING_REMAP_SPLIT_DREF_REGULAR ||
								u.corrections[i] == WEBGPU_BINDING_REMAP_SPLIT_DREF_COMPARISON) {
							is_dref_split = true;
							break;
						}
					}
					if (is_dref_split && sampleType != WGPUTextureSampleType_Float) {
						sampleType = WGPUTextureSampleType_UnfilterableFloat;
					}

					layout_entry.texture = WGPU_TEXTURE_BINDING_LAYOUT_INIT;
					layout_entry.texture.sampleType = sampleType;
					layout_entry.texture.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type);
					layout_entry.texture.multisampled = multisampled;

					if (corrected_binding.remap.second) {
						switch (corrected_binding.remap.first) {
							case WEBGPU_BINDING_REMAP_SPLIT_DREF_REGULAR:
								layout_entry.texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
								break;
							case WEBGPU_BINDING_REMAP_SPLIT_DREF_COMPARISON:
								layout_entry.texture.sampleType = WGPUTextureSampleType_Depth;
								break;
							case WEBGPU_BINDING_REMAP_SPLIT_COMBINED:
								memdelete(shader_info);
								ERR_FAIL_V_MSG(ShaderID(), "Expected texture, got combined image sampler");
							default:
								break;
						}
					}
					if (!pruned) {
						bind_group_layout_entries.write[set_idx].push_back(layout_entry);
					}
				} break;
				case UNIFORM_TYPE_IMAGE: {
					if (u.base_hint.type == WebGpuBindingHintType::TEXTURE) {
						// A read-only image that the shader reads as a sampled texture.
						WGPUTextureViewDimension view_dimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type);
						if (view_dimension == WGPUTextureViewDimension_Cube) {
							view_dimension = WGPUTextureViewDimension_2DArray;
						}
						layout_entry.texture = WGPU_TEXTURE_BINDING_LAYOUT_INIT;
						layout_entry.texture.sampleType = (WGPUTextureSampleType)u.base_hint.texture.sample_type;
						layout_entry.texture.viewDimension = view_dimension;
						bind_group_layout_entries.write[set_idx].push_back(layout_entry);
						break;
					}
					WGPUStorageTextureAccess access;
					switch (info.image_access) {
						case ShaderUniform::ImageAccess::ReadWrite:
							access = WGPUStorageTextureAccess_ReadWrite;
							break;
						case ShaderUniform::ImageAccess::ReadOnly:
							access = WGPUStorageTextureAccess_ReadOnly;
							break;
						case ShaderUniform::ImageAccess::WriteOnly:
							access = WGPUStorageTextureAccess_WriteOnly;
							break;
					}

					// HACK: Replace cube storage texture to bypass a wgpu validation error.
					WGPUTextureViewDimension viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type);
					if (viewDimension == WGPUTextureViewDimension_Cube) {
						viewDimension = WGPUTextureViewDimension_2DArray;
					} else if (viewDimension == WGPUTextureViewDimension_CubeArray) {
						memdelete(shader_info);
						ERR_FAIL_V_MSG(ShaderID(), "WebGpu storage cube arrays are not supported.");
					}

					layout_entry.storageTexture = WGPU_STORAGE_TEXTURE_BINDING_LAYOUT_INIT;
					layout_entry.storageTexture.access = access;
					layout_entry.storageTexture.format = webgpu_texture_format_from_rd(info.image_format);
					layout_entry.storageTexture.viewDimension = viewDimension;

					bind_group_layout_entries.write[set_idx].push_back(layout_entry);
				} break;
				case UNIFORM_TYPE_INPUT_ATTACHMENT: {
					const WebGpuBindingHint &input_hint = u.base_hint;
					layout_entry.texture = WGPU_TEXTURE_BINDING_LAYOUT_INIT;
					layout_entry.texture.sampleType = (WGPUTextureSampleType)input_hint.texture.sample_type;
					layout_entry.texture.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type);
					layout_entry.texture.multisampled = input_hint.texture.multisampled != 0;
					bind_group_layout_entries.write[set_idx].push_back(layout_entry);
				} break;
				case UNIFORM_TYPE_UNIFORM_BUFFER: {
					layout_entry.buffer = WGPU_BUFFER_BINDING_LAYOUT_INIT;
					layout_entry.buffer.type = WGPUBufferBindingType_Uniform;
					layout_entry.buffer.hasDynamicOffset = false;
					layout_entry.buffer.minBindingSize = info.length;
					bind_group_layout_entries.write[set_idx].push_back(layout_entry);
				} break;
				case UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC: {
					layout_entry.buffer = WGPU_BUFFER_BINDING_LAYOUT_INIT;
					layout_entry.buffer.type = WGPUBufferBindingType_Uniform;
					layout_entry.buffer.hasDynamicOffset = true;
					layout_entry.buffer.minBindingSize = info.length;
					bind_group_layout_entries.write[set_idx].push_back(layout_entry);
				} break;
				case UNIFORM_TYPE_STORAGE_BUFFER: {
					layout_entry.buffer = WGPU_BUFFER_BINDING_LAYOUT_INIT;
					layout_entry.buffer.type = info.writable ? WGPUBufferBindingType_Storage : WGPUBufferBindingType_ReadOnlyStorage;
					layout_entry.buffer.hasDynamicOffset = false;
					layout_entry.buffer.minBindingSize = info.length;
					bind_group_layout_entries.write[set_idx].push_back(layout_entry);
				} break;
				case UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC: {
					layout_entry.buffer = WGPU_BUFFER_BINDING_LAYOUT_INIT;
					layout_entry.buffer.type = info.writable ? WGPUBufferBindingType_Storage : WGPUBufferBindingType_ReadOnlyStorage;
					layout_entry.buffer.hasDynamicOffset = true;
					layout_entry.buffer.minBindingSize = info.length;
					bind_group_layout_entries.write[set_idx].push_back(layout_entry);
				} break;
				case UNIFORM_TYPE_TEXTURE_BUFFER:
				case UNIFORM_TYPE_IMAGE_BUFFER:
					memdelete(shader_info);
					ERR_FAIL_V_MSG(ShaderID(), "WebGpu UNIFORM_TYPE_TEXTURE_BUFFER and UNIFORM_TYPE_IMAGE_BUFFER not supported.");
				default: {
					memdelete(shader_info);
					DEV_ASSERT(false);
					return ShaderID();
				}
			}

			if (bind_group_layout_entries[set_idx].size() > set_original_entry_count) {
				used_original_bindings_map.insert(info.binding, info.type);
			}
		}

		global_idx = saved_global_idx + (uint32_t)uniforms.size();

		shader_info->set_binding_remaps.insert(set_idx, binding_remaps);
		shader_info->set_binding_hints.insert(set_idx, binding_hints);

		HashSet<uint32_t> used_bindings;
		for (int i = 0; i < bind_group_layout_entries[set_idx].size(); i++) {
			used_bindings.insert(bind_group_layout_entries[set_idx][i].binding);
		}
		shader_info->used_set_bindings.insert(set_idx, used_bindings);
		shader_info->used_original_bindings_map.insert(set_idx, used_original_bindings_map);
	}

	for (uint32_t i = 0; i < refl.specialization_constants.size(); i++) {
		const ShaderSpecializationConstant &sc = refl.specialization_constants[i];
		CharString key = uitos(sc.constant_id).ascii();
		if (sc.stages.has_flag(SHADER_STAGE_VERTEX_BIT)) {
			shader_info->vertex_override_layout.insert(sc.constant_id, key);
		}
		if (sc.stages.has_flag(SHADER_STAGE_FRAGMENT_BIT)) {
			shader_info->fragment_override_layout.insert(sc.constant_id, key);
		}
		if (sc.stages.has_flag(SHADER_STAGE_COMPUTE_BIT)) {
			shader_info->compute_override_layout.insert(sc.constant_id, key);
		}
	}

	Vector<uint8_t> decompressed_code;
	for (int64_t i = 0; i < container->shaders.size(); i++) {
		const RenderingShaderContainer::Shader &shader = container->shaders[i];

		uint32_t source_size = shader.code_decompressed_size;
		ERR_FAIL_COND_V_MSG(source_size == 0, ShaderID(), "Empty WGSL source in shader container.");
		decompressed_code.resize(source_size);
		bool ok = container->decompress_code(
				shader.code_compressed_bytes.ptr(), shader.code_compressed_bytes.size(),
				shader.code_compression_flags, decompressed_code.ptrw(), source_size);
		ERR_FAIL_COND_V_MSG(!ok, ShaderID(), vformat("Failed to decompress WGSL on shader stage %s.", String(SHADER_STAGE_NAMES[shader.shader_stage])));

		WGPUShaderSourceWGSL source = WGPU_SHADER_SOURCE_WGSL_INIT;
		source.chain = {};
		source.chain.next = nullptr;
		source.chain.sType = WGPUSType_ShaderSourceWGSL;
		source.code = WGPU_STRING_VIEW_INIT;
		source.code.data = (const char *)decompressed_code.ptr();
		source.code.length = source_size;

		shader_info->shader_contents.push_back(String::utf8((const char *)decompressed_code.ptr(), source_size));

		WGPUShaderModuleDescriptor shader_module_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
		shader_module_desc.nextInChain = &source.chain;
		WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_module_desc);
		ERR_FAIL_COND_V(!shader_module, ShaderID());

		switch (shader.shader_stage) {
			case SHADER_STAGE_VERTEX:
				ERR_FAIL_COND_V_MSG(shader_info->vertex_shader, ShaderID(), "More than one vertex stage in one shader.");
				shader_info->vertex_shader = shader_module;
				shader_info->stage_flags = (WGPUShaderStage)(shader_info->stage_flags | WGPUShaderStage_Vertex);
				break;
			case SHADER_STAGE_FRAGMENT:
				ERR_FAIL_COND_V_MSG(shader_info->fragment_shader, ShaderID(), "More than one fragment stage in one shader.");
				shader_info->fragment_shader = shader_module;
				shader_info->stage_flags = (WGPUShaderStage)(shader_info->stage_flags | WGPUShaderStage_Fragment);
				break;
			case SHADER_STAGE_COMPUTE:
				ERR_FAIL_COND_V_MSG(shader_info->compute_shader, ShaderID(), "More than one compute stage in one shader.");
				shader_info->compute_shader = shader_module;
				shader_info->stage_flags = (WGPUShaderStage)(shader_info->stage_flags | WGPUShaderStage_Compute);
				break;
			default:
				memdelete(shader_info);
				ERR_FAIL_V_MSG(ShaderID(), vformat("WebGpu shader stage %d not supported", shader.shader_stage));
		}
	}

	WGPUShaderStage push_constant_stages = (WGPUShaderStage)0;
	bool lone_push_constant_group = false;
	for (uint32_t k = 0; k < SHADER_STAGE_MAX; k++) {
		if (refl.push_constant_stages.has_flag((ShaderStage)(1 << k))) {
			push_constant_stages |= webgpu_shader_stage_from_rd((ShaderStage)k);
		}
	}
	WGPUBindGroupLayoutEntry push_constant_entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
	push_constant_entry.nextInChain = nullptr;
	push_constant_entry.binding = WEBGPU_PUSH_CONSTANT_EMULATION_BINDING;
	push_constant_entry.visibility = push_constant_stages;
	push_constant_entry.buffer = WGPU_BUFFER_BINDING_LAYOUT_INIT;
	push_constant_entry.buffer.type = WGPUBufferBindingType_Uniform;
	push_constant_entry.buffer.hasDynamicOffset = true;
	push_constant_entry.buffer.minBindingSize = 0;
	shader_info->push_constant_stage_flags = push_constant_stages;
	shader_info->push_constant_size = refl.push_constant_size;
	if (shader_info->push_constant_size > 0) {
		ERR_FAIL_COND_V_MSG(bind_group_layout_entries.size() > WEBGPU_MAX_BIND_GROUPS, ShaderID(),
				vformat("Shader %s uses too many uniform sets: %d / %d.",
						shader_info->shader_name, bind_group_layout_entries.size(), WEBGPU_MAX_BIND_GROUPS));

		// Same rule as the SPIR-V to WGSL step uses for the push constant binding point.
		const uint32_t immediates_set = webgpu_immediates_set((uint32_t)bind_group_layout_entries.size());
		shader_info->push_constant_set_index = (int32_t)immediates_set;

		if (immediates_set < (uint32_t)bind_group_layout_entries.size()) {
			Vector<WGPUBindGroupLayoutEntry> &host_group = bind_group_layout_entries.write[immediates_set];
			for (int i = 0; i < host_group.size(); i++) {
				ERR_FAIL_COND_V_MSG(host_group[i].binding == WEBGPU_PUSH_CONSTANT_EMULATION_BINDING, ShaderID(),
						vformat("Shader %s uses the reserved push constant binding %d in set %d.",
								shader_info->shader_name, WEBGPU_PUSH_CONSTANT_EMULATION_BINDING, immediates_set));
			}
			host_group.push_back(push_constant_entry);
		} else {
			lone_push_constant_group = true;
		}
	}

	for (uint32_t set_idx = 0; set_idx < (uint32_t)bind_group_layout_entries.size(); set_idx++) {
		WGPUBindGroupLayoutDescriptor bind_group_layout_desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
		bind_group_layout_desc.entryCount = (size_t)bind_group_layout_entries[set_idx].size();
		bind_group_layout_desc.entries = bind_group_layout_entries[set_idx].ptr();

		WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &bind_group_layout_desc);
		ERR_FAIL_COND_V(!bind_group_layout, ShaderID());
		for (const WGPUBindGroupLayoutEntry &e : bind_group_layout_entries[set_idx]) {
			if (e.sampler.type == WGPUSamplerBindingType_NonFiltering) {
				non_filtering_sampler_bindings[bind_group_layout].insert(e.binding);
			}
		}

		shader_info->bind_group_layouts.push_back(bind_group_layout);
		shader_info->bind_group_layout_descs.push_back(bind_group_layout_desc);
	}

	for (uint32_t set_idx = 0; set_idx < (uint32_t)bind_group_layout_entries.size(); set_idx++) {
		uint32_t dynamic_count = 0;
		for (int i = 0; i < bind_group_layout_entries[set_idx].size(); i++) {
			const WGPUBindGroupLayoutEntry &e = bind_group_layout_entries[set_idx][i];
			if (e.buffer.hasDynamicOffset && e.binding != WEBGPU_PUSH_CONSTANT_EMULATION_BINDING) {
				dynamic_count++;
			}
		}
		shader_info->set_dynamic_offset_counts.push_back(dynamic_count);
	}

	if (lone_push_constant_group) {
		WGPUBindGroupLayoutEntry layout_entry = push_constant_entry;
		WGPUBindGroupLayoutDescriptor bind_group_layout_desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
		bind_group_layout_desc.nextInChain = nullptr;
		bind_group_layout_desc.entryCount = 1;
		bind_group_layout_desc.entries = &layout_entry;
		WGPUBindGroupLayout push_constant_layout = wgpuDeviceCreateBindGroupLayout(device, &bind_group_layout_desc);
		shader_info->standalone_push_constant_layout = push_constant_layout;
		shader_info->bind_group_layouts.push_back(push_constant_layout);

		WGPUBindGroupEntry entry = {};
		entry.binding = WEBGPU_PUSH_CONSTANT_EMULATION_BINDING;
		entry.buffer = push_constant_emulation_buffer;
		entry.offset = 0;
		entry.size = WEBGPU_MAX_IMMEDIATE_SIZE;

		WGPUBindGroupDescriptor bind_group_desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
		bind_group_desc.layout = push_constant_layout;
		bind_group_desc.entryCount = 1;
		bind_group_desc.entries = &entry;
		WGPUBindGroup push_constant_group = wgpuDeviceCreateBindGroup(device, &bind_group_desc);
		shader_info->standalone_push_constant_group = push_constant_group;
	}

	// Superseded by immediates patching.
	// Though maybe this will come back in the future.
	//
	// WGPUPipelineLayoutExtras wgpu_pipeline_extras = (WGPUPipelineLayoutExtras){
	// 	.chain = (WGPUChainedStruct){
	// 			.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras,
	// 	},
	// 	.immediateDataSize = refl.push_constant_size,
	// };

	WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
	pipeline_layout_descriptor.bindGroupLayoutCount = (size_t)shader_info->bind_group_layouts.size();
	pipeline_layout_descriptor.bindGroupLayouts = shader_info->bind_group_layouts.ptr();

	shader_info->pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &pipeline_layout_descriptor);

	ERR_FAIL_COND_V(!shader_info->pipeline_layout, ShaderID());

	return ShaderID(shader_info);
}

void RenderingDeviceDriverWebGpu::shader_free(ShaderID p_shader) {
	// TODO: impl
}

void RenderingDeviceDriverWebGpu::shader_destroy_modules(ShaderID p_shader) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> shader_destroy_modules");
}

/*********************/
/**** UNIFORM SET ****/
/*********************/

Vector<RenderingDeviceDriverWebGpu::RemappedBinding> RenderingDeviceDriverWebGpu::_remap_binding_indices(const Vector<Pair<uint32_t, UniformType>> &p_bindings, const HashMap<uint32_t, Vector<uint32_t>> &p_set_binding_remaps, const HashSet<uint32_t> *p_binding_mask, const BindingIndexType p_mask_type) {
	Vector<RemappedBinding> corrected_bindings;

	uint32_t binding_offset = 0;

	// It is important we check for corrections that fall before our first binding.
	if (p_bindings.size() > 0) {
		uint32_t first_binding = p_bindings[0].first;
		for (uint32_t b = 0; b < first_binding; b++) {
			if (p_set_binding_remaps.has(b)) {
				// This makes the assumption that binding array of combined image samplers don't exist.
				binding_offset += p_set_binding_remaps[b].size();
			}
		}
	}

	for (uint32_t uniform_idx = 0; uniform_idx < p_bindings.size(); uniform_idx++) {
		uint32_t binding_idx = p_bindings[uniform_idx].first;
		UniformType binding_type = p_bindings[uniform_idx].second;

		int binding_correction_count = (p_set_binding_remaps.has(binding_idx) ? (int)p_set_binding_remaps[binding_idx].size() : 0);
		uint32_t array_index = 0;

		switch (binding_type) {
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER:
			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE:
			case RenderingDeviceCommons::UNIFORM_TYPE_INPUT_ATTACHMENT:
			case RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_STORAGE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case RenderingDeviceCommons::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC: {
				for (int i = -1; i < binding_correction_count; i++) {
					uint32_t correction = i != -1 ? p_set_binding_remaps[binding_idx][i] : -1;
					if (i >= 0) {
						if (correction == WEBGPU_BINDING_REMAP_SPLIT_BINDING_ARRAY) {
							array_index += 1;
						}
						binding_offset += 1;
					}

					corrected_bindings.push_back((RemappedBinding){
							.input_idx = uniform_idx,
							.corrected_binding_idx = binding_idx + binding_offset,
							.original_array_idx = uniform_idx,
							.binding_id_idx = array_index,
							.original_type = binding_type,
							.remap = { (BindingRemapKind)correction, i != -1 },
					});
				}
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
				for (int i = -1; i < binding_correction_count; i++) {
					uint32_t correction = i != -1 ? p_set_binding_remaps[binding_idx][i] : -1;
					if (i >= 0) {
						if (correction == WEBGPU_BINDING_REMAP_SPLIT_BINDING_ARRAY) {
							array_index += 1;
							binding_offset += 1;
						}
						if (correction == WEBGPU_BINDING_REMAP_SPLIT_COMBINED) {
							continue;
						}
					}

					// This is the texture.
					corrected_bindings.push_back((RemappedBinding){
							.input_idx = uniform_idx,
							.corrected_binding_idx = binding_idx + binding_offset,
							.original_array_idx = uniform_idx,
							// Godot provides uniform ids in a (sampler, texture) pair.
							.binding_id_idx = array_index * 2 + 1,
							.original_type = binding_type,
							.remap = { WEBGPU_BINDING_REMAP_SPLIT_COMBINED, i != -1 },
					});

					binding_offset += 1;

					// This is the sampler.
					corrected_bindings.push_back((RemappedBinding){
							.input_idx = uniform_idx,
							.corrected_binding_idx = binding_idx + binding_offset,
							.original_array_idx = uniform_idx,
							// Godot provides uniform ids in (sampler, texture) pair.
							.binding_id_idx = array_index * 2 + 0,
							.original_type = binding_type,
							.remap = { WEBGPU_BINDING_REMAP_SPLIT_COMBINED, i != -1 },
					});
				}
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE_BUFFER:
				CRASH_NOW_MSG("Unimplemented!"); // TODO.
				break;
			case RenderingDeviceCommons::UNIFORM_TYPE_ACCELERATION_STRUCTURE:
				CRASH_NOW_MSG("WebRTX!!!");
				break;
			case RenderingDeviceCommons::UNIFORM_TYPE_MAX:
				break;
		}

		// It is important we check for corrections between bindings that we previously left out.
		if (uniform_idx + 1 < p_bindings.size()) {
			uint32_t next_binding_idx = p_bindings[uniform_idx + 1].first;
			for (uint32_t b = binding_idx + 1; b < next_binding_idx; b++) {
				if (p_set_binding_remaps.has(b)) {
					// This makes the assumption that binding array of combined image samplers don't exist.
					binding_offset += p_set_binding_remaps[b].size();
				}
			}
		}
	}

	if (p_binding_mask) {
		uint32_t offset = 0;
		uint32_t size = corrected_bindings.size();
		for (uint32_t i = 0; i < size; i++) {
			uint32_t binding = -1;
			switch (p_mask_type) {
				case BindingIndexType::ORIGINAL:
					binding = p_bindings[corrected_bindings[i - offset].input_idx].first;
					break;
				case BindingIndexType::CORRECTED:
					binding = corrected_bindings[i - offset].corrected_binding_idx;
					break;
				default:
					break;
			}

			if (!p_binding_mask->has(binding)) {
				corrected_bindings.remove_at(i - offset);
				offset += 1;
			}
		}
	}
	return corrected_bindings;
}

WGPUBindGroup RenderingDeviceDriverWebGpu::_bind_group_create(const VectorView<BoundUniform> &p_uniforms, WGPUBindGroupLayout p_layout, const HashMap<uint32_t, Vector<uint32_t>> &p_set_binding_remaps, const HashSet<RemappedBindingIndex> *p_binding_mask, uint32_t p_push_constant_size) {
	Vector<WGPUBindGroupEntry> entries;

	Vector<Pair<uint32_t, UniformType>> index_bindings_input;
	for (uint32_t uniform_idx = 0; uniform_idx < p_uniforms.size(); uniform_idx++) {
		const BoundUniform &uniform = p_uniforms.ptr()[uniform_idx];
		index_bindings_input.push_back({ uniform.binding, uniform.type });
	}
	Vector<RemappedBinding> corrected_bindings = _remap_binding_indices(index_bindings_input, p_set_binding_remaps, p_binding_mask, BindingIndexType::CORRECTED);

	for (uint32_t corrected_bindings_idx = 0; corrected_bindings_idx < corrected_bindings.size(); corrected_bindings_idx++) {
		const RemappedBinding corrected_binding = corrected_bindings[corrected_bindings_idx];
		const BoundUniform &uniform = p_uniforms.ptr()[corrected_binding.original_array_idx];

		switch (corrected_binding.original_type) {
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER: {
				WGPUBindGroupEntry entry = {};
				entry.binding = corrected_binding.corrected_binding_idx;
				entry.sampler = (WGPUSampler)uniform.ids[corrected_binding.binding_id_idx].id;
				entries.push_back(entry);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
				// Skip the sampler coming next, we can handle the next entry ourselves.
				corrected_bindings_idx += 1;

				WGPUBindGroupEntry texture_entry = {};
				texture_entry.binding = corrected_binding.corrected_binding_idx;

				TextureInfo *texture_info = (TextureInfo *)uniform.ids[corrected_binding.binding_id_idx].id;
				texture_entry.textureView = texture_info->get_view_with_format();
				entries.push_back(texture_entry);

				WGPUBindGroupEntry sampler_entry = {};
				sampler_entry.binding = corrected_binding.corrected_binding_idx + 1;
				// Godot provides uniform ids in (sampler, texture) pair.
				sampler_entry.sampler = (WGPUSampler)uniform.ids[corrected_binding.binding_id_idx - 1].id;
				entries.push_back(sampler_entry);
			} break;

			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE:
			case RenderingDeviceCommons::UNIFORM_TYPE_INPUT_ATTACHMENT: {
				ERR_FAIL_COND_V_MSG(
						uniform.type != RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE && uniform.type != RenderingDeviceCommons::UNIFORM_TYPE_IMAGE && corrected_binding.remap.second,
						nullptr,
						"UNIFORM_TYPE_INPUT_ATTACHMENT should not have corrections");
				WGPUBindGroupEntry entry = {};
				entry.binding = corrected_binding.corrected_binding_idx;

				TextureInfo *texture_info = (TextureInfo *)uniform.ids[corrected_binding.binding_id_idx].id;
				entry.textureView = texture_info->get_view_with_format();
				const WGPUTextureViewDimension dimension = texture_info->texture_view_desc.dimension;
				if (corrected_binding.original_type == RenderingDeviceCommons::UNIFORM_TYPE_IMAGE &&
						(dimension == WGPUTextureViewDimension_Cube || dimension == WGPUTextureViewDimension_CubeArray)) {
					if (!texture_info->storage_view) {
						WGPUTextureViewDescriptor desc = texture_info->texture_view_desc;
						desc.nextInChain = nullptr;
						desc.dimension = WGPUTextureViewDimension_2DArray;
						texture_info->storage_view = wgpuTextureCreateView(texture_info->texture, &desc);
					}
					entry.textureView = texture_info->storage_view;
				}
				entries.push_back(entry);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE_BUFFER:
				CRASH_NOW_MSG("Unimplemented!"); // TODO.
				break;
			case RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_STORAGE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case RenderingDeviceCommons::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC: {
				WGPUBindGroupEntry entry = {};
				entry.binding = corrected_binding.corrected_binding_idx;

				BufferInfo *buffer_info = (BufferInfo *)uniform.ids[corrected_binding.binding_id_idx].id;
				entry.buffer = buffer_info->buffer;
				// For dynamic bindings the per-slice byte offset is applied via the
				// dynamicOffsets array on wgpuRenderPassEncoderSetBindGroup, so the
				// entry itself stays at offset 0 with the slice size.
				entry.offset = 0;
				entry.size = buffer_info->size;

				entries.push_back(entry);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_ACCELERATION_STRUCTURE:
				CRASH_NOW_MSG("WebRTX!!!");
				break;
			case RenderingDeviceCommons::UNIFORM_TYPE_MAX:
				break;
		}
	}

	// A sampler bound where the layout cannot filter (a depth read; see WEBGPU_DEPTH_READS in the
	// shader container) is replaced by its nearest copy.
	const HashSet<uint32_t> *non_filtering = non_filtering_sampler_bindings.getptr(p_layout);
	if (non_filtering) {
		for (WGPUBindGroupEntry &e : entries) {
			if (e.sampler && non_filtering->has(e.binding)) {
				e.sampler = _sampler_get_non_filtering(e.sampler);
			}
		}
	}

	if (p_push_constant_size > 0) {
		WGPUBindGroupEntry buffer_entry = WGPU_BIND_GROUP_ENTRY_INIT;
		buffer_entry.binding = WEBGPU_PUSH_CONSTANT_EMULATION_BINDING;
		buffer_entry.buffer = push_constant_emulation_buffer;
		buffer_entry.offset = 0;
		buffer_entry.size = WEBGPU_MAX_IMMEDIATE_SIZE;
		entries.push_back(buffer_entry);
	}

	WGPUBindGroupDescriptor bind_group_desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	bind_group_desc.layout = p_layout;
	bind_group_desc.entryCount = (size_t)entries.size();
	bind_group_desc.entries = entries.ptr();
	WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(device, &bind_group_desc);

	return bind_group;
}

LocalVector<RenderingDeviceDriverWebGpu::BoundUniform> RenderingDeviceDriverWebGpu::_prune_bind_group_uniforms(const VectorView<BoundUniform> &p_uniforms, uint32_t p_set_idx, const HashMap<uint32_t, HashSet<uint32_t>> &p_pruned_set_bindings) {
	LocalVector<BoundUniform> new_uniforms;

	for (uint32_t idx = 0; idx < p_uniforms.size(); idx++) {
		const BoundUniform &uniform = p_uniforms.ptr()[idx];

		if (p_pruned_set_bindings[p_set_idx].has(uniform.binding)) {
			new_uniforms.push_back(uniform);
		}
	}

	return new_uniforms;
}

WGPUBindGroup RenderingDeviceDriverWebGpu::_select_bind_group_from_uniform_set(UniformSetInfo *p_uniform_set_info, const ShaderInfo *p_shader_info, uint32_t p_set_index) {
	WGPUBindGroupLayout layout = p_shader_info->bind_group_layouts[p_set_index];

	if (p_uniform_set_info->cached.has(layout)) {
		return p_uniform_set_info->cached.get(layout);
	} else {
		const HashSet<uint32_t> &binding_mask = p_shader_info->used_set_bindings[p_set_index];

		WGPUBindGroup bind_group = _mock_bind_group_create(
				p_shader_info->bind_group_layout_descs[p_set_index],
				p_shader_info->bind_group_layouts[p_set_index],
				p_shader_info->set_binding_remaps[p_set_index],
				p_shader_info->used_original_bindings_map[p_set_index],
				p_uniform_set_info->saved_uniforms,
				&binding_mask,
				p_shader_info->_set_index_has_push_constant_emulation(p_set_index) ? p_shader_info->push_constant_size : 0);
		p_uniform_set_info->bind_groups.push_back(bind_group);
		p_uniform_set_info->cached.insert(layout, bind_group);
		return bind_group;
	}
}

WGPUBindGroup RenderingDeviceDriverWebGpu::_mock_bind_group_create(const WGPUBindGroupLayoutDescriptor &p_descriptor, WGPUBindGroupLayout p_layout, const HashMap<uint32_t, Vector<uint32_t>> &p_set_binding_remaps, const HashMap<OriginalBindingIndex, UniformType> &p_used_original_bindings_map, const HashMap<uint32_t, BoundUniform> &p_override_uniforms, const HashSet<RemappedBindingIndex> *p_binding_mask, uint32_t p_push_constant_size) {
	HashMap<uint32_t, const WGPUBindGroupLayoutEntry *> entry_by_binding;
	for (uint32_t i = 0; i < p_descriptor.entryCount; i++) {
		entry_by_binding.insert(p_descriptor.entries[i].binding, &p_descriptor.entries[i]);
	}

	// _bind_group_create assumes ascending binding order.
	Vector<OriginalBindingIndex> sorted_bindings;
	for (const KeyValue<uint32_t, UniformType> &kv : p_used_original_bindings_map) {
		sorted_bindings.push_back(kv.key);
	}
	sorted_bindings.sort();

	Vector<Pair<uint32_t, UniformType>> index_bindings_input;
	for (uint32_t binding_idx = 0; binding_idx < sorted_bindings.size(); binding_idx++) {
		uint32_t binding = sorted_bindings.ptr()[binding_idx];
		index_bindings_input.push_back({ binding, p_used_original_bindings_map[binding] });
	}
	Vector<RemappedBinding> corrected_bindings = _remap_binding_indices(index_bindings_input, p_set_binding_remaps, p_binding_mask, BindingIndexType::CORRECTED);

	Vector<BoundUniform> uniforms;

	OriginalBindingIndex last_original_binding = -1;
	UniformType last_binding = UNIFORM_TYPE_MAX;
	LocalVector<ID> ids;

	for (int corrected_binding_idx = 0; corrected_binding_idx < corrected_bindings.size(); corrected_binding_idx++) {
		const RemappedBinding &binding = corrected_bindings[corrected_binding_idx];
		const UniformType type = binding.original_type;
		const OriginalBindingIndex original_binding = sorted_bindings[binding.input_idx];

		if (last_original_binding != original_binding && last_original_binding != (OriginalBindingIndex)-1 && ids.size() > 0) {
			uniforms.push_back((RDD::BoundUniform){
					.type = last_binding,
					.binding = last_original_binding,
					.ids = std::move(ids) });
			ids = LocalVector<ID>();
		}

		if (p_override_uniforms.has(original_binding)) {
			if (last_original_binding != original_binding) {
				const BoundUniform &uniform = p_override_uniforms[original_binding];
				uniforms.push_back(uniform);
			}
			last_original_binding = -1;
		} else {
			ERR_FAIL_COND_V_MSG(!entry_by_binding.has(binding.corrected_binding_idx), nullptr, "Missing layout entry while mocking bind group");
			const WGPUBindGroupLayoutEntry &entry = *entry_by_binding[binding.corrected_binding_idx];

			bool is_combined_texture_sampler = binding.remap.second && binding.remap.first == WEBGPU_BINDING_REMAP_SPLIT_COMBINED;

			if (is_combined_texture_sampler) {
				WGPUSamplerBindingLayout filtering = {};
				filtering.type = WGPUSamplerBindingType_Filtering;
				ids.push_back(this->_sampler_mock_binding_create(filtering));
				ids.push_back(this->_texture_mock_binding_create(entry.texture));
				corrected_binding_idx += 1;
			} else {
				ID id;
				if (entry.sampler.type != WGPUSamplerBindingType_BindingNotUsed) {
					id = this->_sampler_mock_binding_create(entry.sampler);
				} else if (entry.texture.sampleType != WGPUTextureSampleType_BindingNotUsed) {
					id = this->_texture_mock_binding_create(entry.texture);
				} else if (entry.storageTexture.access != WGPUStorageTextureAccess_BindingNotUsed) {
					id = this->_storage_texture_mock_binding_create(entry.storageTexture);
				} else if (entry.buffer.type != WGPUBufferBindingType_BindingNotUsed) {
					id = this->_buffer_mock_binding_create(entry.buffer);
				} else {
					id = ID();
				}
				ERR_FAIL_COND_V_MSG(id.id == ID().id, nullptr, "Empty id in _mock_bind_group_entry_create");
				ids.push_back(id);
			}
		}

		last_original_binding = original_binding;
		last_binding = type;
	}

	// Flush the last uniform.
	if (last_original_binding != (OriginalBindingIndex)-1 && ids.size() > 0) {
		uniforms.push_back((RDD::BoundUniform){
				.type = last_binding,
				.binding = last_original_binding,
				.ids = std::move(ids) });
	}

	WGPUBindGroup bind_group = _bind_group_create(uniforms, p_layout, p_set_binding_remaps, p_binding_mask, p_push_constant_size);
	return bind_group;
}

WGPUBindGroup RenderingDeviceDriverWebGpu::_mock_bind_group_create_or_get(const WGPUBindGroupLayoutDescriptor &p_descriptor, WGPUBindGroupLayout p_layout, const HashMap<uint32_t, Vector<uint32_t>> &p_set_binding_remaps, const HashMap<uint32_t, UniformType> &p_used_original_bindings_map, uint32_t p_push_constant_map) {
	if (this->mock_bind_groups.has(p_layout)) {
		return this->mock_bind_groups.get(p_layout);
	} else {
		WGPUBindGroup bind_group = _mock_bind_group_create(
				p_descriptor,
				p_layout,
				p_set_binding_remaps,
				p_used_original_bindings_map,
				{},
				nullptr,
				p_push_constant_map);
		this->mock_bind_groups.insert(p_layout, bind_group);
		return bind_group;
	}
}

RDD::SamplerID RenderingDeviceDriverWebGpu::_sampler_mock_binding_create(WGPUSamplerBindingLayout p_layout) {
	SamplerState sampler_state = SamplerState();

	switch (p_layout.type) {
		case WGPUSamplerBindingType_Comparison:
			sampler_state.enable_compare = true;
			sampler_state.compare_op = COMPARE_OP_ALWAYS;
			break;
		default:
			break;
	}

	return this->sampler_create(sampler_state);
}

RDD::TextureID RenderingDeviceDriverWebGpu::_texture_mock_binding_create(WGPUTextureBindingLayout p_layout) {
	TextureFormat format;
	TextureView view;
	format.usage_bits = TextureUsageBits::TEXTURE_USAGE_SAMPLING_BIT;

	switch (p_layout.sampleType) {
		case WGPUTextureSampleType_Undefined:
		case WGPUTextureSampleType_Float:
		case WGPUTextureSampleType_UnfilterableFloat:
			format.format = DataFormat::DATA_FORMAT_R32G32B32A32_SFLOAT;
			break;
		case WGPUTextureSampleType_Depth:
			format.format = DataFormat::DATA_FORMAT_D32_SFLOAT;
			format.usage_bits |= TextureUsageBits::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			break;
		case WGPUTextureSampleType_Sint:
			format.format = DataFormat::DATA_FORMAT_R32G32B32A32_SINT;
			break;
		case WGPUTextureSampleType_Uint:
			format.format = RDD::DataFormat::DATA_FORMAT_R32G32B32A32_UINT;
			break;
		default:
			break;
	}
	view.format = format.format;

	switch (p_layout.viewDimension) {
		case WGPUTextureViewDimension_1D:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_1D;
			break;
		case WGPUTextureViewDimension_2D:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_2D;
			break;
		case WGPUTextureViewDimension_2DArray:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_2D_ARRAY;
			break;
		case WGPUTextureViewDimension_Cube:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_CUBE;
			break;
		case WGPUTextureViewDimension_CubeArray:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_CUBE_ARRAY;
			format.array_layers = 6;
			break;
		case WGPUTextureViewDimension_3D:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_3D;
			break;
		default:
			break;
	}

	if (p_layout.multisampled) {
		format.samples = RenderingDeviceCommons::TEXTURE_SAMPLES_1;
	}

	TextureID texture = this->texture_create(format, view);
	return texture;
}

RDD::TextureID RenderingDeviceDriverWebGpu::_storage_texture_mock_binding_create(WGPUStorageTextureBindingLayout p_layout) {
	TextureFormat format;
	TextureView view;

	format.usage_bits |= TextureUsageBits::TEXTURE_USAGE_STORAGE_BIT;
	format.format = rd_texture_format_from_webgpu(p_layout.format);
	view.format = format.format;

	switch (p_layout.access) {
		case WGPUStorageTextureAccess_WriteOnly:
			format.usage_bits |=
					TextureUsageBits::TEXTURE_USAGE_CAN_COPY_TO_BIT |
					TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT;
			break;
		case WGPUStorageTextureAccess_ReadOnly:
			format.usage_bits |= TextureUsageBits::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
			break;
		case WGPUStorageTextureAccess_ReadWrite:
			format.usage_bits |=
					TextureUsageBits::TEXTURE_USAGE_CAN_COPY_TO_BIT |
					TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT |
					TextureUsageBits::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
			break;
		default:
			break;
	}

	switch (p_layout.viewDimension) {
		case WGPUTextureViewDimension_1D:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_1D;
			break;
		case WGPUTextureViewDimension_2D:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_2D;
			break;
		case WGPUTextureViewDimension_2DArray:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_2D_ARRAY;
			break;
		case WGPUTextureViewDimension_Cube:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_CUBE;
			break;
		case WGPUTextureViewDimension_CubeArray:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_CUBE_ARRAY;
			break;
		case WGPUTextureViewDimension_3D:
			format.texture_type = RenderingDeviceCommons::TEXTURE_TYPE_3D;
			break;
		default:
			break;
	}

	TextureID texture = this->texture_create(format, view);
	return texture;
}

RDD::BufferID RenderingDeviceDriverWebGpu::_buffer_mock_binding_create(WGPUBufferBindingLayout p_layout) {
	BitField<BufferUsageBits> usage = 0;
	switch (p_layout.type) {
		case WGPUBufferBindingType_Uniform:
			usage.set_flag(RDD::BufferUsageBits::BUFFER_USAGE_UNIFORM_BIT);
			break;
		case WGPUBufferBindingType_Storage:
		case WGPUBufferBindingType_ReadOnlyStorage:
			usage.set_flag(RDD::BufferUsageBits::BUFFER_USAGE_STORAGE_BIT);
			break;
		default:
			break;
	}

	// HACK: At layout time, we cannot know the size of all mock SSBO's.
	// If all else fails, we use this hardcoded, big-ish number.
	// Internally, `wgpu` checks this size using data at draw / dispatch time.
	// We could also use this "late" data for this purpose (among other hacks) if this issue persists.
	const uint32_t max_binding_size = 65536;
	BufferID buffer = this->buffer_create(p_layout.minBindingSize ? p_layout.minBindingSize : max_binding_size, usage, RDD::MemoryAllocationType::MEMORY_ALLOCATION_TYPE_GPU, 0);
	return buffer;
}

RenderingDeviceDriver::UniformSetID RenderingDeviceDriverWebGpu::uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader, uint32_t p_set_index, int p_linear_pool_index) {
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	WGPUBindGroup bind_group = _bind_group_create(
			p_uniforms,
			shader_info->bind_group_layouts[p_set_index],
			shader_info->set_binding_remaps[p_set_index],
			&shader_info->used_set_bindings[p_set_index],
			shader_info->_set_index_has_push_constant_emulation(p_set_index) ? shader_info->push_constant_size : 0);

	HashMap<uint32_t, BoundUniform> saved_uniforms;
	for (uint32_t i = 0; i < p_uniforms.size(); i++) {
		const BoundUniform &uniform = p_uniforms.ptr()[i];
		saved_uniforms.insert(uniform.binding, uniform);
	}

	ERR_FAIL_COND_V(bind_group == nullptr, UniformSetID());

	UniformSetInfo *uniform_set_info = memnew(UniformSetInfo);
	*uniform_set_info = {};
	uniform_set_info->saved_uniforms = saved_uniforms;
	uniform_set_info->bind_groups = { bind_group };
	uniform_set_info->cached = {
		{ shader_info->bind_group_layouts[p_set_index], bind_group }
	};

	// Record dynamic uniform/storage buffers in binding order so the bind commands
	// can resolve frame_idx -> byte offset against this set later.
	for (uint32_t i = 0; i < p_uniforms.size(); i++) {
		const BoundUniform &u = p_uniforms.ptr()[i];
		if (!u.is_dynamic() || u.ids.is_empty()) {
			continue;
		}
		BufferInfo *bi = (BufferInfo *)u.ids[0].id;
		ERR_CONTINUE_MSG(!bi || !bi->is_dynamic(),
				"BoundUniform marked dynamic but bound buffer is not BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT.");
		uniform_set_info->dynamic_buffers.push_back(static_cast<BufferDynamicInfo *>(bi));
	}

	return UniformSetID(uniform_set_info);
}

void RenderingDeviceDriverWebGpu::uniform_set_free(UniformSetID p_uniform_set) {
	// TODO: This is old stuff.
	// WGPUBindGroup bind_group = (WGPUBindGroup)p_uniform_set.id;
	// wgpuBindGroupRelease(bind_group);
}

uint32_t RenderingDeviceDriverWebGpu::uniform_sets_get_dynamic_offsets(VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) const {
	// TODO: Include those related to push constant emulation.
	// Pretty much what Vulkan does.
	uint32_t mask = 0u;
	uint32_t shift = 0u;
	for (uint32_t i = 0; i < p_set_count; i++) {
		const UniformSetInfo *usi = (const UniformSetInfo *)p_uniform_sets[i].id;
		for (const BufferDynamicInfo *dyn : usi->dynamic_buffers) {
			DEV_ASSERT(dyn->frame_idx <= UNIFORM_DYN_MASK);
			mask |= (dyn->frame_idx & UNIFORM_DYN_MASK) << shift;
			shift += UNIFORM_DYN_BITS;
		}
	}
	return mask;
}

// ----- COMMANDS -----

void RenderingDeviceDriverWebGpu::command_uniform_set_prepare_for_use(CommandBufferID _p_cmd_buffer, UniformSetID _p_uniform_set, ShaderID _p_shader, uint32_t _p_set_index) {
	// Empty.
}

/******************/
/**** TRANSFER ****/
/******************/

void RenderingDeviceDriverWebGpu::command_clear_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	wgpuCommandEncoderClearBuffer(command_buffer_info->encoder, buffer_info->buffer, p_offset, p_size);

	if (buffer_info->is_mapped) {
		this->buffer_unmap(p_buffer);
	}
}

void RenderingDeviceDriverWebGpu::command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *src_buffer_info = (BufferInfo *)p_src_buffer.id;
	BufferInfo *dst_buffer_info = (BufferInfo *)p_dst_buffer.id;

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		BufferCopyRegion region = p_regions[i];
		if (src_buffer_info->upload_shadow) {
			_upload_shadow_range(src_buffer_info, region.src_offset, region.size);
		}
		wgpuCommandEncoderCopyBufferToBuffer(command_buffer_info->encoder, src_buffer_info->buffer, region.src_offset, dst_buffer_info->buffer, region.dst_offset, STEPIFY(region.size, 256));
	}

	if (src_buffer_info->is_mapped) {
		this->buffer_unmap(p_src_buffer);
	}
	if (dst_buffer_info->is_mapped) {
		this->buffer_unmap(p_dst_buffer);
	}
}

void RenderingDeviceDriverWebGpu::command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout _p_src_texture_layout, TextureID p_dst_texture, TextureLayout _p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	TextureInfo *src_texture_info = (TextureInfo *)p_src_texture.id;
	TextureInfo *dst_texture_info = (TextureInfo *)p_dst_texture.id;

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		TextureCopyRegion region = p_regions[i];
		WGPUTexelCopyTextureInfo src_texture_cp = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
		src_texture_cp.texture = src_texture_info->texture;
		src_texture_cp.mipLevel = region.src_subresources.mipmap;
		src_texture_cp.origin = WGPU_ORIGIN_3D_INIT;
		src_texture_cp.origin.x = (uint32_t)region.src_offset.x;
		src_texture_cp.origin.y = (uint32_t)region.src_offset.y;
		src_texture_cp.origin.z = (uint32_t)region.src_offset.z;
		src_texture_cp.aspect = webgpu_texture_aspect_from_rd(region.src_subresources.aspect);
		WGPUTexelCopyTextureInfo dst_texture_cp = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
		dst_texture_cp.texture = dst_texture_info->texture;
		dst_texture_cp.mipLevel = region.dst_subresources.mipmap;
		dst_texture_cp.origin = WGPU_ORIGIN_3D_INIT;
		dst_texture_cp.origin.x = (uint32_t)region.dst_offset.x;
		dst_texture_cp.origin.y = (uint32_t)region.dst_offset.y;
		dst_texture_cp.origin.z = (uint32_t)region.dst_offset.z;
		dst_texture_cp.aspect = webgpu_texture_aspect_from_rd(region.dst_subresources.aspect);

		WGPUExtent3D cp_size = WGPU_EXTENT_3D_INIT;
		cp_size.width = (uint32_t)region.size.x;
		cp_size.height = (uint32_t)region.size.y;
		cp_size.depthOrArrayLayers = (uint32_t)region.size.z;

		// WebGPU requires copy extents to be multiples of the compressed block size.
		// For uncompressed formats, block_w/block_h are 1 so this is a no-op.
		uint32_t block_w = 1, block_h = 1;
		get_compressed_image_format_block_dimensions(src_texture_info->rd_texture_format, block_w, block_h);
		if (block_w > 1 || block_h > 1) {
			cp_size.width = STEPIFY(cp_size.width, block_w);
			cp_size.height = STEPIFY(cp_size.height, block_h);
		}

		wgpuCommandEncoderCopyTextureToTexture(command_buffer_info->encoder, &src_texture_cp, &dst_texture_cp, &cp_size);
	}
}

void RenderingDeviceDriverWebGpu::command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}
void RenderingDeviceDriverWebGpu::command_clear_color_texture(CommandBufferID _p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, const Color &p_color, const TextureSubresourceRange &p_subresources) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}

void RenderingDeviceDriverWebGpu::command_clear_depth_stencil_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, float p_depth, uint8_t p_stencil, const TextureSubresourceRange &p_subresources) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}

void RenderingDeviceDriverWebGpu::command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, TextureID p_dst_texture, TextureLayout _p_dst_texture_layout, VectorView<BufferTextureCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *src_buffer_info = (BufferInfo *)p_src_buffer.id;
	TextureInfo *dst_texture_info = (TextureInfo *)p_dst_texture.id;

	FormatBlockDimension block_dimensions = webgpu_texture_format_block_dimensions(dst_texture_info->texture_view_desc.format);

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		BufferTextureCopyRegion region = p_regions[i];

		uint32_t block_copy_size = webgpu_texture_format_block_copy_size(dst_texture_info->texture_desc.format, dst_texture_info->texture_view_desc.aspect);

		uint32_t block_width = block_dimensions.block_dim_x;
		uint32_t block_height = block_dimensions.block_dim_y;
		uint32_t bytes_per_block = block_copy_size;

		uint32_t blocks_per_row =
				(region.texture_region_size.x + block_width - 1) / block_width;

		uint32_t blocks_per_column =
				(region.texture_region_size.y + block_height - 1) / block_height;

		WGPUTexelCopyBufferInfo cp_buffer = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
		cp_buffer.layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
		cp_buffer.layout.offset = region.buffer_offset;
		cp_buffer.layout.bytesPerRow = (blocks_per_row * bytes_per_block + 255) & ~255;
		cp_buffer.layout.rowsPerImage = region.texture_region_size.z > 1
				? blocks_per_column
				: WGPU_COPY_STRIDE_UNDEFINED;
		cp_buffer.buffer = src_buffer_info->buffer;

		WGPUTexelCopyTextureInfo cp_texture = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
		cp_texture.texture = dst_texture_info->texture;
		cp_texture.mipLevel = region.texture_subresource.mipmap;
		cp_texture.origin = WGPU_ORIGIN_3D_INIT;
		cp_texture.origin.x = (uint32_t)region.texture_offset.x;
		cp_texture.origin.y = (uint32_t)region.texture_offset.y;
		cp_texture.origin.z = (uint32_t)region.texture_offset.z;
		cp_texture.aspect = webgpu_texture_aspect_from_rd(region.texture_subresource.aspect);
		WGPUExtent3D cp_size = WGPU_EXTENT_3D_INIT;
		cp_size.width = (uint32_t)region.texture_region_size.x;
		cp_size.height = (uint32_t)region.texture_region_size.y;
		cp_size.depthOrArrayLayers = (uint32_t)region.texture_region_size.z;

		// WebGPU requires copy extents to be multiples of the compressed block size.
		// For uncompressed formats, block_w/block_h are 1 so this is a no-op.
		uint32_t block_w = 1, block_h = 1;
		get_compressed_image_format_block_dimensions(dst_texture_info->rd_texture_format, block_w, block_h);
		if (block_w > 1 || block_h > 1) {
			cp_size.width = STEPIFY(cp_size.width, block_w);
			cp_size.height = STEPIFY(cp_size.height, block_h);
		}

		if (src_buffer_info->upload_shadow) {
			uint64_t rows = (cp_size.height + block_height - 1) / block_height;
			uint64_t bytes = uint64_t(cp_buffer.layout.bytesPerRow) * rows * MAX(cp_size.depthOrArrayLayers, 1u);
			_upload_shadow_range(src_buffer_info, region.buffer_offset, bytes);
		}
		wgpuCommandEncoderCopyBufferToTexture(command_buffer_info->encoder, &cp_buffer, &cp_texture, &cp_size);
	}

	if (src_buffer_info->is_mapped) {
		this->buffer_unmap(p_src_buffer);
	}
}

void RenderingDeviceDriverWebGpu::command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer, VectorView<BufferTextureCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	TextureInfo *src_texture_info = (TextureInfo *)p_src_texture.id;
	BufferInfo *dst_buffer_info = (BufferInfo *)p_dst_buffer.id;

	FormatBlockDimension block_dimensions = webgpu_texture_format_block_dimensions(src_texture_info->texture_view_desc.format);

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		BufferTextureCopyRegion region = p_regions[i];

		uint32_t block_copy_size = webgpu_texture_format_block_copy_size(src_texture_info->texture_desc.format, src_texture_info->texture_view_desc.aspect);

		WGPUTexelCopyTextureInfo cp_texture = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
		cp_texture.texture = src_texture_info->texture;
		cp_texture.mipLevel = region.texture_subresource.mipmap;
		cp_texture.origin = WGPU_ORIGIN_3D_INIT;
		cp_texture.origin.x = (uint32_t)region.texture_offset.x;
		cp_texture.origin.y = (uint32_t)region.texture_offset.y;
		cp_texture.origin.z = (uint32_t)region.texture_offset.z;
		cp_texture.aspect = webgpu_texture_aspect_from_rd(region.texture_subresource.aspect);

		WGPUTexelCopyBufferInfo cp_buffer = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
		cp_buffer.layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
		cp_buffer.layout.offset = region.buffer_offset;
		cp_buffer.layout.bytesPerRow = ((region.texture_region_size.x * block_copy_size) / block_dimensions.block_dim_x + 255) & ~255;
		cp_buffer.layout.rowsPerImage = region.texture_region_size.z > 1 ? region.texture_region_size.y / block_dimensions.block_dim_y : WGPU_COPY_STRIDE_UNDEFINED;
		cp_buffer.buffer = dst_buffer_info->buffer;

		WGPUExtent3D cp_size = WGPU_EXTENT_3D_INIT;
		cp_size.width = (uint32_t)region.texture_region_size.x;
		cp_size.height = (uint32_t)region.texture_region_size.y;
		cp_size.depthOrArrayLayers = (uint32_t)region.texture_region_size.z;

		wgpuCommandEncoderCopyTextureToBuffer(command_buffer_info->encoder, &cp_texture, &cp_buffer, &cp_size);
	}

	if (dst_buffer_info->is_mapped) {
		this->buffer_unmap(p_dst_buffer);
	}
}

/******************/
/**** PIPELINE ****/
/******************/

void RenderingDeviceDriverWebGpu::pipeline_free(PipelineID p_pipeline) {
	// TODO: impl
}

// ----- BINDING -----

void RenderingDeviceDriverWebGpu::command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader, uint32_t p_first_index, VectorView<uint32_t> p_data) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	uint32_t byte_size = p_data.size() * (uint32_t)sizeof(uint32_t);
	Vector<uint8_t> data = Vector<uint8_t>();
	data.resize_initialized(byte_size);
	memcpy(data.ptrw(), p_data.ptr(), byte_size);

	uint32_t byte_offset = p_first_index * (uint32_t)sizeof(uint32_t);
	ERR_FAIL_COND_MSG(byte_offset + byte_size > WEBGPU_MAX_IMMEDIATE_SIZE,
			vformat("Push constant write of %d bytes at exceeds emulation max of %d",
					byte_size, WEBGPU_MAX_IMMEDIATE_SIZE));

	if (shader_info->push_constant_stage_flags & WGPUShaderStage_Compute) {
		command_buffer_info->has_compute_commands = true;
		command_buffer_info->commands.push_back((PassEncoderCommand){
				.type = PassEncoderCommand::CommandType::COMPUTE_SET_IMMEDIATES,
				.compute_set_immediates = (PassEncoderCommand::ComputeSetImmediates){
						.offset = byte_offset,
						.emulation_offset = 0,
				},
				.compute_push_constants = data,
		});
	} else if (shader_info->push_constant_stage_flags & WGPUShaderStage_Vertex || shader_info->push_constant_stage_flags & WGPUShaderStage_Fragment) {
		command_buffer_info->commands.push_back((PassEncoderCommand){
				.type = PassEncoderCommand::CommandType::RENDER_SET_IMMEDIATES,
				.render_set_immediates = (PassEncoderCommand::RenderSetImmediates){
						.offset = byte_offset,
						.emulation_offset = 0,
				},
				.render_push_constants = data,
		});
	}
}

// ----- CACHE -----

bool RenderingDeviceDriverWebGpu::pipeline_cache_create(const Vector<uint8_t> &_p_data) {
	// WebGpu does not have pipeline caches.
	return false;
}
void RenderingDeviceDriverWebGpu::pipeline_cache_free() {
	// Empty.
}
size_t RenderingDeviceDriverWebGpu::pipeline_cache_query_size() {
	return 0;
}
Vector<uint8_t> RenderingDeviceDriverWebGpu::pipeline_cache_serialize() {
	return Vector<uint8_t>();
}

/*******************/
/**** RENDERING ****/
/*******************/

// ----- SUBPASS -----

RenderingDeviceDriverWebGpu::RenderPassAttachmentInfo RenderingDeviceDriverWebGpu::_empty_render_pass_attachment_create() {
	return (RenderPassAttachmentInfo){
		.format = WGPUTextureFormat_Depth32Float,
		// TODO: HELLO WHY IS THIS FOUR???
		.sample_count = 4,
		.load_op = WGPULoadOp_Undefined,
		.store_op = WGPUStoreOp_Undefined,
		.stencil_load_op = WGPULoadOp_Undefined,
		.stencil_store_op = WGPUStoreOp_Undefined,
		.is_depth_stencil = true,
		.is_depth_stencil_read_only = true
	};
}

RenderingDeviceDriver::RenderPassID RenderingDeviceDriverWebGpu::render_pass_create(VectorView<Attachment> p_attachments, VectorView<Subpass> _p_subpasses, VectorView<SubpassDependency> _p_subpass_dependencies, uint32_t p_view_count, AttachmentReference p_fragment_density_map_attachment) {
	// WebGpu does not have subpasses so we will store this info until we create a render pipeline later.
	RenderPassInfo *render_pass_info = memnew(RenderPassInfo);
	render_pass_info->depth_attachment_index = UINT32_MAX;

	render_pass_info->attachments = Vector<RenderPassAttachmentInfo>();
	for (uint32_t i = 0; i < p_attachments.size(); i++) {
		Attachment attachment = p_attachments[i];
		bool is_depth_stencil =
				attachment.final_layout == TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
				attachment.final_layout == TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		bool is_depth_stencil_read_only =
				attachment.final_layout == TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		if (is_depth_stencil) {
			render_pass_info->depth_attachment_index = i;
		}

		RenderPassAttachmentInfo attachment_info = (RenderPassAttachmentInfo){
			.format = webgpu_texture_format_from_rd(attachment.format),
			// TODO: Assert that p_format.samples follows this behavior.
			.sample_count = (uint32_t)pow(2, (uint32_t)attachment.samples),
			.load_op = webgpu_load_op_from_rd(attachment.load_op),
			.store_op = webgpu_store_op_from_rd(attachment.store_op),
			.stencil_load_op = webgpu_load_op_from_rd(attachment.stencil_load_op),
			.stencil_store_op = webgpu_store_op_from_rd(attachment.stencil_store_op),
			.is_depth_stencil = is_depth_stencil,
			.is_depth_stencil_read_only = is_depth_stencil_read_only
		};
		render_pass_info->attachments.push_back(attachment_info);
	}

	// WebGPU has one subpass per pass; its resolve references become resolveTarget views.
	if (_p_subpasses.size() > 0) {
		const Subpass &subpass = _p_subpasses[0];
		for (uint32_t i = 0; i < subpass.resolve_references.size() && i < subpass.color_references.size(); i++) {
			const uint32_t resolve = subpass.resolve_references[i].attachment;
			const uint32_t color = subpass.color_references[i].attachment;
			if (resolve != AttachmentReference::UNUSED && color != AttachmentReference::UNUSED && resolve < (uint32_t)render_pass_info->attachments.size()) {
				render_pass_info->attachments.write[resolve].resolve_of = (int32_t)color;
			}
		}
		const uint32_t depth_resolve = subpass.depth_resolve_reference.attachment;
		if (depth_resolve != AttachmentReference::UNUSED && depth_resolve < (uint32_t)render_pass_info->attachments.size()) {
			render_pass_info->attachments.write[depth_resolve].resolve_of = -2;
			render_pass_info->attachments.write[depth_resolve].is_depth_stencil = false;
			render_pass_info->attachments.write[depth_resolve].is_depth_stencil_read_only = false;
			const uint32_t depth = subpass.depth_stencil_reference.attachment;
			if (depth != AttachmentReference::UNUSED) {
				render_pass_info->depth_attachment_index = depth;
			}
		}
	}

	if (p_attachments.size() == 0) {
		RenderPassAttachmentInfo attachment = _empty_render_pass_attachment_create();
		render_pass_info->attachments.push_back(attachment);

		render_pass_info->empty_attachment = true;
		render_pass_info->depth_attachment_index = 0;
	}

	render_pass_info->view_count = p_view_count;

	return RenderPassID(render_pass_info);
}
void RenderingDeviceDriverWebGpu::render_pass_free(RenderPassID p_render_pass) {
	RenderPassInfo *render_pass_info = (RenderPassInfo *)p_render_pass.id;
	memdelete(render_pass_info);
}

// ----- COMMANDS -----

void RenderingDeviceDriverWebGpu::command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i &p_rect, VectorView<RenderPassClearValue> p_clear_values) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	Vector<WGPURenderPassColorAttachment> color_attachments;

	RenderPassInfo *render_pass_info = (RenderPassInfo *)p_render_pass.id;
	FramebufferInfo *framebuffer_info = (FramebufferInfo *)p_framebuffer.id;
	command_buffer_info->render_area = framebuffer_info->size;

	// DEV_ASSERT(render_pass_info->attachments.size() == framebuffer_info->attachments.size());

	WGPUTextureView maybe_surface_texture_view = nullptr;
	Pair<WGPURenderPassDepthStencilAttachment, bool> maybe_depth_stencil_attachment = Pair((WGPURenderPassDepthStencilAttachment){}, false);

	int32_t *color_slot = ALLOCA_ARRAY(int32_t, render_pass_info->attachments.size());
	for (uint32_t i = 0; i < render_pass_info->attachments.size(); i++) {
		color_slot[i] = -1;
	}
	for (uint32_t i = 0; i < render_pass_info->attachments.size(); i++) {
		RenderPassAttachmentInfo attachment = render_pass_info->attachments[i];

		if (attachment.resolve_of != -1) {
			continue; // A resolve target; attached to its color attachment below.
		}
		if (attachment.is_depth_stencil || attachment.is_depth_stencil_read_only) {
			TextureID attachment_texture_id = framebuffer_info->attachments[i];
			TextureInfo *attachment_texture = (TextureInfo *)attachment_texture_id.id;

			const bool has_depth = webgpu_texture_format_has_depth_aspect(attachment.format);
			const bool has_stencil = webgpu_texture_format_has_stencil_aspect(attachment.format);
			const bool use_depth = has_depth && !attachment.is_depth_stencil_read_only;
			const bool use_stencil = has_stencil && !attachment.is_depth_stencil_read_only;

			maybe_depth_stencil_attachment.first = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
			maybe_depth_stencil_attachment.first.view = attachment_texture->get_default_view();
			maybe_depth_stencil_attachment.first.depthLoadOp = use_depth ? attachment.load_op : WGPULoadOp_Undefined;
			maybe_depth_stencil_attachment.first.depthStoreOp = use_depth ? attachment.store_op : WGPUStoreOp_Undefined;
			maybe_depth_stencil_attachment.first.depthClearValue = p_clear_values[i].depth;
			maybe_depth_stencil_attachment.first.depthReadOnly = has_depth && attachment.is_depth_stencil_read_only;
			maybe_depth_stencil_attachment.first.stencilLoadOp = use_stencil ? attachment.stencil_load_op : WGPULoadOp_Undefined;
			maybe_depth_stencil_attachment.first.stencilStoreOp = use_stencil ? attachment.stencil_store_op : WGPUStoreOp_Undefined;
			maybe_depth_stencil_attachment.first.stencilClearValue = p_clear_values[i].stencil;
			maybe_depth_stencil_attachment.first.stencilReadOnly = has_stencil && attachment.is_depth_stencil_read_only;
			maybe_depth_stencil_attachment.second = true;
		} else {
			WGPUTextureView view;
			if (framebuffer_info->maybe_swapchain) {
				SwapChainInfo *swapchain_info = (SwapChainInfo *)framebuffer_info->maybe_swapchain.id;
				RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;

				WGPUSurfaceTexture surface_texture;
				wgpuSurfaceGetCurrentTexture(surface->surface, &surface_texture);

				WGPUTextureView surface_texture_view = wgpuTextureCreateView(surface_texture.texture, nullptr);
				maybe_surface_texture_view = surface_texture_view;

				view = surface_texture_view;
			} else {
				TextureID attachment_texture_id = framebuffer_info->attachments[i];
				TextureInfo *attachment_texture = (TextureInfo *)attachment_texture_id.id;
				view = attachment_texture->get_default_view();
			}

			WGPURenderPassColorAttachment color_attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
			color_attachment.view = view;
			color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
			color_attachment.loadOp = attachment.load_op;
			color_attachment.storeOp = attachment.store_op;
			color_attachment.clearValue = { p_clear_values[i].color.r, p_clear_values[i].color.g, p_clear_values[i].color.b, p_clear_values[i].color.a };
			color_slot[i] = color_attachments.size();
			color_attachments.push_back(color_attachment);
		}
	}
	for (uint32_t i = 0; i < render_pass_info->attachments.size(); i++) {
		const int32_t of = render_pass_info->attachments[i].resolve_of;
		if (of >= 0 && of < (int32_t)render_pass_info->attachments.size() && color_slot[of] >= 0 && !framebuffer_info->maybe_swapchain) {
			TextureInfo *resolve_texture = (TextureInfo *)framebuffer_info->attachments[i].id;
			color_attachments.write[color_slot[of]].resolveTarget = resolve_texture->get_default_view();
		}
	}

	command_buffer_info->active_render_pass_info = (RenderPassEncoderInfo){
		.color_attachments = color_attachments,
		.depth_stencil_attachment = maybe_depth_stencil_attachment,
		.maybe_surface_texture_view = maybe_surface_texture_view,
	};
	command_buffer_info->is_render_pass_active = true;
}

void RenderingDeviceDriverWebGpu::command_end_render_pass(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	// Flush compute pass to preserve ordering.
	_flush_active_command_pass(*command_buffer_info);
}

void RenderingDeviceDriverWebGpu::command_next_render_subpass(CommandBufferID _p_cmd_buffer, CommandBufferType _p_cmd_buffer_type) {
	// Empty.
}

void RenderingDeviceDriverWebGpu::command_render_set_viewport(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	ERR_FAIL_COND_MSG(p_viewports.size() != 1, "WebGpu cannot set multiple viewports.");

	for (uint32_t i = 0; i < p_viewports.size(); i++) {
		command_buffer_info->commands.push_back(((PassEncoderCommand){
				.type = PassEncoderCommand::CommandType::RENDER_SET_VIEWPORT,
				.render_set_viewport = (PassEncoderCommand::RenderSetViewport){
						.x = (float)p_viewports[i].position.x,
						.y = (float)p_viewports[i].position.y,
						.width = (float)p_viewports[i].size.x,
						.height = (float)p_viewports[i].size.y,
						.min_depth = 0.0,
						.max_depth = 1.0,
				},
		}));
	}
}

void RenderingDeviceDriverWebGpu::command_render_set_scissor(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	ERR_FAIL_COND_MSG(p_scissors.size() != 1, "WebGpu cannot set multiple scissors.");

	for (uint32_t i = 0; i < p_scissors.size(); i++) {
		// Vulkan clips a scissor to the render area; WebGPU rejects one that reaches outside it.
		Rect2i scissor = p_scissors[i];
		if (command_buffer_info->render_area != Size2i()) {
			scissor = scissor.intersection(Rect2i(Point2i(), command_buffer_info->render_area));
		}
		command_buffer_info->commands.push_back(((PassEncoderCommand){
				.type = PassEncoderCommand::CommandType::RENDER_SET_SCISSOR_RECT,
				.render_set_scissor_rect = (PassEncoderCommand::RenderSetScissorRect){
						.x = (uint32_t)scissor.position.x,
						.y = (uint32_t)scissor.position.y,
						.width = (uint32_t)scissor.size.width,
						.height = (uint32_t)scissor.size.height,
				},
		}));
	}
}

void RenderingDeviceDriverWebGpu::command_render_clear_attachments(CommandBufferID _p_cmd_buffer, VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}

// Binding.
void RenderingDeviceDriverWebGpu::command_bind_render_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	PipelineInfo *pipeline_info = (PipelineInfo *)p_pipeline.id;
	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_SET_PIPELINE,

			.set_pipeline = (PassEncoderCommand::SetPipeline){
					.pipeline_info = pipeline_info,
			} }));
}

void RenderingDeviceDriverWebGpu::command_bind_render_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	uint32_t shift = 0u;
	for (uint32_t i = 0; i < p_set_count; i++) {
		UniformSetInfo *uniform_set_info = (UniformSetInfo *)p_uniform_sets[i].id;
		WGPUBindGroup bind_group = _select_bind_group_from_uniform_set(uniform_set_info, shader_info, p_first_set_index + i);

		PassEncoderCommand cmd = {
			.type = PassEncoderCommand::CommandType::RENDER_SET_BIND_GROUP,
			.set_bind_group = (PassEncoderCommand::SetBindGroup){
					.group_index = p_first_set_index + i,
					.bind_group = bind_group,
					.shader_info = shader_info,
			},
		};
		// Peel one slot per dynamic binding and convert frame_idx -> byte offset.
		for (const BufferDynamicInfo *dyn : uniform_set_info->dynamic_buffers) {
			uint32_t frame_idx = (p_dynamic_offsets >> shift) & UNIFORM_DYN_MASK;
			shift += UNIFORM_DYN_BITS;
			cmd.dynamic_offsets.push_back(uint32_t(frame_idx * dyn->size));
		}
		command_buffer_info->commands.push_back(cmd);
	}
}

// Drawing.
void RenderingDeviceDriverWebGpu::command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count, uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_DRAW,
			.render_draw = (PassEncoderCommand::RenderDraw){
					.vertex_count = p_vertex_count,
					.instance_count = p_instance_count,
					.first_vertex = p_base_vertex,
					.first_instance = p_first_instance,
			},
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset, uint32_t p_first_instance) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_DRAW_INDEXED,
			.render_draw_indexed = (PassEncoderCommand::RenderDrawIndexed){
					.index_count = p_index_count,
					.instance_count = p_instance_count,
					.first_index = p_first_index,
					.base_vertex = p_vertex_offset,
					.first_instance = p_first_instance,
			} }));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDIRECT,
			.render_multi_draw_indirect = (PassEncoderCommand::RenderMultiDrawIndirect){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count = p_draw_count },
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;
	BufferInfo *count_buffer = (BufferInfo *)p_count_buffer.id;

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDIRECT_COUNT,
			.render_multi_draw_indirect_count = (PassEncoderCommand::RenderMultiDrawIndirectCount){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count_buffer = count_buffer->buffer,
					.count_offset = p_count_buffer_offset,
					.max_count = p_max_draw_count,

			},
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDEXED_INDIRECT,
			.render_multi_draw_indexed_indirect = (PassEncoderCommand::RenderMultiDrawIndexedIndirect){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count = p_draw_count },
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;
	BufferInfo *count_buffer = (BufferInfo *)p_count_buffer.id;

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_MULTI_DRAW_INDEXED_INDIRECT_COUNT,
			.render_multi_draw_indexed_indirect_count = (PassEncoderCommand::RenderMultiDrawIndexedIndirectCount){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count_buffer = count_buffer->buffer,
					.count_offset = p_count_buffer_offset,
					.max_count = p_max_draw_count,
			} }));
}

// Buffer binding.
void RenderingDeviceDriverWebGpu::command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer, uint32_t p_binding_count, const BufferID *p_buffers, const uint64_t *p_offsets, uint64_t p_dynamic_offsets) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	for (uint32_t i = 0; i < p_binding_count; i++) {
		BufferInfo *buffer_info = (BufferInfo *)p_buffers[i].id;
		uint64_t offset = p_offsets[i];
		if (buffer_info->is_dynamic()) {
			uint64_t frame_idx = p_dynamic_offsets & VERTEX_DYN_MASK;
			p_dynamic_offsets >>= VERTEX_DYN_BITS;
			offset += frame_idx * buffer_info->size;
		}

		command_buffer_info->commands.push_back(((PassEncoderCommand){
				.type = PassEncoderCommand::CommandType::RENDER_SET_VERTEX_BUFFER,
				.render_set_vertex_buffer = (PassEncoderCommand::RenderSetVertexBuffer){
						.slot = i,
						.buffer = buffer_info->buffer,
						.offset = offset,
						.size = WGPU_WHOLE_SIZE,
				},
		}));
	}
}

void RenderingDeviceDriverWebGpu::command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, IndexBufferFormat p_format, uint64_t p_offset) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	WGPUIndexFormat format = WGPUIndexFormat_Undefined;
	switch (p_format) {
		case RenderingDeviceCommons::INDEX_BUFFER_FORMAT_UINT16:
			format = WGPUIndexFormat_Uint16;
			break;
		case RenderingDeviceCommons::INDEX_BUFFER_FORMAT_UINT32:
			format = WGPUIndexFormat_Uint32;
			break;
	}

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_SET_INDEX_BUFFER,
			.render_set_index_buffer = (PassEncoderCommand::RenderSetIndexBuffer){
					.buffer = buffer_info->buffer,
					.format = format,
					.offset = p_offset,
					.size = WGPU_WHOLE_SIZE,
			} }));
}

// Dynamic state.
void RenderingDeviceDriverWebGpu::command_render_set_blend_constants(CommandBufferID p_cmd_buffer, const Color &p_constants) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	command_buffer_info->commands.push_back(((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::RENDER_SET_BLEND_CONSTANTS,
			.render_set_blend_constant = (PassEncoderCommand::RenderSetBlendConstant){
					.color = { p_constants.r, p_constants.g, p_constants.b, p_constants.a } },
	}));
}

void RenderingDeviceDriverWebGpu::command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) {
	// Note: This functionality is unsupported.
	// Empty.
}

// ----- PIPELINE -----

Vector<WGPUConstantEntry> RenderingDeviceDriverWebGpu::_get_specialization_constant_entries(const VectorView<PipelineSpecializationConstant> &p_specialization_constants, const HashMap<uint32_t, CharString> &p_override_layout) {
	Vector<WGPUConstantEntry> overrides;
	for (uint32_t i = 0; i < p_specialization_constants.size(); i++) {
		const PipelineSpecializationConstant &constant = p_specialization_constants.ptr()[i];
		if (p_override_layout.has(constant.constant_id)) {
			const CharString &name = p_override_layout.get(constant.constant_id);
			double value;
			if (constant.type == PipelineSpecializationConstantType::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT) {
				value = (double)constant.float_value;
			} else if (constant.type == PipelineSpecializationConstantType::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT) {
				value = (double)constant.int_value;
			} else {
				value = (double)constant.bool_value;
			}
			WGPUConstantEntry constant_entry = WGPU_CONSTANT_ENTRY_INIT;
			constant_entry.key = { name.ptr(), WGPU_STRLEN };
			constant_entry.value = value;
			overrides.push_back(constant_entry);
		}
	}
	return overrides;
}

RenderingDeviceDriver::PipelineID RenderingDeviceDriverWebGpu::render_pipeline_create(
		ShaderID p_shader,
		VertexFormatID p_vertex_format,
		RenderPrimitive p_render_primitive,
		PipelineRasterizationState p_rasterization_state,
		PipelineMultisampleState p_multisample_state,
		PipelineDepthStencilState p_depth_stencil_state,
		PipelineColorBlendState p_blend_state,
		VectorView<int32_t> p_color_attachments,
		BitField<PipelineDynamicStateFlags> p_dynamic_state,
		RenderPassID p_render_pass,
		uint32_t p_render_subpass,
		VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	WEBGPU_ON_MAIN_THREAD(PipelineID, render_pipeline_create(p_shader, p_vertex_format, p_render_primitive, p_rasterization_state, p_multisample_state, p_depth_stencil_state, p_blend_state, p_color_attachments, p_dynamic_state, p_render_pass, p_render_subpass, p_specialization_constants));
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;
	WGPURenderPipelineDescriptor pipeline_descriptor = {};

	CharString pipeline_label = shader_info->shader_name.utf8();
	pipeline_descriptor.label = WGPU_STRING_VIEW_INIT;
	pipeline_descriptor.label.data = pipeline_label.ptr();
	pipeline_descriptor.label.length = (size_t)pipeline_label.length();

	// pipeline_descriptor.layout
	pipeline_descriptor.layout = shader_info->pipeline_layout;

	// pipeline_descriptor.vertex
	Vector<WGPUConstantEntry> vertex_overrides = _get_specialization_constant_entries(p_specialization_constants, shader_info->vertex_override_layout);
	WGPUVertexState vertex_state = WGPU_VERTEX_STATE_INIT;
	vertex_state.module = shader_info->vertex_shader;
	vertex_state.entryPoint = { "main", WGPU_STRLEN };
	vertex_state.constantCount = (size_t)vertex_overrides.size();
	vertex_state.constants = vertex_overrides.ptr();
	vertex_state.bufferCount = 0;

	// NOTE: I'm not sure dynamic vertex state is supported.
	if (p_vertex_format) {
		VertexFormatInfo *format_info = (VertexFormatInfo *)p_vertex_format.id;
		vertex_state.buffers = format_info->layouts.ptr();
		vertex_state.bufferCount = format_info->layouts.size();
	}

	pipeline_descriptor.vertex = vertex_state;

	// pipeline_descriptor.fragment
	WGPUColorTargetState *targets = ALLOCA_ARRAY(WGPUColorTargetState, p_color_attachments.size());
	size_t targets_count = 0;

	RenderPassInfo *render_pass_info = (RenderPassInfo *)p_render_pass.id;
	uint32_t render_pass_attachments_offset = 0;

	for (uint32_t i = 0; i < p_color_attachments.size(); i++) {
		if (p_color_attachments[i] != ATTACHMENT_UNUSED) {
			const PipelineColorBlendState::Attachment attachment = p_blend_state.attachments[i];
			WGPUBlendState *blend_state = ALLOCA_SINGLE(WGPUBlendState);
			*blend_state = WGPU_BLEND_STATE_INIT;
			blend_state->color.operation = webgpu_blend_operation_from_rd(attachment.color_blend_op);
			blend_state->color.srcFactor = webgpu_blend_factor_from_rd(attachment.src_color_blend_factor);
			blend_state->color.dstFactor = webgpu_blend_factor_from_rd(attachment.dst_color_blend_factor);
			blend_state->alpha.operation = webgpu_blend_operation_from_rd(attachment.alpha_blend_op);
			blend_state->alpha.srcFactor = webgpu_blend_factor_from_rd(attachment.src_alpha_blend_factor);
			blend_state->alpha.dstFactor = webgpu_blend_factor_from_rd(attachment.dst_alpha_blend_factor);

			uint32_t write_mask = WGPUColorWriteMask_None;
			if (attachment.write_r) {
				write_mask |= WGPUColorWriteMask_Red;
			}
			if (attachment.write_g) {
				write_mask |= WGPUColorWriteMask_Green;
			}
			if (attachment.write_b) {
				write_mask |= WGPUColorWriteMask_Blue;
			}
			if (attachment.write_a) {
				write_mask |= WGPUColorWriteMask_Alpha;
			}

			targets[targets_count] = WGPU_COLOR_TARGET_STATE_INIT;
			// TODO: We do not have info on color target format.
			targets[targets_count].format = render_pass_info->attachments[i + render_pass_attachments_offset].format;
			targets[targets_count].blend = attachment.enable_blend ? blend_state : nullptr;
			targets[targets_count].writeMask = write_mask;
			targets_count++;
		} else {
			render_pass_attachments_offset += 1;
		}
	}

	Vector<WGPUConstantEntry> fragment_overrides = _get_specialization_constant_entries(p_specialization_constants, shader_info->fragment_override_layout);
	WGPUFragmentState fragment_state = WGPU_FRAGMENT_STATE_INIT;
	fragment_state.module = shader_info->fragment_shader;
	fragment_state.entryPoint = { "main", WGPU_STRLEN };
	fragment_state.constantCount = (size_t)fragment_overrides.size();
	fragment_state.constants = fragment_overrides.ptr();
	fragment_state.targetCount = p_color_attachments.size() - render_pass_attachments_offset;
	fragment_state.targets = targets;
	pipeline_descriptor.fragment = &fragment_state;

	// pipeline_descriptor.primitive
	// NOTE: We will default to `WGPUPrimitiveTopology_PointList` since not all topologies are supported.
	WGPUPrimitiveTopology topology;
	switch (p_render_primitive) {
		case RenderingDeviceCommons::RENDER_PRIMITIVE_POINTS:
			topology = WGPUPrimitiveTopology_PointList;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_LINES:
			topology = WGPUPrimitiveTopology_LineList;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_LINESTRIPS:
			topology = WGPUPrimitiveTopology_LineStrip;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_TRIANGLES:
			topology = WGPUPrimitiveTopology_TriangleList;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_TRIANGLE_STRIPS:
			topology = WGPUPrimitiveTopology_TriangleStrip;
			break;
		default:
			topology = WGPUPrimitiveTopology_PointList;
			break;
	}

	WGPUFrontFace front_face;
	switch (p_rasterization_state.front_face) {
		case RenderingDeviceCommons::POLYGON_FRONT_FACE_CLOCKWISE:
			front_face = WGPUFrontFace_CW;
			break;
		case RenderingDeviceCommons::POLYGON_FRONT_FACE_COUNTER_CLOCKWISE:
			front_face = WGPUFrontFace_CCW;
			break;
	}

	WGPUCullMode cull_mode = WGPUCullMode_None;
	switch (p_rasterization_state.cull_mode) {
		case RenderingDeviceCommons::POLYGON_CULL_FRONT:
			cull_mode = WGPUCullMode_Front;
			break;
		case RenderingDeviceCommons::POLYGON_CULL_BACK:
			cull_mode = WGPUCullMode_Back;
			break;
		case RenderingDeviceCommons::POLYGON_CULL_DISABLED:
		case RenderingDeviceCommons::POLYGON_CULL_MAX:
			break;
	}

	WGPUPrimitiveState primitive_state = WGPU_PRIMITIVE_STATE_INIT;
	primitive_state.topology = topology;
	// TODO: We need this for primitive restart but currently cannot know the proper value.
	primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;
	primitive_state.frontFace = front_face;
	primitive_state.cullMode = cull_mode;
	// TODO Consider implementing wireframe rendering (required native feature).
	// TODO Consider implementing `p_rasterization_state.enable_depth_clamp` (required native feature).
	pipeline_descriptor.primitive = primitive_state;

	// pipeline_descriptor.depth_stencil
	WGPUDepthStencilState depth_stencil_state;

	const RenderPassAttachmentInfo *depth_attachment = render_pass_info->get_depth_attachment();
	if (depth_attachment) {
		const bool use_stencil = p_depth_stencil_state.enable_stencil &&
				webgpu_texture_format_has_stencil_aspect(depth_attachment->format);
		const bool use_depth = webgpu_texture_format_has_depth_aspect(depth_attachment->format);

		WGPUStencilFaceState stencil_front = WGPU_STENCIL_FACE_STATE_INIT;
		stencil_front.compare = WGPUCompareFunction_Always;
		stencil_front.failOp = WGPUStencilOperation_Keep;
		stencil_front.depthFailOp = WGPUStencilOperation_Keep;
		stencil_front.passOp = WGPUStencilOperation_Keep;
		WGPUStencilFaceState stencil_back = stencil_front;
		if (use_stencil) {
			stencil_front = WGPU_STENCIL_FACE_STATE_INIT;
			stencil_front.compare = webgpu_compare_mode_from_rd(p_depth_stencil_state.front_op.compare);
			stencil_front.failOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.front_op.fail);
			stencil_front.depthFailOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.front_op.depth_fail);
			stencil_front.passOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.front_op.pass);
			stencil_back = WGPU_STENCIL_FACE_STATE_INIT;
			stencil_back.compare = webgpu_compare_mode_from_rd(p_depth_stencil_state.back_op.compare);
			stencil_back.failOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.back_op.fail);
			stencil_back.depthFailOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.back_op.depth_fail);
			stencil_back.passOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.back_op.pass);
		}

		depth_stencil_state = WGPU_DEPTH_STENCIL_STATE_INIT;
		depth_stencil_state.format = depth_attachment->format;
		depth_stencil_state.depthWriteEnabled = (use_depth && p_depth_stencil_state.enable_depth_write) ? WGPUOptionalBool_True : WGPUOptionalBool_False;
		depth_stencil_state.depthCompare = use_depth ? webgpu_compare_mode_from_rd(p_depth_stencil_state.depth_compare_operator) : WGPUCompareFunction_Always;
		depth_stencil_state.stencilFront = stencil_front;
		depth_stencil_state.stencilBack = stencil_back;
		// NOTE: We assume stencil read masks are the same for both front and back.
		// This is how wgpu does it, see https://github.com/gfx-rs/wgpu/blob/6405dcf611a336eb7d3bf9de7b78d7d0b3d3b48d/wgpu-hal/src/vulkan/device.rs#L1778
		depth_stencil_state.stencilReadMask = use_stencil ? p_depth_stencil_state.front_op.compare_mask : 0;
		depth_stencil_state.stencilWriteMask = use_stencil ? p_depth_stencil_state.front_op.write_mask : 0;
		depth_stencil_state.depthBias = (int32_t)p_rasterization_state.depth_bias_constant_factor;
		depth_stencil_state.depthBiasSlopeScale = p_rasterization_state.depth_bias_slope_factor;
		depth_stencil_state.depthBiasClamp = p_rasterization_state.depth_bias_clamp;
		pipeline_descriptor.depthStencil = &depth_stencil_state;
	} else {
		pipeline_descriptor.depthStencil = nullptr;
	}

	// pipeline_descriptor.multisample
	// TODO: Assert that p_format.samples follows this behavior.
	uint32_t sample_count = pow(2, (uint32_t)p_multisample_state.sample_count);
	pipeline_descriptor.multisample = WGPU_MULTISAMPLE_STATE_INIT;
	pipeline_descriptor.multisample.count = sample_count;
	pipeline_descriptor.multisample.mask = p_multisample_state.sample_mask.size() ? *p_multisample_state.sample_mask.ptr() : ~0;
	pipeline_descriptor.multisample.alphaToCoverageEnabled = p_multisample_state.enable_alpha_to_coverage;

	// pipeline_descriptor.multiview
	// TODO: Implement render pipeline multiview.

	WGPURenderPipeline render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_descriptor);
	ERR_FAIL_COND_V(!render_pipeline, PipelineID());

	PipelineInfo *pipeline_info = memnew(PipelineInfo);
	pipeline_info->type = PipelineInfo::PipelineType::RENDER;
	pipeline_info->render_pipeline = render_pipeline;
	pipeline_info->render_pipeline_desc = pipeline_descriptor;
	pipeline_info->shader_id = p_shader;

	return PipelineID(pipeline_info);
}

/*****************/
/**** COMPUTE ****/
/*****************/

// ----- COMMANDS -----

// Binding.
void RenderingDeviceDriverWebGpu::command_bind_compute_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	PipelineInfo *pipeline = (PipelineInfo *)p_pipeline.id;

	command_buffer_info->commands.push_back((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::COMPUTE_SET_PIPELINE,
			.set_pipeline = (PassEncoderCommand::SetPipeline){
					.pipeline_info = pipeline,
			} });
}
void RenderingDeviceDriverWebGpu::command_bind_compute_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	command_buffer_info->has_compute_commands = true;

	uint32_t shift = 0u;
	for (uint32_t i = 0; i < p_set_count; i++) {
		UniformSetInfo *uniform_set_info = (UniformSetInfo *)p_uniform_sets[i].id;
		WGPUBindGroup bind_group = _select_bind_group_from_uniform_set(uniform_set_info, shader_info, p_first_set_index + i);

		PassEncoderCommand cmd = {
			.type = PassEncoderCommand::CommandType::COMPUTE_SET_BIND_GROUP,
			.set_bind_group = (PassEncoderCommand::SetBindGroup){
					.group_index = p_first_set_index + i,
					.bind_group = bind_group,
					.shader_info = shader_info,
			},
		};
		for (const BufferDynamicInfo *dyn : uniform_set_info->dynamic_buffers) {
			uint32_t frame_idx = (p_dynamic_offsets >> shift) & UNIFORM_DYN_MASK;
			shift += UNIFORM_DYN_BITS;
			cmd.dynamic_offsets.push_back(uint32_t(frame_idx * dyn->size));
		}
		command_buffer_info->commands.push_back(cmd);
	}
}

// Dispatching.
void RenderingDeviceDriverWebGpu::command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	command_buffer_info->has_compute_commands = true;
	command_buffer_info->commands.push_back((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::COMPUTE_DISPATCH_WORKGROUPS,
			.compute_dispatch_workgroups = (PassEncoderCommand::ComputeDispatchWorkgroups){
					.workgroup_count_x = p_x_groups,
					.workgroup_count_y = p_y_groups,
					.workgroup_count_z = p_z_groups,
			},
	});
}
void RenderingDeviceDriverWebGpu::command_compute_dispatch_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *buffer_info = (BufferInfo *)p_indirect_buffer.id;

	command_buffer_info->has_compute_commands = true;
	command_buffer_info->commands.push_back((PassEncoderCommand){
			.type = PassEncoderCommand::CommandType::COMPUTE_DISPATCH_WORKGROUPS_INDIRECT,
			.compute_dispatch_workgroups_indirect = (PassEncoderCommand::ComputeDispatchWorkgroupsIndirect){
					.indirect_buffer = buffer_info->buffer,
					.indirect_offset = p_offset,
			},
	});
}

// ----- PIPELINE -----

RenderingDeviceDriver::PipelineID RenderingDeviceDriverWebGpu::compute_pipeline_create(ShaderID p_shader, VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	WEBGPU_ON_MAIN_THREAD(PipelineID, compute_pipeline_create(p_shader, p_specialization_constants));
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	ERR_FAIL_COND_V_MSG(!shader_info->compute_shader, PipelineID(), "Compute pipeline shader null.");

	Vector<WGPUConstantEntry> overrides = _get_specialization_constant_entries(p_specialization_constants, shader_info->compute_override_layout);

	WGPUComputeState programmable_stage_desc = WGPU_COMPUTE_STATE_INIT;
	programmable_stage_desc.module = shader_info->compute_shader;
	programmable_stage_desc.entryPoint = { "main", WGPU_STRLEN };
	programmable_stage_desc.constantCount = (size_t)overrides.size();
	programmable_stage_desc.constants = overrides.ptr();

	CharString pipeline_label = shader_info->shader_name.utf8();

	WGPUComputePipelineDescriptor compute_pipeline_descriptor = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
	compute_pipeline_descriptor.label = WGPU_STRING_VIEW_INIT;
	compute_pipeline_descriptor.label.data = pipeline_label.ptr();
	compute_pipeline_descriptor.label.length = (size_t)pipeline_label.length();
	compute_pipeline_descriptor.layout = shader_info->pipeline_layout;
	compute_pipeline_descriptor.compute = programmable_stage_desc;

	WGPUComputePipeline compute_pipeline = wgpuDeviceCreateComputePipeline(device, &compute_pipeline_descriptor);

	PipelineInfo *pipeline_info = memnew(PipelineInfo);
	pipeline_info->type = PipelineInfo::PipelineType::COMPUTE;
	pipeline_info->compute_pipeline = compute_pipeline;
	pipeline_info->compute_pipeline_desc = compute_pipeline_descriptor;
	pipeline_info->shader_id = p_shader;

	return PipelineID(pipeline_info);
}

/********************/
/**** RAYTRACING ****/
/********************/

// ----- ACCELERATION STRUCTURE -----

// NOTE: Pay attention to WebRTX and [acceleration extension](https://github.com/gfx-rs/wgpu/issues/6762).
// But realistically this is not coming for a long long time.

RenderingDeviceDriver::AccelerationStructureID RenderingDeviceDriverWebGpu::blas_create(VectorView<AccelerationStructureGeometry> p_geometries, BitField<AccelerationStructureFlagBits> p_flags) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
	return AccelerationStructureID();
}

RenderingDeviceDriver::AccelerationStructureID RenderingDeviceDriverWebGpu::tlas_create(uint32_t p_max_instance_count, BitField<AccelerationStructureFlagBits> p_flags) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
	return AccelerationStructureID();
}

void RenderingDeviceDriverWebGpu::acceleration_structure_instance_write(uint8_t *r_driver_instance, const AccelerationStructureInstance &p_instance) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

void RenderingDeviceDriverWebGpu::acceleration_structure_free(AccelerationStructureID p_acceleration_structure) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

uint32_t RenderingDeviceDriverWebGpu::acceleration_structure_get_scratch_size_bytes(AccelerationStructureID p_acceleration_structure) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
	return 0;
}

RenderingDeviceDriver::RaytracingPipelineID RenderingDeviceDriverWebGpu::raytracing_pipeline_create(VectorView<PipelineShader> p_shaders, VectorView<uint32_t> p_raygen_shader_indices, VectorView<uint32_t> p_miss_shader_indices, VectorView<HitGroup> p_hit_groups, uint32_t p_max_trace_recursion_depth, ShaderID p_layout_defining_shader) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
	return RaytracingPipelineID();
}

void RenderingDeviceDriverWebGpu::raytracing_pipeline_free(RaytracingPipelineID p_pipeline) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

bool RenderingDeviceDriverWebGpu::raytracing_pipeline_get_shader_group_handles(RaytracingPipelineID p_pipeline, uint32_t p_group_index_offset, VectorView<uint32_t> p_group_indices, uint8_t *r_data, uint32_t p_data_stride_bytes) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
	return false;
}

void RenderingDeviceDriverWebGpu::command_build_blas(CommandBufferID p_cmd_buffer, AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

void RenderingDeviceDriverWebGpu::command_build_tlas(CommandBufferID p_cmd_buffer, AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer, BufferID p_instance_buffer, uint32_t p_instance_offset, uint32_t p_instance_count) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

void RenderingDeviceDriverWebGpu::command_bind_raytracing_pipeline(CommandBufferID p_cmd_buffer, RaytracingPipelineID p_pipeline) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

void RenderingDeviceDriverWebGpu::command_bind_raytracing_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

void RenderingDeviceDriverWebGpu::command_trace_rays(CommandBufferID p_cmd_buffer, const ShaderBindingTable &p_raygen_sbt, const ShaderBindingTable &p_miss_sbt, const ShaderBindingTable &p_hit_sbt, uint32_t p_width, uint32_t p_height, uint32_t p_depth) {
	CRASH_NOW_MSG("RAYTRACING NOT SUPPORTED");
}

/*****************/
/**** QUERIES ****/
/*****************/

// ----- TIMESTAMP -----

// Basic.
RenderingDeviceDriver::QueryPoolID RenderingDeviceDriverWebGpu::timestamp_query_pool_create(uint32_t p_query_count) {
	// TODO
	return QueryPoolID(1);
}

void RenderingDeviceDriverWebGpu::timestamp_query_pool_free(QueryPoolID p_pool_id) {
	// TODO
}

void RenderingDeviceDriverWebGpu::timestamp_query_pool_get_results(QueryPoolID p_pool_id, uint32_t p_query_count, uint64_t *r_results) {
	// TODO
}

uint64_t RenderingDeviceDriverWebGpu::timestamp_query_result_to_time(uint64_t p_result) {
	// TODO
	return 1;
}

// Commands.
void RenderingDeviceDriverWebGpu::command_timestamp_query_pool_reset(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_query_count) {}
void RenderingDeviceDriverWebGpu::command_timestamp_write(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_index) {}

/****************/
/**** LABELS ****/
/****************/

void RenderingDeviceDriverWebGpu::command_begin_label(CommandBufferID p_cmd_buffer, const char *p_label_name, const Color &p_color) {}
void RenderingDeviceDriverWebGpu::command_end_label(CommandBufferID p_cmd_buffer) {}

/****************/
/**** DEBUG *****/
/****************/

void RenderingDeviceDriverWebGpu::command_insert_breadcrumb(CommandBufferID p_cmd_buffer, uint32_t p_data) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> command_insert_breadcrumb");
}

/********************/
/**** SUBMISSION ****/
/********************/

void RenderingDeviceDriverWebGpu::begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) {}
void RenderingDeviceDriverWebGpu::end_segment() {}

/**************/
/**** MISC ****/
/**************/

void RenderingDeviceDriverWebGpu::set_object_name(ObjectType p_type, ID p_driver_id, const String &p_name) {}
uint64_t RenderingDeviceDriverWebGpu::get_resource_native_handle(DriverResource p_type, ID p_driver_id) {
	// TODO: impl
	return 0;
}

uint64_t RenderingDeviceDriverWebGpu::get_total_memory_used() {
	// TODO: impl
	return 0;
}

uint64_t RenderingDeviceDriverWebGpu::get_lazily_memory_used() {
	// TODO: impl
	return 0;
}

uint64_t RenderingDeviceDriverWebGpu::limit_get(Limit p_limit) {
	WGPULimits limits = (WGPULimits){};

#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
	WGPUNativeLimits extras;
	limits.nextInChain = &extras.chain;
#endif

	if (!subgroups_supported && (p_limit == LIMIT_SUBGROUP_IN_SHADERS || p_limit == LIMIT_SUBGROUP_OPERATIONS)) {
		return 0;
	}

	wgpuDeviceGetLimits(device, &limits);
	return rd_limit_from_webgpu(p_limit, limits);
}

uint64_t RenderingDeviceDriverWebGpu::api_trait_get(ApiTrait p_trait) {
	switch (p_trait) {
		case API_TRAIT_HONORS_PIPELINE_BARRIERS:
			return 0;
		case API_TRAIT_SHADER_CHANGE_INVALIDATION:
			return SHADER_CHANGE_INVALIDATION_ALL_BOUND_UNIFORM_SETS;
		case API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT:
			return 256;
		case API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP:
			return 256;
		case API_TRAIT_SECONDARY_VIEWPORT_SCISSOR:
			return 0;
		case API_TRAIT_USE_GENERAL_IN_COPY_QUEUES:
			return 0;
		case API_TRAIT_BUFFERS_REQUIRE_TRANSITIONS:
			return 0;
		default:
			return RenderingDeviceDriver::api_trait_get(p_trait);
	}
}

bool RenderingDeviceDriverWebGpu::has_feature(Features p_feature) {
	switch (p_feature) {
		// HACK: This lie may just be cover up for a higher level bug.
		// Turning this off, we still create an empty render pass (big no no).
		case SUPPORTS_FRAGMENT_SHADER_WITH_ONLY_SIDE_EFFECTS:
			return true;
		case SUPPORTS_POINT_SIZE:
			return false;
		case SUPPORTS_HDR_OUTPUT:
			return false;
		case SUPPORTS_TEXTURE_SWIZZLE:
			return texture_swizzle_supported;
		default:
			return false;
	}
}

const RenderingDeviceDriver::MultiviewCapabilities &RenderingDeviceDriverWebGpu::get_multiview_capabilities() {
	return multiview_capabilities;
}

const RenderingDeviceDriver::FragmentShadingRateCapabilities &RenderingDeviceDriverWebGpu::get_fragment_shading_rate_capabilities() {
	return fsr_capabilities;
}

const RenderingDeviceDriver::FragmentDensityMapCapabilities &RenderingDeviceDriverWebGpu::get_fragment_density_map_capabilities() {
	return fdm_capabilities;
}

String RenderingDeviceDriverWebGpu::get_api_name() const {
	return "WebGpu";
}

String RenderingDeviceDriverWebGpu::get_api_version() const {
	// TODO: We should compile this in based on the wgpu / dawn version
	return "v29.0.0 (wgpu)";
}

String RenderingDeviceDriverWebGpu::get_pipeline_cache_uuid() const {
	// TODO: impl
	return "webgpu";
}

const RenderingDeviceDriver::Capabilities &RenderingDeviceDriverWebGpu::get_capabilities() const {
	return capabilities;
}

const RenderingShaderContainerFormat &RenderingDeviceDriverWebGpu::get_shader_container_format() const {
	static RenderingShaderContainerFormatWebGpu format;
	return format;
}

RenderingDeviceDriverWebGpu::RenderingDeviceDriverWebGpu(RenderingContextDriverWebGpu *p_context_driver) {
	DEV_ASSERT(p_context_driver != nullptr);

	context_driver = p_context_driver;
}
RenderingDeviceDriverWebGpu::~RenderingDeviceDriverWebGpu() {
	if (queue != nullptr) {
		wgpuQueueRelease(queue);
	}

	if (adapter != nullptr) {
		wgpuAdapterRelease(adapter);
	}

	if (device != nullptr) {
		wgpuDeviceRelease(device);
	}
}
