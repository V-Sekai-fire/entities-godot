/**************************************************************************/
/*  pixal3d_model.h                                                       */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "scene/resources/mesh.h"

// wraps pixal3d.cpp (trellis2_capi.h C ABI). Generates a textured mesh from a single reference image.
// Scaffolded per RFD 2242. Implementation of image_to_mesh() lands with the
// #thirdparty/pixal3d/ import PR — this class exists so
// ClassDB.class_get_method_list("Pixal3DModel") returns non-empty in the
// scaffolding pass.

class Pixal3DModel : public Resource {
	GDCLASS(Pixal3DModel, Resource);

protected:
	static void _bind_methods();

public:
	Error load_from_path(const String &p_path);
	Ref<ArrayMesh> image_to_mesh(const Ref<Image> &p_image);
};
