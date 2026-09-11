/**************************************************************************/
/*  order_independent_transparency_compositor_effect.cpp                  */
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

#include "order_independent_transparency_compositor_effect.h"

#include "order_independent_transparency_settings.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

OrderIndependentTransparencyCompositorEffect::OrderIndependentTransparencyCompositorEffect() {
	if (get_effect_callback_type() != EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT) {
		set_effect_callback_type(EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT);
	}
	set_access_resolved_depth(true);

	set_enabled(OrderIndependentTransparencySettings::get_enabled());
	ldr_scale = OrderIndependentTransparencySettings::get_ldr_scale();
	msaa_enabled = OrderIndependentTransparencySettings::get_msaa_enabled();
	rt_format = static_cast<RenderTargetFormat>(CLAMP(OrderIndependentTransparencySettings::get_rt_format(), 0, 1));
	average_depth_complexity = OrderIndependentTransparencySettings::get_average_depth_complexity();
	debug_overlay_enabled = OrderIndependentTransparencySettings::get_debug_overlay_enabled();

	renderer.instantiate();
	buffer_pool.instantiate();
	if (debug_overlay_enabled) {
		debug_overlay.instantiate();
	}
}

OrderIndependentTransparencyCompositorEffect::~OrderIndependentTransparencyCompositorEffect() {
}

void OrderIndependentTransparencyCompositorEffect::_call_render_callback(int p_effect_callback_type, const RenderData *p_render_data) {
	if (!get_enabled()) {
		return;
	}
	(void)p_effect_callback_type;
	(void)p_render_data;
	print_verbose("OrderIndependentTransparencyCompositorEffect::_call_render_callback stub");
}

void OrderIndependentTransparencyCompositorEffect::_prepare_batches() {
}

void OrderIndependentTransparencyCompositorEffect::_finalize_composite() {
}

void OrderIndependentTransparencyCompositorEffect::_on_batches_ready(const Array &p_batches) {
	pending_batches.clear();
	pending_batches.resize(p_batches.size());
	for (int i = 0; i < p_batches.size(); i++) {
		pending_batches.write[i] = p_batches[i];
	}
}

void OrderIndependentTransparencyCompositorEffect::_bind_relay(Object *p_relay) {
	if (p_relay) {
		p_relay->connect(SNAME("transparency_batches_ready"), callable_mp(this, &OrderIndependentTransparencyCompositorEffect::_on_batches_ready));
	}
}

void OrderIndependentTransparencyCompositorEffect::set_ldr_scale(float p_scale) {
	ldr_scale = p_scale;
}

float OrderIndependentTransparencyCompositorEffect::get_ldr_scale() const {
	return ldr_scale;
}

void OrderIndependentTransparencyCompositorEffect::set_msaa_enabled(bool p_enabled) {
	msaa_enabled = p_enabled;
}

bool OrderIndependentTransparencyCompositorEffect::get_msaa_enabled() const {
	return msaa_enabled;
}

void OrderIndependentTransparencyCompositorEffect::set_rt_format(RenderTargetFormat p_format) {
	rt_format = p_format;
}

OrderIndependentTransparencyCompositorEffect::RenderTargetFormat OrderIndependentTransparencyCompositorEffect::get_rt_format() const {
	return rt_format;
}

void OrderIndependentTransparencyCompositorEffect::set_average_depth_complexity(float p_value) {
	average_depth_complexity = p_value;
}

float OrderIndependentTransparencyCompositorEffect::get_average_depth_complexity() const {
	return average_depth_complexity;
}

void OrderIndependentTransparencyCompositorEffect::set_debug_overlay_enabled(bool p_enabled) {
	debug_overlay_enabled = p_enabled;
	if (debug_overlay_enabled && debug_overlay.is_null()) {
		debug_overlay.instantiate();
	} else if (!debug_overlay_enabled) {
		debug_overlay.unref();
	}
}

bool OrderIndependentTransparencyCompositorEffect::get_debug_overlay_enabled() const {
	return debug_overlay_enabled;
}

void OrderIndependentTransparencyCompositorEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_ldr_scale", "scale"), &OrderIndependentTransparencyCompositorEffect::set_ldr_scale);
	ClassDB::bind_method(D_METHOD("get_ldr_scale"), &OrderIndependentTransparencyCompositorEffect::get_ldr_scale);

	ClassDB::bind_method(D_METHOD("set_msaa_enabled", "enabled"), &OrderIndependentTransparencyCompositorEffect::set_msaa_enabled);
	ClassDB::bind_method(D_METHOD("get_msaa_enabled"), &OrderIndependentTransparencyCompositorEffect::get_msaa_enabled);

	ClassDB::bind_method(D_METHOD("set_rt_format", "format"), &OrderIndependentTransparencyCompositorEffect::set_rt_format);
	ClassDB::bind_method(D_METHOD("get_rt_format"), &OrderIndependentTransparencyCompositorEffect::get_rt_format);

	ClassDB::bind_method(D_METHOD("set_average_depth_complexity", "value"), &OrderIndependentTransparencyCompositorEffect::set_average_depth_complexity);
	ClassDB::bind_method(D_METHOD("get_average_depth_complexity"), &OrderIndependentTransparencyCompositorEffect::get_average_depth_complexity);

	ClassDB::bind_method(D_METHOD("set_debug_overlay_enabled", "enabled"), &OrderIndependentTransparencyCompositorEffect::set_debug_overlay_enabled);
	ClassDB::bind_method(D_METHOD("get_debug_overlay_enabled"), &OrderIndependentTransparencyCompositorEffect::get_debug_overlay_enabled);

	ClassDB::bind_method(D_METHOD("_on_batches_ready", "batches"), &OrderIndependentTransparencyCompositorEffect::_on_batches_ready);
	ClassDB::bind_method(D_METHOD("_bind_relay", "relay"), &OrderIndependentTransparencyCompositorEffect::_bind_relay);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ldr_scale", PROPERTY_HINT_RANGE, "0.0,4.0,0.01"), "set_ldr_scale", "get_ldr_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "msaa_enabled"), "set_msaa_enabled", "get_msaa_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rt_format", PROPERTY_HINT_ENUM, "LDR,HDR"), "set_rt_format", "get_rt_format");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "average_depth_complexity", PROPERTY_HINT_RANGE, "0.5,32.0,0.1"), "set_average_depth_complexity", "get_average_depth_complexity");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_overlay_enabled"), "set_debug_overlay_enabled", "get_debug_overlay_enabled");

	ADD_SIGNAL(MethodInfo("order_independent_transparency_stats",
			PropertyInfo(Variant::DICTIONARY, "stats")));

	BIND_ENUM_CONSTANT(RT_FORMAT_LDR);
	BIND_ENUM_CONSTANT(RT_FORMAT_HDR);
}
