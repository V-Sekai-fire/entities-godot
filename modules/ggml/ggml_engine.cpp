/**************************************************************************/
/*  ggml_engine.cpp                                                       */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "ggml_engine.h"

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
