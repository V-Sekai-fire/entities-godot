/**************************************************************************/
/*  skin_tokens_model.cpp                                                     */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "skin_tokens_model.h"

Error SkinTokensModel::load_from_path(const String &p_path) {
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "SkinTokensModel::load_from_path is scaffolding; the #thirdparty/skin_tokens/ import PR provides the implementation.");
}

Dictionary SkinTokensModel::retarget(const PackedByteArray& p_glb_bytes) {
	ERR_FAIL_V_MSG({}, "SkinTokensModel::retarget is scaffolding; the #thirdparty/skin_tokens/ import PR provides the implementation.");
}

void SkinTokensModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_from_path", "path"), &SkinTokensModel::load_from_path);
	ClassDB::bind_method(D_METHOD("retarget", "glb_bytes"), &SkinTokensModel::retarget);
}
