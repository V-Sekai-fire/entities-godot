/**************************************************************************/
/*  rendering_context_driver_webgpu.cpp                                   */
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

#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu.h"

#include "core/error/error_macros.h"
#include "drivers/webgpu/rendering_device_driver_webgpu.h"
#include "drivers/webgpu/webgpu_platform.h"

static void handle_request_adapter(WGPURequestAdapterStatus p_status,
		WGPUAdapter p_adapter, WGPUStringView p_message,
		void *p_userdata, void *_) {
	String message = String::utf8(p_message.data, p_message.length);
	ERR_FAIL_COND_V_MSG(
			p_status != WGPURequestAdapterStatus_Success, (void)0,
			vformat("Failed to get WebGPU adapter: %s", message));

	WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
	ERR_FAIL_COND_V_MSG(
			wgpuAdapterGetInfo(p_adapter, &info) != WGPUStatus_Success, (void)0,
			"Failed to get WebGPU adapter info.");

	RenderingContextDriver::Device device;
	device.name = String::utf8(info.device.data, info.device.length);
	device.vendor = info.vendorID;
	device.type = (RenderingContextDriver::DeviceType)info.adapterType;

	wgpuAdapterInfoFreeMembers(info);

	RenderingContextDriverWebGpu *context = (RenderingContextDriverWebGpu *)p_userdata;
	context->adapter_push_back(
			p_adapter, device);
}

RenderingContextDriverWebGpu::RenderingContextDriverWebGpu() {
}

RenderingContextDriverWebGpu::~RenderingContextDriverWebGpu() {
	if (instance != nullptr) {
		wgpuInstanceRelease(instance);
	}

	for (WGPUAdapter &adapter : adapters) {
		wgpuAdapterRelease(adapter);
	}
}

Error RenderingContextDriverWebGpu::initialize() {
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	static const WGPUInstanceFeatureName required_features[] = { WGPUInstanceFeatureName_TimedWaitAny };
#endif

#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
	// HACK: Forcing Vulkan works nicely if you need to use lavapipe (CPU vulkan implementation) for debugging
	WGPUInstanceExtras instance_extras = (WGPUInstanceExtras){
		.chain = (WGPUChainedStruct){ .sType = (WGPUSType)WGPUSType_InstanceExtras },
		.backends = WGPUInstanceBackend_Vulkan
	};
#endif

	WGPUInstanceDescriptor instance_descriptor = WGPU_INSTANCE_DESCRIPTOR_INIT;
#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
	instance_descriptor.nextInChain = &instance_extras.chain;
#endif
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	instance_descriptor.requiredFeatureCount = sizeof(required_features) / sizeof(WGPUInstanceFeatureName);
	instance_descriptor.requiredFeatures = required_features;
#endif

	instance = wgpuCreateInstance(&instance_descriptor);
	ERR_FAIL_NULL_V_MSG(instance, ERR_CANT_CREATE, "Failed to create wgpu instance.");

	WGPURequestAdapterOptions adapter_options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
	WGPURequestAdapterCallbackInfo adapter_callback_info = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
	// WaitAnyOnly so the callback fires under wgpuInstanceWaitAny below;
	// see the equivalent switch in RenderingDeviceDriverWebGpu::initialize.
	adapter_callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
	adapter_callback_info.callback = handle_request_adapter;
	adapter_callback_info.userdata1 = this;

	// There is no way to request all adapters, so we just get the high and low power ones.

	const WGPUPowerPreference powers[] = { WGPUPowerPreference_HighPerformance, WGPUPowerPreference_LowPower };
	for (WGPUPowerPreference power : powers) {
		adapter_options.powerPreference = power;
		requesting_power = power;
		WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapter_options, adapter_callback_info);
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
		WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
		wait_info.future = future;
		wait_info.completed = false;
		WGPUWaitStatus wait_status = wgpuInstanceWaitAny(instance, 1, &wait_info, UINT64_MAX);
		ERR_FAIL_COND_V_MSG(wait_status != WGPUWaitStatus_Success, ERR_CANT_CREATE,
				"Failed to wait on WebGPU adapter request.");
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
		(void)future;
		wgpuInstanceProcessEvents(instance);
#endif
	}

	ERR_FAIL_COND_V_MSG(adapters.is_empty(), ERR_CANT_CREATE, "No suitable WebGPU adapter found.");

	return OK;
}

const RenderingContextDriver::Device &RenderingContextDriverWebGpu::device_get(uint32_t p_device_index) const {
	DEV_ASSERT(p_device_index < adapters.size());
	const RenderingContextDriver::Device &driver_device = driver_devices[p_device_index];
	return driver_device;
}

uint32_t RenderingContextDriverWebGpu::device_get_count() const {
	return adapters.size();
}

bool RenderingContextDriverWebGpu::device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const {
	DEV_ASSERT(p_device_index < adapters.size());
	WGPUAdapter adapter = adapters[p_device_index];
	Surface *surface = (Surface *)p_surface;
	WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
	if (wgpuSurfaceGetCapabilities(surface->surface, adapter, &caps) != WGPUStatus_Success) {
		return false;
	}
	bool supported = caps.formatCount != 0;
	wgpuSurfaceCapabilitiesFreeMembers(caps);
	return supported;
}

RenderingDeviceDriver *RenderingContextDriverWebGpu::driver_create() {
	return memnew(RenderingDeviceDriverWebGpu(this));
}

void RenderingContextDriverWebGpu::driver_free(RenderingDeviceDriver *p_driver) {
	memdelete(p_driver);
}

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpu::surface_create(const void *p_platform_data) {
	print_error("Surface creation should not be called on the platform-agnostic version of the driver.");
	return SurfaceID();
}

void RenderingContextDriverWebGpu::surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) {
	Surface *surface = (Surface *)(p_surface);
	surface->width = p_width;
	surface->height = p_height;
	surface->needs_resize = true;
}

void RenderingContextDriverWebGpu::surface_set_vsync_mode(SurfaceID p_surface, DisplayServerEnums::VSyncMode p_vsync_mode) {
	Surface *surface = (Surface *)(p_surface);
	surface->vsync_mode = p_vsync_mode;
	surface->needs_resize = true;
}

DisplayServerEnums::VSyncMode RenderingContextDriverWebGpu::surface_get_vsync_mode(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->vsync_mode;
}

// TODO: HDR
void RenderingContextDriverWebGpu::surface_set_hdr_output_enabled(SurfaceID p_surface, bool p_enabled) {
	// TODO: HDR — route to a WGPUSurfaceConfiguration update with an HDR color-space chain.
}

bool RenderingContextDriverWebGpu::surface_get_hdr_output_enabled(SurfaceID p_surface) const {
	// TODO: HDR — query actual surface capability once the chain is configured.
	return false;
}

void RenderingContextDriverWebGpu::surface_set_hdr_output_reference_luminance(SurfaceID p_surface, float p_reference_luminance) {
	// TODO: HDR — feed into the canvas tone-mapping reference luminance.
}

float RenderingContextDriverWebGpu::surface_get_hdr_output_reference_luminance(SurfaceID p_surface) const {
	// TODO: HDR
	return 0.0f;
}

void RenderingContextDriverWebGpu::surface_set_hdr_output_max_luminance(SurfaceID p_surface, float p_max_luminance) {
	// TODO: HDR — feed into the canvas tone-mapping peak luminance.
}

float RenderingContextDriverWebGpu::surface_get_hdr_output_max_luminance(SurfaceID p_surface) const {
	// TODO: HDR
	return 0.0f;
}

void RenderingContextDriverWebGpu::surface_set_hdr_output_linear_luminance_scale(SurfaceID p_surface, float p_linear_luminance_scale) {
	// TODO: HDR — feed into the linear-light scaling used by the display chain.
}

float RenderingContextDriverWebGpu::surface_get_hdr_output_linear_luminance_scale(SurfaceID p_surface) const {
	// TODO: HDR
	return 0.0f;
}

float RenderingContextDriverWebGpu::surface_get_hdr_output_max_value(SurfaceID p_surface) const {
	// TODO: HDR — return the surface's max representable linear value (>1.0 for HDR).
	// Defensive SDR fallback for now; gated unreachable while _enabled is false.
	return 1.0f;
}

uint32_t RenderingContextDriverWebGpu::surface_get_width(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->width;
}

uint32_t RenderingContextDriverWebGpu::surface_get_height(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->height;
}

void RenderingContextDriverWebGpu::surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) {
	Surface *surface = (Surface *)(p_surface);
	surface->needs_resize = p_needs_resize;
}

bool RenderingContextDriverWebGpu::surface_get_needs_resize(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->needs_resize;
}

void RenderingContextDriverWebGpu::surface_destroy(SurfaceID p_surface) {
	Surface *surface = (Surface *)(p_surface);
	wgpuSurfaceRelease(surface->surface);
	memdelete(surface);
}
bool RenderingContextDriverWebGpu::is_debug_utils_enabled() const {
	// Although there is a flag to enable validation in WGPU, the spec doesn't support this.
	// See: https://docs.rs/wgpu/latest/wgpu/struct.InstanceFlags.html#associatedconstant.DEBUG
	return false;
}

WGPUInstance RenderingContextDriverWebGpu::instance_get() const {
	return instance;
}

WGPUAdapter RenderingContextDriverWebGpu::adapter_get(uint32_t p_adapter_index) const {
	DEV_ASSERT(p_adapter_index < adapters.size());
	WGPUAdapter adapter = adapters[p_adapter_index];
	return adapter;
}

void RenderingContextDriverWebGpu::adapter_push_back(WGPUAdapter p_adapter, Device p_device) {
	adapters.push_back(p_adapter);
	adapter_power.push_back(requesting_power);
	adapter_consumed.push_back(false);
	driver_devices.push_back(p_device);
}

static void handle_request_fresh_adapter(WGPURequestAdapterStatus p_status,
		WGPUAdapter p_adapter, WGPUStringView p_message, void *p_userdata, void *_) {
	ERR_FAIL_COND_MSG(p_status != WGPURequestAdapterStatus_Success,
			vformat("Failed to get WebGPU adapter: %s", String::utf8(p_message.data, p_message.length)));
	*(WGPUAdapter *)p_userdata = p_adapter;
}

WGPUAdapter RenderingContextDriverWebGpu::adapter_acquire(uint32_t p_adapter_index) {
	ERR_FAIL_UNSIGNED_INDEX_V(p_adapter_index, adapters.size(), nullptr);
	if (!adapter_consumed[p_adapter_index]) {
		adapter_consumed[p_adapter_index] = true;
		wgpuAdapterAddRef(adapters[p_adapter_index]);
		return adapters[p_adapter_index];
	}
	// The cached adapter already made a device (a local RenderingDevice): request another.
	WGPUAdapter fresh = nullptr;
	WGPURequestAdapterOptions adapter_options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
	adapter_options.powerPreference = adapter_power[p_adapter_index];
	WGPURequestAdapterCallbackInfo callback_info = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
	callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
	callback_info.callback = handle_request_fresh_adapter;
	callback_info.userdata1 = &fresh;
	WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapter_options, callback_info);
#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
	WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
	wait_info.future = future;
	wait_info.completed = false;
	ERR_FAIL_COND_V_MSG(wgpuInstanceWaitAny(instance, 1, &wait_info, UINT64_MAX) != WGPUWaitStatus_Success, nullptr,
			"Failed to wait on WebGPU adapter request.");
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
	(void)future;
	wgpuInstanceProcessEvents(instance);
#endif
	return fresh;
}

void RenderingContextDriverWebGpu::Surface::configure(WGPUAdapter p_adapter, WGPUDevice p_device) {
	WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
	wgpuSurfaceGetCapabilities(surface, p_adapter, &capabilities);

	// Godot only supports these swapchain formats.
	for (uint32_t i = 0; i < capabilities.formatCount; i++) {
		WGPUTextureFormat format = capabilities.formats[i];
		switch (format) {
			case WGPUTextureFormat_BGRA8Unorm:
				this->format = format;
				this->rd_format = RDD::DATA_FORMAT_B8G8R8A8_UNORM;
				break;
			case WGPUTextureFormat_RGBA8Unorm:
				this->format = format;
				this->rd_format = RDD::DATA_FORMAT_R8G8B8A8_UNORM;
				break;
			default:
				break;
		}
	}

	// TODO: Complete full surface config.
	WGPUSurfaceConfiguration surface_config = WGPU_SURFACE_CONFIGURATION_INIT;
	surface_config.device = p_device;
	surface_config.format = this->format;
	surface_config.usage = WGPUTextureUsage_RenderAttachment;
	surface_config.width = this->width;
	surface_config.height = this->height;

	wgpuSurfaceConfigure(this->surface, &surface_config);
}

#endif // WEBGPU_ENABLED
