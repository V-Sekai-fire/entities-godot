/**************************************************************************/
/*  rendering_context_driver_webgpu.h                                     */
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
#include "servers/rendering/rendering_context_driver.h"
#include "servers/rendering/rendering_device_driver.h"

class RenderingContextDriverWebGpu : public RenderingContextDriver {
private:
	WGPUInstance instance = nullptr;
	TightLocalVector<WGPUAdapter> adapters;
	TightLocalVector<WGPUPowerPreference> adapter_power;
	TightLocalVector<bool> adapter_consumed;
	TightLocalVector<Device> driver_devices;
	WGPUPowerPreference requesting_power = WGPUPowerPreference_Undefined;

public:
	virtual Error initialize() override;
	virtual const RenderingContextDriver::Device &device_get(uint32_t p_device_index) const override;
	virtual uint32_t device_get_count() const override;
	virtual bool device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const override;
	virtual RenderingDeviceDriver *driver_create() override;
	virtual void driver_free(RenderingDeviceDriver *p_driver) override;
	virtual SurfaceID surface_create(const void *p_platform_data) override;
	virtual void surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) override;
	virtual void surface_set_vsync_mode(SurfaceID p_surface, DisplayServerEnums::VSyncMode p_vsync_mode) override;
	virtual DisplayServerEnums::VSyncMode surface_get_vsync_mode(SurfaceID p_surface) const override final;
	virtual void surface_set_hdr_output_enabled(SurfaceID p_surface, bool p_enabled) override final;
	virtual bool surface_get_hdr_output_enabled(SurfaceID p_surface) const override final;
	virtual void surface_set_hdr_output_reference_luminance(SurfaceID p_surface, float p_reference_luminance) override final;
	virtual float surface_get_hdr_output_reference_luminance(SurfaceID p_surface) const override final;
	virtual void surface_set_hdr_output_max_luminance(SurfaceID p_surface, float p_max_luminance) override final;
	virtual float surface_get_hdr_output_max_luminance(SurfaceID p_surface) const override;
	virtual void surface_set_hdr_output_linear_luminance_scale(SurfaceID p_surface, float p_linear_luminance_scale) override final;
	virtual float surface_get_hdr_output_linear_luminance_scale(SurfaceID p_surface) const override final;
	virtual float surface_get_hdr_output_max_value(SurfaceID p_surface) const override final;
	virtual uint32_t surface_get_width(SurfaceID p_surface) const override;
	virtual uint32_t surface_get_height(SurfaceID p_surface) const override;
	virtual void surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) override;
	virtual bool surface_get_needs_resize(SurfaceID p_surface) const override;
	virtual void surface_destroy(SurfaceID p_surface) override;
	virtual bool is_debug_utils_enabled() const override;

	RenderingContextDriverWebGpu();
	virtual ~RenderingContextDriverWebGpu() override;

	class Surface {
	public:
		WGPUSurface surface = nullptr;
		WGPUTextureFormat format = WGPUTextureFormat_Undefined;
		RDD::DataFormat rd_format = RDD::DataFormat::DATA_FORMAT_MAX;
		uint32_t width = 0;
		uint32_t height = 0;
		DisplayServerEnums::VSyncMode vsync_mode = DisplayServerEnums::VSYNC_ENABLED;
		bool needs_resize = false;

		void configure(WGPUAdapter p_adapter, WGPUDevice p_device);
	};

	WGPUInstance instance_get() const;
	WGPUAdapter adapter_get(uint32_t p_adapter_index) const;
	// A WebGPU adapter creates one device, so each device driver takes its own reference.
	WGPUAdapter adapter_acquire(uint32_t p_adapter_index);
	void adapter_push_back(WGPUAdapter p_adapter, Device p_device);
};
