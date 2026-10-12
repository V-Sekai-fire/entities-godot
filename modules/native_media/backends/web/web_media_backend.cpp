/**************************************************************************/
/*  web_media_backend.cpp                                                 */
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

#include "web_media_backend.h"

#include <cstdlib>

extern "C" {
// library_godot_native_media.js. Frame calls return 1 for a new frame, 0 when none is
// ready, -1 at the end and -2 on error.
extern int godot_js_native_media_create(const uint8_t *p_data, int p_len, int p_hint, void *p_ref,
		void (*p_on_meta)(void *, int, int, int, double),
		void (*p_on_audio)(void *, float *, int, int, double));
extern void godot_js_native_media_start_video(int p_id);
extern void godot_js_native_media_set_paused(int p_id, int p_paused);
extern void godot_js_native_media_seek(int p_id, double p_time);
extern int godot_js_native_media_upload(int p_id, uint32_t p_texture, double *r_pts);
extern int godot_js_native_media_read(int p_id, uint8_t *p_buffer, int p_size, double *r_pts);
extern void godot_js_native_media_destroy(int p_id);
}

void WebMediaBackend::_on_meta(void *p_ref, int p_has_video, int p_width, int p_height, double p_duration) {
	WebMediaBackend *self = static_cast<WebMediaBackend *>(p_ref);
	MutexLock lock(self->mutex);
	self->meta_done = true;
	self->has_video = p_has_video != 0;
	self->width = uint32_t(MAX(p_width, 0));
	self->height = uint32_t(MAX(p_height, 0));
	self->video_duration = p_duration;
}

void WebMediaBackend::_on_audio(void *p_ref, float *p_pcm, int p_frames, int p_rate, double p_duration) {
	WebMediaBackend *self = static_cast<WebMediaBackend *>(p_ref);
	MutexLock lock(self->mutex);
	self->pcm = p_pcm;
	self->pcm_frames = p_frames > 0 ? uint64_t(p_frames) : 0;
	self->audio_duration = p_duration;
	self->audio_done = true;
	if (self->pending_seek >= 0.0) {
		self->cursor = MIN(uint64_t(self->pending_seek * AUDIO_RATE), self->pcm_frames);
		self->pending_seek = -1.0;
	}
}

Error WebMediaBackend::open(const Vector<uint8_t> &p_data, ContainerFormat p_hint) {
	ERR_FAIL_COND_V(js_id != 0, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_data.is_empty(), ERR_INVALID_DATA);
	js_id = godot_js_native_media_create(p_data.ptr(), p_data.size(), int(p_hint), this, &_on_meta, &_on_audio);
	return js_id != 0 ? OK : ERR_CANT_OPEN;
}

void WebMediaBackend::_start_video() const {
	if (!video_started && js_id != 0) {
		video_started = true;
		godot_js_native_media_start_video(js_id);
	}
}

NativeMediaBackend::AudioInfo WebMediaBackend::get_audio_info() const {
	MutexLock lock(mutex);
	AudioInfo info;
	// The rate and channel count are fixed, so they hold before decoding finishes.
	info.sample_rate = AUDIO_RATE;
	info.channels = 2;
	info.duration_seconds = audio_duration;
	info.present = audio_done && pcm_frames > 0;
	return info;
}

NativeMediaBackend::VideoInfo WebMediaBackend::get_video_info() const {
	_start_video();
	MutexLock lock(mutex);
	VideoInfo info;
	info.width = width;
	info.height = height;
	info.frame_rate = 0.0f; // Browsers do not expose it.
	info.duration_seconds = video_duration;
	info.preferred_format = VIDEO_FRAME_RGBA8;
	info.present = meta_done && has_video;
	return info;
}

int WebMediaBackend::decode_audio(AudioFrame *p_buffer, int p_frames) {
	MutexLock lock(mutex);
	if (!audio_done || p_frames <= 0) {
		return 0;
	}
	const uint64_t count = MIN(uint64_t(p_frames), pcm_frames - cursor);
	const float *src = pcm + cursor * 2;
	for (uint64_t i = 0; i < count; i++) {
		p_buffer[i] = AudioFrame(src[i * 2], src[i * 2 + 1]);
	}
	cursor += count;
	return int(count);
}

bool WebMediaBackend::is_audio_eof() const {
	MutexLock lock(mutex);
	return audio_done && cursor >= pcm_frames;
}

Error WebMediaBackend::_frame_result(int p_code) const {
	switch (p_code) {
		case 1:
			return OK;
		case 0:
			return ERR_BUSY;
		case -1:
			return ERR_FILE_EOF;
		default:
			return FAILED;
	}
}

Error WebMediaBackend::present_video_frame(uint64_t p_native_texture, double *r_pts_seconds) {
	ERR_FAIL_COND_V(js_id == 0, ERR_UNCONFIGURED);
	_start_video();
	double pts = 0.0;
	const Error err = _frame_result(godot_js_native_media_upload(js_id, uint32_t(p_native_texture), &pts));
	if (err == OK && r_pts_seconds) {
		*r_pts_seconds = pts;
	}
	return err;
}

Error WebMediaBackend::decode_video_frame(Vector<uint8_t> *r_buffer, double *r_pts_seconds) {
	ERR_FAIL_COND_V(js_id == 0, ERR_UNCONFIGURED);
	ERR_FAIL_NULL_V(r_buffer, ERR_INVALID_PARAMETER);
	const VideoInfo info = get_video_info();
	if (!info.present) {
		MutexLock lock(mutex);
		return meta_done ? ERR_FILE_EOF : ERR_BUSY;
	}
	r_buffer->resize(int64_t(info.width) * info.height * 4);
	double pts = 0.0;
	const Error err = _frame_result(godot_js_native_media_read(js_id, r_buffer->ptrw(), r_buffer->size(), &pts));
	if (err == OK && r_pts_seconds) {
		*r_pts_seconds = pts;
	}
	return err;
}

Error WebMediaBackend::seek(double p_time_seconds) {
	ERR_FAIL_COND_V(js_id == 0, ERR_UNCONFIGURED);
	const double t = MAX(p_time_seconds, 0.0);
	{
		MutexLock lock(mutex);
		if (audio_done) {
			cursor = MIN(uint64_t(t * AUDIO_RATE), pcm_frames);
		} else {
			pending_seek = t;
		}
	}
	godot_js_native_media_seek(js_id, t);
	return OK;
}

void WebMediaBackend::reset() {
	if (js_id == 0) {
		return;
	}
	set_paused(true);
	seek(0.0);
}

void WebMediaBackend::set_paused(bool p_paused) {
	if (js_id != 0) {
		godot_js_native_media_set_paused(js_id, p_paused ? 1 : 0);
	}
}

WebMediaBackend::~WebMediaBackend() {
	if (js_id != 0) {
		// After this, no callback can reach this object: the JS side drops it first.
		godot_js_native_media_destroy(js_id);
		js_id = 0;
	}
	MutexLock lock(mutex);
	if (pcm) {
		free(pcm); // Allocated by GodotRuntime.malloc, which is malloc.
		pcm = nullptr;
	}
}
