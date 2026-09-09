/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "register_types.h"

#include "core/object/class_db.h"

#include "skin_tokens_model.h"

void initialize_skin_tokens_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(SkinTokensModel);
}

void uninitialize_skin_tokens_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
