/**************************************************************************/
/*  kimodo_model.h                                                       */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "scene/resources/animation.h"

// wraps kimodo.cpp (kimodo_capi.h C ABI). Generates an animation from a text prompt.
// Scaffolded per RFD 2242. Implementation of generate() lands with the
// #thirdparty/kimodo/ import PR — this class exists so
// ClassDB.class_get_method_list("KimodoModel") returns non-empty in the
// scaffolding pass.

class KimodoModel : public Resource {
	GDCLASS(KimodoModel, Resource);

protected:
	static void _bind_methods();

public:
	Error load_from_path(const String &p_path);
	Ref<Animation> generate(const String& p_prompt);
};
