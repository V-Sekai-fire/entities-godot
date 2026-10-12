/**************************************************************************/
/*  web_media_backend.h                                                   */
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

#include "modules/native_media/native_media_backend.h"

#include "core/os/mutex.h"

// The browser's own decoders, through library_godot_native_media.js.
//
// Video plays in a hidden <video> element. present_video_frame() copies each new frame
// on the GPU into the GL texture behind the caller's ImageTexture, so no pixel passes
// through the CPU. decode_video_frame() is the contract's RGBA8 fallback, read back
// through a canvas.
//
// Audio is decoded once, in full, by decodeAudioData at a fixed 48 kHz stereo. The PCM
// arrives through a main-thread callback; decode_audio() only reads it, so the audio
// thread never calls into JavaScript.
//
// The browser answers asynchronously: until the metadata arrives, video info is absent
// and frames are ERR_BUSY; until decoding finishes, audio pulls return 0 frames without
// reaching end-of-stream.
class WebMediaBackend : public NativeMediaBackend {
	static constexpr uint32_t AUDIO_RATE = 48000;

	int js_id = 0;
	mutable Mutex mutex;

	// Audio, filled once by _on_audio. pcm is interleaved stereo and owned here.
	float *pcm = nullptr;
	uint64_t pcm_frames = 0;
	uint64_t cursor = 0;
	double audio_duration = 0.0;
	double pending_seek = -1.0; // A seek that arrived before the PCM did.
	bool audio_done = false;

	// Video, filled by _on_meta once the element has loaded its metadata.
	mutable bool video_started = false;
	bool meta_done = false;
	bool has_video = false;
	uint32_t width = 0;
	uint32_t height = 0;
	double video_duration = 0.0;

	void _start_video() const;
	Error _frame_result(int p_code) const;

	static void _on_meta(void *p_ref, int p_has_video, int p_width, int p_height, double p_duration);
	static void _on_audio(void *p_ref, float *p_pcm, int p_frames, int p_rate, double p_duration);

public:
	virtual Error open(const Vector<uint8_t> &p_data, ContainerFormat p_hint = CONTAINER_AUTO) override;

	virtual AudioInfo get_audio_info() const override;
	virtual VideoInfo get_video_info() const override;

	virtual int decode_audio(AudioFrame *p_buffer, int p_frames) override;
	virtual bool is_audio_eof() const override;

	virtual Error decode_video_frame(Vector<uint8_t> *r_buffer, double *r_pts_seconds) override;
	virtual Error present_video_frame(uint64_t p_native_texture, double *r_pts_seconds) override;

	virtual Error seek(double p_time_seconds) override;
	virtual void reset() override;
	virtual void set_paused(bool p_paused) override;

	virtual String get_backend_name() const override { return "Web"; }

	~WebMediaBackend();
};
