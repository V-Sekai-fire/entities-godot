/**************************************************************************/
/*  rf_detr_model.cpp                                                     */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#include "rf_detr_model.h"

Error RFDetrModel::load_from_path(const String &p_path) {
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "RFDetrModel::load_from_path is scaffolding; the #thirdparty/rf_detr/ import PR provides the implementation.");
}

Array RFDetrModel::detect(const Ref<Image>& p_image) {
	ERR_FAIL_V_MSG({}, "RFDetrModel::detect is scaffolding; the #thirdparty/rf_detr/ import PR provides the implementation.");
}

void RFDetrModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_from_path", "path"), &RFDetrModel::load_from_path);
	ClassDB::bind_method(D_METHOD("detect", "image"), &RFDetrModel::detect);
}
