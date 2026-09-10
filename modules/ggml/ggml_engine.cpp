/**************************************************************************/
/*  ggml_engine.cpp                                                       */
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

#include "ggml_engine.h"

#include "core/object/class_db.h"

// Stub bodies until the follow-up import PR vendors #thirdparty/ggml/ and
// links these against ggml_init / ggml_new_tensor / ggml_free. The class
// registration lands here so ClassDB.class_get_method_list("GgmlEngine")
// returns non-empty from the scaffolding pass alone.

GgmlEngine *GgmlEngine::singleton = nullptr;

GgmlEngine *GgmlEngine::get_singleton() {
	return singleton;
}

GgmlEngine::GgmlEngine() {
	singleton = this;
}

GgmlEngine::~GgmlEngine() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void GgmlEngine::_bind_methods() {
}

Vector<int> GgmlTensor::get_shape() const {
	return Vector<int>();
}

void GgmlTensor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_shape"), &GgmlTensor::get_shape);
}
