/**************************************************************************/
/*  oit_settings.cpp                                                      */
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

#include "oit_settings.h"

#include "core/config/project_settings.h"

void oit_register_project_settings() {
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, "rendering/oit/enabled"), false);
	GLOBAL_DEF(PropertyInfo(Variant::INT, "rendering/oit/slice_count", PROPERTY_HINT_ENUM, "64,128,256,512"), 128);
	GLOBAL_DEF(PropertyInfo(Variant::VECTOR2I, "rendering/oit/tile_size"), Vector2i(6, 6));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/oit/near_plane", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), 0.1);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/oit/far_plane", PROPERTY_HINT_RANGE, "1.0,10000.0,1.0"), 500.0);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/oit/linearization_factor", PROPERTY_HINT_RANGE, "0.001,100000.0,0.001,exp"), 1000.0);
}
