/**************************************************************************/
/*  motion_bricks_model.h                                                       */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "scene/resources/animation.h"

// wraps motion-bricks-cpp (motionbricks.h C ABI). Generates an animation from a movement/facing/target command.
// Scaffolded per RFD 2242. Implementation of generate() lands with the
// #thirdparty/motion_bricks/ import PR — this class exists so
// ClassDB.class_get_method_list("MotionBricksModel") returns non-empty in the
// scaffolding pass.

class MotionBricksModel : public Resource {
	GDCLASS(MotionBricksModel, Resource);

protected:
	static void _bind_methods();

public:
	Error load_from_path(const String &p_path);
	Ref<Animation> generate(const Ref<class MotionBricksCommand>& p_command);
};
