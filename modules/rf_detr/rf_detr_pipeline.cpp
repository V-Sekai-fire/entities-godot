/**************************************************************************/
/*  rf_detr_pipeline.cpp                                                  */
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

#include "rf_detr_pipeline.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <rfdetr/rfdetr_capi.h>

RFDetrPipeline::RFDetrPipeline() {}

RFDetrPipeline::~RFDetrPipeline() {
	unload();
}

bool RFDetrPipeline::load(const Dictionary &p_opts) {
	if (model) {
		unload();
	}

	ERR_FAIL_COND_V_MSG(!p_opts.has("checkpoint_path"), false, "rf_detr: opts must have checkpoint_path.");

	CharString s_ckpt = String(p_opts["checkpoint_path"]).utf8();

	rfdetr_options opts;
	rfdetr_options_init(&opts);
	if (p_opts.has("num_queries")) {
		opts.num_queries = (int32_t)(int)p_opts["num_queries"];
	}
	if (p_opts.has("score_threshold")) {
		opts.score_threshold = (float)(double)p_opts["score_threshold"];
	}

	rfdetr_status s = rfdetr_model_load(s_ckpt.get_data(), &opts, &model);
	if (s != RFDETR_OK || model == nullptr) {
		model = nullptr;
		ERR_FAIL_V_MSG(false, String("rf_detr: model load failed: ") + rfdetr_status_string(s));
	}
	return true;
}

bool RFDetrPipeline::is_loaded() const {
	return model != nullptr;
}

void RFDetrPipeline::unload() {
	if (model) {
		rfdetr_model_free(model);
		model = nullptr;
	}
}

Array RFDetrPipeline::detect(const PackedByteArray &p_image_rgba, int p_width, int p_height, const Dictionary &p_opts) const {
	Array empty;

	ERR_FAIL_NULL_V_MSG(model, empty, "rf_detr: pipeline not loaded; call load() first.");
	ERR_FAIL_COND_V_MSG(p_image_rgba.size() != p_width * p_height * 4, empty, "rf_detr: image_rgba size must equal width*height*4.");

	const size_t capacity = (size_t)(int)p_opts.get("max_boxes", 512);
	Vector<rfdetr_box> boxes;
	boxes.resize(capacity);
	size_t count = 0;
	rfdetr_status ds = rfdetr_detect(model, p_image_rgba.ptr(), p_width, p_height, boxes.ptrw(), capacity, &count);

	if (ds != RFDETR_OK) {
		ERR_FAIL_V_MSG(empty, String("rf_detr: detect failed: ") + rfdetr_status_string(ds));
	}

	Array out;
	out.resize(count);
	for (size_t i = 0; i < count; i++) {
		Dictionary d;
		d["x"] = boxes[i].x;
		d["y"] = boxes[i].y;
		d["width"] = boxes[i].width;
		d["height"] = boxes[i].height;
		d["score"] = boxes[i].score;
		d["class_id"] = (int)boxes[i].class_id;
		out[i] = d;
	}
	return out;
}

void RFDetrPipeline::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "opts"), &RFDetrPipeline::load);
	ClassDB::bind_method(D_METHOD("is_loaded"), &RFDetrPipeline::is_loaded);
	ClassDB::bind_method(D_METHOD("unload"), &RFDetrPipeline::unload);
	ClassDB::bind_method(D_METHOD("detect", "image_rgba", "width", "height", "opts"), &RFDetrPipeline::detect);
}
