/**************************************************************************/
/*  rf_detr_model.h                                                       */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"


// wraps rf-detr-cpp via its rfdetr_capi.{h,cpp} (added in a paired PR to V-Sekai-fire/rf-detr-cpp). Returns detected boxes and keypoints.
// Scaffolded per RFD 2242. Implementation of detect() lands with the
// #thirdparty/rf_detr/ import PR — this class exists so
// ClassDB.class_get_method_list("RFDetrModel") returns non-empty in the
// scaffolding pass.

class RFDetrModel : public Resource {
	GDCLASS(RFDetrModel, Resource);

protected:
	static void _bind_methods();

public:
	Error load_from_path(const String &p_path);
	Array detect(const Ref<Image>& p_image);
};
