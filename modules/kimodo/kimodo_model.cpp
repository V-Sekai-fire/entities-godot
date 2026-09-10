/**************************************************************************/
/*  kimodo_model.cpp                                                     */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "kimodo_model.h"

Error KimodoModel::load_from_path(const String &p_path) {
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "KimodoModel::load_from_path is scaffolding; the #thirdparty/kimodo/ import PR provides the implementation.");
}

Ref<Animation> KimodoModel::generate(const String &p_prompt) {
	ERR_FAIL_V_MSG({}, "KimodoModel::generate is scaffolding; the #thirdparty/kimodo/ import PR provides the implementation.");
}

void KimodoModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_from_path", "path"), &KimodoModel::load_from_path);
	ClassDB::bind_method(D_METHOD("generate", "prompt"), &KimodoModel::generate);
}
