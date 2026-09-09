/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "register_types.h"

#include "core/object/class_db.h"

#include "ggml_engine.h"

void initialize_ggml_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}
	GDREGISTER_CLASS(GgmlEngine);
	GDREGISTER_CLASS(GgmlTensor);
}

void uninitialize_ggml_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}
}
