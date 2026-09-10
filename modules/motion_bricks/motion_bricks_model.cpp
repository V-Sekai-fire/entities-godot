/**************************************************************************/
/*  motion_bricks_model.cpp                                                     */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "motion_bricks_model.h"

Error MotionBricksModel::load_from_path(const String &p_path) {
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "MotionBricksModel::load_from_path is scaffolding; the #thirdparty/motion_bricks/ import PR provides the implementation.");
}

Ref<Animation> MotionBricksModel::generate(const Ref<class MotionBricksCommand> &p_command) {
	ERR_FAIL_V_MSG({}, "MotionBricksModel::generate is scaffolding; the #thirdparty/motion_bricks/ import PR provides the implementation.");
}

void MotionBricksModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_from_path", "path"), &MotionBricksModel::load_from_path);
	ClassDB::bind_method(D_METHOD("generate", "command"), &MotionBricksModel::generate);
}
