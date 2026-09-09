/**************************************************************************/
/*  skin_tokens_model.h                                                       */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"


// wraps skin-tokens-cpp (skintokens.h C ABI). Retargets a source motion onto a humanoid rig described by the GLB bytes.
// Scaffolded per RFD 2242. Implementation of retarget() lands with the
// #thirdparty/skin_tokens/ import PR — this class exists so
// ClassDB.class_get_method_list("SkinTokensModel") returns non-empty in the
// scaffolding pass.

class SkinTokensModel : public Resource {
	GDCLASS(SkinTokensModel, Resource);

protected:
	static void _bind_methods();

public:
	Error load_from_path(const String &p_path);
	Dictionary retarget(const PackedByteArray& p_glb_bytes);
};
