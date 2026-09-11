/**************************************************************************/
/*  order_independent_transparency_settings.cpp                           */
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

#include "order_independent_transparency_settings.h"

#include "core/config/project_settings.h"

namespace OrderIndependentTransparencySettings {

void register_project_settings() {
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, "rendering/order_independent_transparency/enabled"), false);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/order_independent_transparency/ldr_scale", PROPERTY_HINT_RANGE, "0.0,4.0,0.01"), 0.5);
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, "rendering/order_independent_transparency/msaa_enabled"), false);
	GLOBAL_DEF(PropertyInfo(Variant::INT, "rendering/order_independent_transparency/rt_format", PROPERTY_HINT_ENUM, "LDR,HDR"), 1);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/order_independent_transparency/average_depth_complexity", PROPERTY_HINT_RANGE, "0.5,32.0,0.1"), 4.0);
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, "rendering/order_independent_transparency/debug_overlay"), false);
}

bool get_enabled() {
	return GLOBAL_GET("rendering/order_independent_transparency/enabled");
}

float get_ldr_scale() {
	return GLOBAL_GET("rendering/order_independent_transparency/ldr_scale");
}

bool get_msaa_enabled() {
	return GLOBAL_GET("rendering/order_independent_transparency/msaa_enabled");
}

int get_rt_format() {
	return GLOBAL_GET("rendering/order_independent_transparency/rt_format");
}

float get_average_depth_complexity() {
	return GLOBAL_GET("rendering/order_independent_transparency/average_depth_complexity");
}

bool get_debug_overlay_enabled() {
	return GLOBAL_GET("rendering/order_independent_transparency/debug_overlay");
}

} // namespace OrderIndependentTransparencySettings
