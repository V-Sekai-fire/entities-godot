/**************************************************************************/
/*  order_independent_transparency_compositor_effect.h                    */
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

#include "order_independent_transparency_buffer_pool.h"
#include "order_independent_transparency_debug_overlay.h"
#include "order_independent_transparency_renderer.h"

#include "scene/resources/compositor.h"

#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

class OrderIndependentTransparencyCompositorEffect : public CompositorEffect {
	GDCLASS(OrderIndependentTransparencyCompositorEffect, CompositorEffect);

public:
	enum RenderTargetFormat {
		RT_FORMAT_LDR,
		RT_FORMAT_HDR,
	};

private:
	float ldr_scale = 0.5;
	bool msaa_enabled = false;
	RenderTargetFormat rt_format = RT_FORMAT_HDR;
	float average_depth_complexity = 4.0;
	bool debug_overlay_enabled = false;

	Ref<OrderIndependentTransparencyRenderer> renderer;
	Ref<OrderIndependentTransparencyBufferPool> buffer_pool;
	Ref<OrderIndependentTransparencyDebugOverlay> debug_overlay;

	Vector<Dictionary> pending_batches;
	bool has_warned_msaa_fallback = false;

	void _on_batches_ready(const Array &p_batches);
	void _bind_relay(Object *p_relay);

protected:
	static void _bind_methods();

	virtual void _prepare_batches();
	virtual void _finalize_composite();

	virtual void _call_render_callback(int p_effect_callback_type, const RenderData *p_render_data) override;

public:
	void set_ldr_scale(float p_scale);
	float get_ldr_scale() const;

	void set_msaa_enabled(bool p_enabled);
	bool get_msaa_enabled() const;

	void set_rt_format(RenderTargetFormat p_format);
	RenderTargetFormat get_rt_format() const;

	void set_average_depth_complexity(float p_value);
	float get_average_depth_complexity() const;

	void set_debug_overlay_enabled(bool p_enabled);
	bool get_debug_overlay_enabled() const;

	OrderIndependentTransparencyCompositorEffect();
	~OrderIndependentTransparencyCompositorEffect();
};

VARIANT_ENUM_CAST(OrderIndependentTransparencyCompositorEffect::RenderTargetFormat);
