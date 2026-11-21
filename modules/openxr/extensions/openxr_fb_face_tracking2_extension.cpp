/**************************************************************************/
/*  openxr_fb_face_tracking2_extension.cpp                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/


#include "openxr_fb_face_tracking2_extension.h"

#include "../openxr_api.h"

#include "core/config/project_settings.h"
#include "core/string/print_string.h"
#include "servers/xr/xr_server.h"

#include <openxr/openxr.h>

namespace {

struct FaceMapping {
	XrFaceExpression2FB source;
	XRFaceTracker::BlendShapeEntry target[2];
};

// Each source weight feeds up to two Unified Expressions shapes; FT_MAX means no shape.
const FaceMapping face_mapping[] = {
	{ XR_FACE_EXPRESSION2_BROW_LOWERER_L_FB, XRFaceTracker::FT_BROW_LOWERER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_BROW_LOWERER_R_FB, XRFaceTracker::FT_BROW_LOWERER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHEEK_PUFF_L_FB, XRFaceTracker::FT_CHEEK_PUFF_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHEEK_PUFF_R_FB, XRFaceTracker::FT_CHEEK_PUFF_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHEEK_RAISER_L_FB, XRFaceTracker::FT_CHEEK_SQUINT_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHEEK_RAISER_R_FB, XRFaceTracker::FT_CHEEK_SQUINT_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHEEK_SUCK_L_FB, XRFaceTracker::FT_CHEEK_SUCK_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHEEK_SUCK_R_FB, XRFaceTracker::FT_CHEEK_SUCK_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHIN_RAISER_B_FB, XRFaceTracker::FT_MOUTH_RAISER_LOWER, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_CHIN_RAISER_T_FB, XRFaceTracker::FT_MOUTH_RAISER_UPPER, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_DIMPLER_L_FB, XRFaceTracker::FT_MOUTH_DIMPLE_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_DIMPLER_R_FB, XRFaceTracker::FT_MOUTH_DIMPLE_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_CLOSED_L_FB, XRFaceTracker::FT_EYE_CLOSED_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_CLOSED_R_FB, XRFaceTracker::FT_EYE_CLOSED_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_DOWN_L_FB, XRFaceTracker::FT_EYE_LOOK_DOWN_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_DOWN_R_FB, XRFaceTracker::FT_EYE_LOOK_DOWN_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_LEFT_L_FB, XRFaceTracker::FT_EYE_LOOK_OUT_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_LEFT_R_FB, XRFaceTracker::FT_EYE_LOOK_IN_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_RIGHT_L_FB, XRFaceTracker::FT_EYE_LOOK_IN_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_RIGHT_R_FB, XRFaceTracker::FT_EYE_LOOK_OUT_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_UP_L_FB, XRFaceTracker::FT_EYE_LOOK_UP_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_EYES_LOOK_UP_R_FB, XRFaceTracker::FT_EYE_LOOK_UP_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_INNER_BROW_RAISER_L_FB, XRFaceTracker::FT_BROW_INNER_UP_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_INNER_BROW_RAISER_R_FB, XRFaceTracker::FT_BROW_INNER_UP_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_JAW_DROP_FB, XRFaceTracker::FT_JAW_OPEN, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_JAW_SIDEWAYS_LEFT_FB, XRFaceTracker::FT_JAW_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_JAW_SIDEWAYS_RIGHT_FB, XRFaceTracker::FT_JAW_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_JAW_THRUST_FB, XRFaceTracker::FT_JAW_FORWARD, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LID_TIGHTENER_L_FB, XRFaceTracker::FT_EYE_SQUINT_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LID_TIGHTENER_R_FB, XRFaceTracker::FT_EYE_SQUINT_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_CORNER_DEPRESSOR_L_FB, XRFaceTracker::FT_MOUTH_FROWN_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_CORNER_DEPRESSOR_R_FB, XRFaceTracker::FT_MOUTH_FROWN_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_CORNER_PULLER_L_FB, XRFaceTracker::FT_MOUTH_CORNER_PULL_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_CORNER_PULLER_R_FB, XRFaceTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_FUNNELER_LB_FB, XRFaceTracker::FT_LIP_FUNNEL_LOWER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_FUNNELER_LT_FB, XRFaceTracker::FT_LIP_FUNNEL_UPPER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_FUNNELER_RB_FB, XRFaceTracker::FT_LIP_FUNNEL_LOWER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_FUNNELER_RT_FB, XRFaceTracker::FT_LIP_FUNNEL_UPPER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_PRESSOR_L_FB, XRFaceTracker::FT_MOUTH_PRESS_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_PRESSOR_R_FB, XRFaceTracker::FT_MOUTH_PRESS_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_PUCKER_L_FB, XRFaceTracker::FT_LIP_PUCKER_UPPER_LEFT, XRFaceTracker::FT_LIP_PUCKER_LOWER_LEFT },
	{ XR_FACE_EXPRESSION2_LIP_PUCKER_R_FB, XRFaceTracker::FT_LIP_PUCKER_UPPER_RIGHT, XRFaceTracker::FT_LIP_PUCKER_LOWER_RIGHT },
	{ XR_FACE_EXPRESSION2_LIP_STRETCHER_L_FB, XRFaceTracker::FT_MOUTH_STRETCH_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_STRETCHER_R_FB, XRFaceTracker::FT_MOUTH_STRETCH_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_SUCK_LB_FB, XRFaceTracker::FT_LIP_SUCK_LOWER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_SUCK_LT_FB, XRFaceTracker::FT_LIP_SUCK_UPPER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_SUCK_RB_FB, XRFaceTracker::FT_LIP_SUCK_LOWER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_SUCK_RT_FB, XRFaceTracker::FT_LIP_SUCK_UPPER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_TIGHTENER_L_FB, XRFaceTracker::FT_MOUTH_TIGHTENER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIP_TIGHTENER_R_FB, XRFaceTracker::FT_MOUTH_TIGHTENER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LIPS_TOWARD_FB, XRFaceTracker::FT_MOUTH_CLOSED, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LOWER_LIP_DEPRESSOR_L_FB, XRFaceTracker::FT_MOUTH_LOWER_DOWN_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_LOWER_LIP_DEPRESSOR_R_FB, XRFaceTracker::FT_MOUTH_LOWER_DOWN_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_MOUTH_LEFT_FB, XRFaceTracker::FT_MOUTH_UPPER_LEFT, XRFaceTracker::FT_MOUTH_LOWER_LEFT },
	{ XR_FACE_EXPRESSION2_MOUTH_RIGHT_FB, XRFaceTracker::FT_MOUTH_UPPER_RIGHT, XRFaceTracker::FT_MOUTH_LOWER_RIGHT },
	{ XR_FACE_EXPRESSION2_NOSE_WRINKLER_L_FB, XRFaceTracker::FT_NOSE_SNEER_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_NOSE_WRINKLER_R_FB, XRFaceTracker::FT_NOSE_SNEER_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_OUTER_BROW_RAISER_L_FB, XRFaceTracker::FT_BROW_OUTER_UP_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_OUTER_BROW_RAISER_R_FB, XRFaceTracker::FT_BROW_OUTER_UP_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_UPPER_LID_RAISER_L_FB, XRFaceTracker::FT_EYE_WIDE_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_UPPER_LID_RAISER_R_FB, XRFaceTracker::FT_EYE_WIDE_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_UPPER_LIP_RAISER_L_FB, XRFaceTracker::FT_MOUTH_UPPER_UP_LEFT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_UPPER_LIP_RAISER_R_FB, XRFaceTracker::FT_MOUTH_UPPER_UP_RIGHT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_TIP_INTERDENTAL_FB, XRFaceTracker::FT_MAX, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_TIP_ALVEOLAR_FB, XRFaceTracker::FT_MAX, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_FRONT_DORSAL_PALATE_FB, XRFaceTracker::FT_MAX, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_MID_DORSAL_PALATE_FB, XRFaceTracker::FT_MAX, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_BACK_DORSAL_VELAR_FB, XRFaceTracker::FT_MAX, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_OUT_FB, XRFaceTracker::FT_TONGUE_OUT, XRFaceTracker::FT_MAX },
	{ XR_FACE_EXPRESSION2_TONGUE_RETREAT_FB, XRFaceTracker::FT_MAX, XRFaceTracker::FT_MAX },
};

static_assert(sizeof(face_mapping) / sizeof(face_mapping[0]) == XR_FACE_EXPRESSION2_COUNT_FB, "Every XR_FB_face_tracking2 expression needs a mapping row.");

} // namespace

OpenXRFBFaceTracking2Extension *OpenXRFBFaceTracking2Extension::singleton = nullptr;

OpenXRFBFaceTracking2Extension *OpenXRFBFaceTracking2Extension::get_singleton() {
	return singleton;
}

OpenXRFBFaceTracking2Extension::OpenXRFBFaceTracking2Extension() {
	singleton = this;
}

OpenXRFBFaceTracking2Extension::~OpenXRFBFaceTracking2Extension() {
	singleton = nullptr;
}

HashMap<String, bool *> OpenXRFBFaceTracking2Extension::get_requested_extensions(XrVersion p_version) {
	HashMap<String, bool *> request_extensions;
	if (GLOBAL_GET("xr/openxr/extensions/face_tracking")) {
		request_extensions[XR_FB_FACE_TRACKING2_EXTENSION_NAME] = &available;
	}
	return request_extensions;
}

void *OpenXRFBFaceTracking2Extension::set_system_properties_and_get_next_pointer(void *p_next_pointer) {
	if (!available) {
		return p_next_pointer;
	}

	properties.type = XR_TYPE_SYSTEM_FACE_TRACKING_PROPERTIES2_FB;
	properties.next = p_next_pointer;
	properties.supportsVisualFaceTracking = XR_FALSE;
	properties.supportsAudioFaceTracking = XR_FALSE;

	return &properties;
}

void OpenXRFBFaceTracking2Extension::on_instance_created(const XrInstance p_instance) {
	if (available) {
		EXT_INIT_XR_FUNC(xrCreateFaceTracker2FB);
		EXT_INIT_XR_FUNC(xrDestroyFaceTracker2FB);
		EXT_INIT_XR_FUNC(xrGetFaceExpressionWeights2FB);
		available = xrCreateFaceTracker2FB_ptr && xrDestroyFaceTracker2FB_ptr && xrGetFaceExpressionWeights2FB_ptr;
	}
}

void OpenXRFBFaceTracking2Extension::on_instance_destroyed() {
	xrCreateFaceTracker2FB_ptr = nullptr;
	xrDestroyFaceTracker2FB_ptr = nullptr;
	xrGetFaceExpressionWeights2FB_ptr = nullptr;
}

bool OpenXRFBFaceTracking2Extension::is_active() const {
	return available && (properties.supportsVisualFaceTracking || properties.supportsAudioFaceTracking);
}

void OpenXRFBFaceTracking2Extension::on_session_created(const XrSession p_session) {
	if (!is_active()) {
		return;
	}

	XrFaceTrackingDataSource2FB data_source = properties.supportsVisualFaceTracking ? XR_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB : XR_FACE_TRACKING_DATA_SOURCE2_AUDIO_FB;
	XrFaceTrackerCreateInfo2FB create_info = {
		XR_TYPE_FACE_TRACKER_CREATE_INFO2_FB, // type
		nullptr, // next
		XR_FACE_EXPRESSION_SET2_DEFAULT_FB, // faceExpressionSet
		1, // requestedDataSourceCount
		&data_source, // requestedDataSources
	};

	XrResult result = xrCreateFaceTracker2FB(p_session, &create_info, &face_tracker);
	if (XR_FAILED(result)) {
		WARN_PRINT("OpenXR: Failed to create the face tracker [" + OpenXRAPI::get_singleton()->get_error_string(result) + "]");
		face_tracker = XR_NULL_HANDLE;
		return;
	}

	tracker.instantiate();
	tracker->set_tracker_name("/user/face_tracker");
	XRServer::get_singleton()->add_tracker(tracker);
}

void OpenXRFBFaceTracking2Extension::on_session_destroyed() {
	if (tracker.is_valid()) {
		XRServer::get_singleton()->remove_tracker(tracker);
		tracker.unref();
	}
	if (face_tracker != XR_NULL_HANDLE) {
		xrDestroyFaceTracker2FB(face_tracker);
		face_tracker = XR_NULL_HANDLE;
	}
}

void OpenXRFBFaceTracking2Extension::on_process() {
	if (face_tracker == XR_NULL_HANDLE || tracker.is_null()) {
		return;
	}

	float weights[XR_FACE_EXPRESSION2_COUNT_FB] = {};
	float confidences[XR_FACE_CONFIDENCE2_COUNT_FB] = {};
	XrFaceExpressionInfo2FB info = {
		XR_TYPE_FACE_EXPRESSION_INFO2_FB, // type
		nullptr, // next
		OpenXRAPI::get_singleton()->get_predicted_display_time(), // time
	};
	XrFaceExpressionWeights2FB result = {
		XR_TYPE_FACE_EXPRESSION_WEIGHTS2_FB, // type
		nullptr, // next
		XR_FACE_EXPRESSION2_COUNT_FB, // weightCount
		weights, // weights
		XR_FACE_CONFIDENCE2_COUNT_FB, // confidenceCount
		confidences, // confidences
		XR_FALSE, // isValid
		XR_FALSE, // isEyeFollowingBlendshapesValid
		XR_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB, // dataSource
		0, // time
	};

	if (XR_FAILED(xrGetFaceExpressionWeights2FB(face_tracker, &info, &result)) || !result.isValid) {
		return;
	}

	for (const FaceMapping &mapping : face_mapping) {
		for (XRFaceTracker::BlendShapeEntry target : mapping.target) {
			if (target != XRFaceTracker::FT_MAX) {
				tracker->set_blend_shape(target, weights[mapping.source]);
			}
		}
	}
}
