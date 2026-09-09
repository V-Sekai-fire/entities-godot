/**************************************************************************/
/*  pixal3d_model.cpp                                                     */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "pixal3d_model.h"

Error Pixal3DModel::load_from_path(const String &p_path) {
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "Pixal3DModel::load_from_path is scaffolding; the #thirdparty/pixal3d/ import PR provides the implementation.");
}

Ref<ArrayMesh> Pixal3DModel::image_to_mesh(const Ref<Image>& p_image) {
	ERR_FAIL_V_MSG({}, "Pixal3DModel::image_to_mesh is scaffolding; the #thirdparty/pixal3d/ import PR provides the implementation.");
}

void Pixal3DModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_from_path", "path"), &Pixal3DModel::load_from_path);
	ClassDB::bind_method(D_METHOD("image_to_mesh", "image"), &Pixal3DModel::image_to_mesh);
}
