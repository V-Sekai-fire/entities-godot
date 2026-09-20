/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/**************************************************************************/

#include "register_types.h"

#include "pixel_stream_client.h"
#include "pixel_stream_server.h"
#include "core/object/class_db.h"

void initialize_pixel_stream_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(PixelStreamServer);
	GDREGISTER_CLASS(PixelStreamClient);
}

void uninitialize_pixel_stream_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
