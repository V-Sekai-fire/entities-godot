/**************************************************************************/
/*  openxr_fb_face_tracking2_extension.h                                  */
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


#pragma once

#include "openxr_extension_wrapper.h"

#include "servers/xr/xr_face_tracker.h"

// Reads XR_FB_face_tracking2 weights from the runtime and writes them to the "/user/face_tracker" XRFaceTracker.
//
// See: https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#XR_FB_face_tracking2

class OpenXRFBFaceTracking2Extension : public OpenXRExtensionWrapper {
	GDCLASS(OpenXRFBFaceTracking2Extension, OpenXRExtensionWrapper);

protected:
	static void _bind_methods() {}

public:
	static OpenXRFBFaceTracking2Extension *get_singleton();

	OpenXRFBFaceTracking2Extension();
	virtual ~OpenXRFBFaceTracking2Extension() override;

	virtual HashMap<String, bool *> get_requested_extensions(XrVersion p_version) override;
	virtual void *set_system_properties_and_get_next_pointer(void *p_next_pointer) override;

	virtual void on_instance_created(const XrInstance p_instance) override;
	virtual void on_instance_destroyed() override;
	virtual void on_session_created(const XrSession p_session) override;
	virtual void on_session_destroyed() override;
	virtual void on_process() override;

	bool is_active() const;

private:
	static OpenXRFBFaceTracking2Extension *singleton;

	bool available = false;
	XrSystemFaceTrackingProperties2FB properties;
	XrFaceTracker2FB face_tracker = XR_NULL_HANDLE;
	Ref<XRFaceTracker> tracker;

	EXT_PROTO_XRRESULT_FUNC3(xrCreateFaceTracker2FB, (XrSession), p_session, (const XrFaceTrackerCreateInfo2FB *), p_create_info, (XrFaceTracker2FB *), p_face_tracker)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyFaceTracker2FB, (XrFaceTracker2FB), p_face_tracker)
	EXT_PROTO_XRRESULT_FUNC3(xrGetFaceExpressionWeights2FB, (XrFaceTracker2FB), p_face_tracker, (const XrFaceExpressionInfo2FB *), p_expression_info, (XrFaceExpressionWeights2FB *), p_expression_weights)
};
