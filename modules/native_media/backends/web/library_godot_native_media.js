/**************************************************************************/
/*  library_godot_native_media.js                                         */
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

// The browser is the decoder. Video plays in a hidden <video> element and each new
// frame goes to the GPU with texSubImage2D, straight into the GL texture behind a
// Godot ImageTexture. Audio is decoded once with decodeAudioData, resampled by the
// browser to a fixed rate, and handed to C++ as interleaved stereo float PCM.
//
// Everything here touches the DOM or the WebGL context, so every entry point runs on
// the main thread (__proxy: 'sync'). Results that arrive later (metadata, decoded
// audio) come back through callbacks, also on the main thread.
const GodotNativeMedia = {
	$GodotNativeMedia__deps: ['$IDHandler', '$GodotRuntime', '$GL'],
	$GodotNativeMedia: {
		// The rate decodeAudioData resamples to. Fixed, so C++ can report it before
		// decoding finishes.
		AUDIO_RATE: 48000,

		// Codes shared with web_media_backend.cpp.
		FRAME_NEW: 1,
		FRAME_BUSY: 0,
		FRAME_EOF: -1,
		FRAME_ERROR: -2,

		// Container hints, in NativeMediaBackend::ContainerFormat order.
		mime: function (p_hint, p_bytes) {
			switch (p_hint) {
			case 1:
				return 'video/webm';
			case 2:
				return 'video/mp4';
			case 3:
				return 'video/ogg';
			case 4:
				return 'audio/wav';
			default:
				break;
			}
			const b = p_bytes;
			if (b.length >= 4 && b[0] === 0x1a && b[1] === 0x45 && b[2] === 0xdf && b[3] === 0xa3) {
				return 'video/webm';
			}
			if (b.length >= 8 && b[4] === 0x66 && b[5] === 0x74 && b[6] === 0x79 && b[7] === 0x70) { // ftyp
				return 'video/mp4';
			}
			if (b.length >= 4 && b[0] === 0x4f && b[1] === 0x67 && b[2] === 0x67 && b[3] === 0x53) { // OggS
				return 'video/ogg';
			}
			if (b.length >= 4 && b[0] === 0x52 && b[1] === 0x49 && b[2] === 0x46 && b[3] === 0x46) { // RIFF
				return 'audio/wav';
			}
			if (b.length >= 4 && b[0] === 0x66 && b[1] === 0x4c && b[2] === 0x61 && b[3] === 0x43) { // fLaC
				return 'audio/flac';
			}
			return '';
		},

		decode_audio: function (p_media, p_on_audio) {
			const ctor = window['OfflineAudioContext'] || window['webkitOfflineAudioContext'];
			const rate = GodotNativeMedia.AUDIO_RATE;
			if (!ctor) {
				p_on_audio(0, 0, rate, 0);
				return;
			}
			const ctx = new ctor(2, 1, rate);
			// decodeAudioData detaches the buffer it is given, so it gets a copy.
			const buffer = p_media.bytes.slice().buffer;
			const done = function (p_audio) {
				if (p_media.dead) {
					return;
				}
				if (!p_audio || p_audio.length === 0) {
					p_on_audio(0, 0, rate, 0);
					return;
				}
				const frames = p_audio.length;
				const left = p_audio.getChannelData(0);
				const right = p_audio.numberOfChannels > 1 ? p_audio.getChannelData(1) : left;
				// C++ takes ownership of this block and frees it with free().
				const ptr = GodotRuntime.malloc(frames * 2 * 4);
				if (!ptr) {
					p_on_audio(0, 0, rate, 0);
					return;
				}
				const out = HEAPF32.subarray(ptr >> 2, (ptr >> 2) + frames * 2);
				for (let i = 0; i < frames; i++) {
					out[i * 2] = left[i];
					out[i * 2 + 1] = right[i];
				}
				p_on_audio(ptr, frames, rate, p_audio.duration);
			};
			ctx.decodeAudioData(buffer).then(done, function () {
				done(null);
			});
		},

		// The <video> element is made on the first video request, so an audio-only
		// stream never creates one.
		start_video: function (p_media) {
			if (p_media.video) {
				return;
			}
			const video = document.createElement('video');
			video.muted = true;
			video.playsInline = true;
			video.preload = 'auto';
			// Attached and inside the viewport: some browsers stop decoding, or stop
			// frame callbacks, for a video that is detached or scrolled out of view.
			video.style.position = 'fixed';
			video.style.left = '0';
			video.style.top = '0';
			video.style.width = '1px';
			video.style.height = '1px';
			video.style.opacity = '0';
			video.style.pointerEvents = 'none';
			video.style.zIndex = '-1';
			p_media.video = video;
			p_media.url = URL.createObjectURL(new Blob([p_media.bytes], { type: p_media.mime }));

			const meta = function (p_ok) {
				if (p_media.dead || p_media.meta_sent) {
					return;
				}
				p_media.meta_sent = true;
				const w = p_ok ? video.videoWidth : 0;
				const h = p_ok ? video.videoHeight : 0;
				const duration = p_ok && Number.isFinite(video.duration) ? video.duration : 0;
				p_media.has_video = w > 0 && h > 0;
				p_media.on_meta(p_media.has_video ? 1 : 0, w, h, duration);
			};
			video.addEventListener('loadedmetadata', function () {
				meta(true);
			});
			video.addEventListener('error', function () {
				meta(false);
			});
			if (typeof video['requestVideoFrameCallback'] === 'function') {
				const on_frame = function (p_now, p_info) {
					if (p_media.dead) {
						return;
					}
					p_media.serial++;
					p_media.pts = p_info['mediaTime'];
					video['requestVideoFrameCallback'](on_frame);
				};
				video['requestVideoFrameCallback'](on_frame);
				p_media.frame_callbacks = true;
			}
			document.body.appendChild(video);
			video.src = p_media.url;
			video.load();
		},

		// Plays or pauses the element to match the player. Muted video may start
		// without a user gesture, so play() is expected to succeed.
		sync_play_state: function (p_media) {
			const video = p_media.video;
			if (!video || !p_media.has_video) {
				return;
			}
			if (p_media.paused) {
				if (!video.paused) {
					video.pause();
				}
			} else if (video.paused && !video.ended) {
				const promise = video.play();
				if (promise) {
					promise.catch(function () {});
				}
			}
		},

		// Whether a frame newer than the one last taken is showing. Without frame
		// callbacks, a change of currentTime stands in for a new frame.
		take_frame: function (p_media) {
			const video = p_media.video;
			if (!p_media.frame_callbacks) {
				if (video.currentTime !== p_media.last_time) {
					p_media.serial++;
					p_media.pts = video.currentTime;
					p_media.last_time = video.currentTime;
				}
			}
			if (p_media.serial === p_media.taken) {
				return false;
			}
			p_media.taken = p_media.serial;
			return true;
		},

		// Shared start of both frame paths. Returns FRAME_NEW when a new frame is
		// ready to copy, or the code to return.
		frame_ready: function (p_media) {
			if (!p_media.meta_sent) {
				return GodotNativeMedia.FRAME_BUSY;
			}
			if (!p_media.has_video) {
				return GodotNativeMedia.FRAME_EOF;
			}
			const video = p_media.video;
			GodotNativeMedia.sync_play_state(p_media);
			if (video.readyState < 2) { // HAVE_CURRENT_DATA
				return GodotNativeMedia.FRAME_BUSY;
			}
			if (!GodotNativeMedia.take_frame(p_media)) {
				return video.ended ? GodotNativeMedia.FRAME_EOF : GodotNativeMedia.FRAME_BUSY;
			}
			return GodotNativeMedia.FRAME_NEW;
		},
	},

	godot_js_native_media_create__proxy: 'sync',
	godot_js_native_media_create__sig: 'iiiiiii',
	godot_js_native_media_create: function (p_data, p_len, p_hint, p_ref, p_on_meta, p_on_audio) {
		const on_meta = GodotRuntime.get_func(p_on_meta);
		const on_audio = GodotRuntime.get_func(p_on_audio);
		const bytes = HEAPU8.slice(p_data, p_data + p_len);
		const media = {
			bytes: bytes,
			mime: GodotNativeMedia.mime(p_hint, bytes),
			video: null,
			url: null,
			canvas: null,
			context: null,
			dead: false,
			meta_sent: false,
			has_video: false,
			paused: true,
			frame_callbacks: false,
			serial: 0,
			taken: 0,
			last_time: -1,
			pts: 0,
			on_meta: function (p_has_video, p_width, p_height, p_duration) {
				on_meta(p_ref, p_has_video, p_width, p_height, p_duration);
			},
		};
		const id = IDHandler.add(media);
		GodotNativeMedia.decode_audio(media, function (p_ptr, p_frames, p_rate, p_duration) {
			if (media.dead) {
				if (p_ptr) {
					GodotRuntime.free(p_ptr);
				}
				return;
			}
			on_audio(p_ref, p_ptr, p_frames, p_rate, p_duration);
		});
		return id;
	},

	godot_js_native_media_start_video__proxy: 'sync',
	godot_js_native_media_start_video__sig: 'vi',
	godot_js_native_media_start_video: function (p_id) {
		const media = IDHandler.get(p_id);
		if (media) {
			GodotNativeMedia.start_video(media);
		}
	},

	godot_js_native_media_set_paused__proxy: 'sync',
	godot_js_native_media_set_paused__sig: 'vii',
	godot_js_native_media_set_paused: function (p_id, p_paused) {
		const media = IDHandler.get(p_id);
		if (!media) {
			return;
		}
		media.paused = p_paused !== 0;
		GodotNativeMedia.sync_play_state(media);
	},

	godot_js_native_media_seek__proxy: 'sync',
	godot_js_native_media_seek__sig: 'vid',
	godot_js_native_media_seek: function (p_id, p_time) {
		const media = IDHandler.get(p_id);
		if (!media || !media.video) {
			return;
		}
		media.video.currentTime = p_time;
		media.last_time = -1;
		GodotNativeMedia.sync_play_state(media);
	},

	// Uploads the newest frame into the GL texture p_texture, which must be RGBA8 at
	// the video's size. Writes its presentation time to r_pts.
	godot_js_native_media_upload__proxy: 'sync',
	godot_js_native_media_upload__sig: 'iiii',
	godot_js_native_media_upload: function (p_id, p_texture, r_pts) {
		const media = IDHandler.get(p_id);
		if (!media) {
			return GodotNativeMedia.FRAME_ERROR;
		}
		const ready = GodotNativeMedia.frame_ready(media);
		if (ready !== GodotNativeMedia.FRAME_NEW) {
			return ready;
		}
		const gl = GL.currentContext ? GL.currentContext.GLctx : null;
		const texture = GL.textures[p_texture];
		if (!gl || !texture) {
			return GodotNativeMedia.FRAME_ERROR;
		}
		const previous = gl.getParameter(gl.TEXTURE_BINDING_2D);
		const flip = gl.getParameter(gl.UNPACK_FLIP_Y_WEBGL);
		const premultiply = gl.getParameter(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL);
		gl.bindTexture(gl.TEXTURE_2D, texture);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
		gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
		gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, media.video);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, flip);
		gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, premultiply);
		gl.bindTexture(gl.TEXTURE_2D, previous);
		GodotRuntime.setHeapValue(r_pts, media.pts, 'double');
		return GodotNativeMedia.FRAME_NEW;
	},

	// Copies the newest frame as RGBA8 into p_buffer (p_size bytes). The fallback for
	// callers without a GL texture; it reads the frame back through a canvas.
	godot_js_native_media_read__proxy: 'sync',
	godot_js_native_media_read__sig: 'iiiii',
	godot_js_native_media_read: function (p_id, p_buffer, p_size, r_pts) {
		const media = IDHandler.get(p_id);
		if (!media) {
			return GodotNativeMedia.FRAME_ERROR;
		}
		const ready = GodotNativeMedia.frame_ready(media);
		if (ready !== GodotNativeMedia.FRAME_NEW) {
			return ready;
		}
		const video = media.video;
		const w = video.videoWidth;
		const h = video.videoHeight;
		if (p_size < w * h * 4) {
			return GodotNativeMedia.FRAME_ERROR;
		}
		if (!media.canvas || media.canvas.width !== w || media.canvas.height !== h) {
			media.canvas = document.createElement('canvas');
			media.canvas.width = w;
			media.canvas.height = h;
			media.context = media.canvas.getContext('2d', { willReadFrequently: true });
		}
		media.context.drawImage(video, 0, 0, w, h);
		const pixels = media.context.getImageData(0, 0, w, h).data;
		HEAPU8.set(pixels, p_buffer);
		GodotRuntime.setHeapValue(r_pts, media.pts, 'double');
		return GodotNativeMedia.FRAME_NEW;
	},

	godot_js_native_media_destroy__proxy: 'sync',
	godot_js_native_media_destroy__sig: 'vi',
	godot_js_native_media_destroy: function (p_id) {
		const media = IDHandler.get(p_id);
		if (!media) {
			return;
		}
		media.dead = true;
		if (media.video) {
			media.video.pause();
			media.video.removeAttribute('src');
			media.video.load();
			if (media.video.parentNode) {
				media.video.parentNode.removeChild(media.video);
			}
		}
		if (media.url) {
			URL.revokeObjectURL(media.url);
		}
		media.bytes = null;
		IDHandler.remove(p_id);
	},
};

autoAddDeps(GodotNativeMedia, '$GodotNativeMedia');
mergeInto(LibraryManager.library, GodotNativeMedia);
