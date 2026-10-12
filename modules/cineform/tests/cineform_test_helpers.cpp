/**************************************************************************/
/*  cineform_test_helpers.cpp                                             */
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

#include "cineform_test_helpers.h"

#include "../movie_writer_cineform.h"
#include "CFHDDecoder.h"
#include "mkvparser/mkvparser.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace TestMovieWriterCineForm {

// write_begin, write_frame and write_end are protected: MovieWriter calls them. The test
// calls them directly, so no viewport or RenderingServer is involved.
class CineFormWriterProbe : public MovieWriterCineForm {
public:
	using MovieWriterCineForm::write_begin;
	using MovieWriterCineForm::write_end;
	using MovieWriterCineForm::write_frame;
};

constexpr uint32_t FPS = 30;
constexpr uint32_t MIX_RATE = 48000;

void set_writer_settings(int p_threads) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	ps->set_setting("editor/movie_writer/mix_rate", MIX_RATE);
	ps->set_setting("editor/movie_writer/speaker_mode", 0);
	ps->set_setting("editor/movie_writer/audio_bit_depth", 16);
	ps->set_setting("editor/movie_writer/cineform/quality", 2);
	ps->set_setting("editor/movie_writer/cineform/keep_alpha", false);
	// 8-bit: the writer's SIMD swizzle submits faster than one encoder thread encodes 4K, so
	// the job queue fills. The 16-bit path converts half floats per component and is slow
	// enough that the encoder keeps up.
	ps->set_setting("editor/movie_writer/cineform/bit_depth", 0);
	ps->set_setting("editor/movie_writer/cineform/thread_count", p_threads);
}

Color frame_color(int p_index) {
	// Far apart in every channel, so a frame mixed with its successor cannot pass for itself.
	// Six colors far apart in every channel, so a frame mixed with its successor cannot pass
	// for itself. Frame i is drawn in color i % 6.
	const int k = p_index % 6;
	return Color(0.1f + 0.15f * k, 0.85f - 0.14f * k, 0.2f + 0.12f * ((k * 7) % 6));
}

// Writes p_frames solid frames. Returns false when write_begin or a write_frame failed.
bool write_movie(const String &p_path, int p_width, int p_height, int p_frames) {
	CineFormWriterProbe *writer = memnew(CineFormWriterProbe);
	bool ok = writer->write_begin(Size2i(p_width, p_height), FPS, p_path) == OK;
	// 16-bit stereo: write_frame reads (mix_rate / fps) * 2 samples of int32.
	Vector<int32_t> audio;
	audio.resize(MIX_RATE / FPS * 2);
	audio.fill(0);
	// Made before the first frame, so the loop submits as fast as the writer allows.
	// RGBA8 is the writer's own format at bit_depth 0, so it submits without a conversion.
	constexpr int PALETTE = 6;
	Ref<Image> images[PALETTE];
	for (int i = 0; i < PALETTE; i++) {
		images[i] = Image::create_empty(p_width, p_height, false, Image::FORMAT_RGBA8);
		images[i]->fill(frame_color(i));
	}
	for (int i = 0; ok && i < p_frames; i++) {
		ok = writer->write_frame(images[i % PALETTE], audio.ptr()) == OK;
	}
	writer->write_end();
	memdelete(writer);
	return ok;
}

// Runs write_movie on a worker and gives up after p_seconds. A writer stuck inside the SDK
// cannot be stopped or joined, so a timeout ends the whole test process with exit 1.
bool write_movie_or_die(const String &p_path, int p_width, int p_height, int p_frames, int p_seconds) {
	std::mutex mutex;
	std::condition_variable signal;
	bool done = false;
	bool ok = false;
	std::thread worker([&]() {
		const bool result = write_movie(p_path, p_width, p_height, p_frames);
		std::lock_guard<std::mutex> lock(mutex);
		ok = result;
		done = true;
		signal.notify_one();
	});
	std::unique_lock<std::mutex> lock(mutex);
	if (!signal.wait_for(lock, std::chrono::seconds(p_seconds), [&] { return done; })) {
		std::printf("FAIL MovieWriterCineForm wrote no file in %d s: the writer is stuck on a full job queue\n", p_seconds);
		std::fflush(stdout);
		std::_Exit(1);
	}
	lock.unlock();
	worker.join();
	return ok;
}

class FileMkvReader : public mkvparser::IMkvReader {
	Ref<FileAccess> file;

public:
	bool open(const String &p_path) {
		file = FileAccess::open(p_path, FileAccess::READ);
		return file.is_valid();
	}
	int Read(long long p_pos, long p_len, unsigned char *p_buf) override {
		file->seek(uint64_t(p_pos));
		return file->get_buffer(p_buf, uint64_t(p_len)) == uint64_t(p_len) ? 0 : -1;
	}
	int Length(long long *r_total, long long *r_available) override {
		const long long len = (long long)file->get_length();
		if (r_total) {
			*r_total = len;
		}
		if (r_available) {
			*r_available = len;
		}
		return 0;
	}
};

// Decodes every video frame of the file and returns the mean color of each, in file order.
Vector<Color> decode_means(const String &p_path) {
	Vector<Color> means;
	FileMkvReader reader;
	if (!reader.open(p_path)) {
		return means;
	}
	long long pos = 0;
	mkvparser::EBMLHeader header;
	if (header.Parse(&reader, pos) < 0) {
		return means;
	}
	mkvparser::Segment *segment = nullptr;
	if (mkvparser::Segment::CreateInstance(&reader, pos, segment) < 0 || segment->Load() < 0) {
		delete segment;
		return means;
	}
	long long video_track = -1;
	int width = 0;
	int height = 0;
	const mkvparser::Tracks *tracks = segment->GetTracks();
	for (unsigned long i = 0; i < tracks->GetTracksCount(); i++) {
		const mkvparser::Track *track = tracks->GetTrackByIndex(i);
		if (track && track->GetType() == mkvparser::Track::kVideo) {
			video_track = track->GetNumber();
			width = int(static_cast<const mkvparser::VideoTrack *>(track)->GetWidth());
			height = int(static_cast<const mkvparser::VideoTrack *>(track)->GetHeight());
			break;
		}
	}

	CFHD_DecoderRef decoder = nullptr;
	if (video_track < 0 || CFHD_OpenDecoder(&decoder, nullptr) != CFHD_ERROR_OKAY) {
		delete segment;
		return means;
	}
	Vector<uint8_t> sample;
	Vector<uint8_t> decoded;
	int32_t pitch = 0;
	for (const mkvparser::Cluster *cluster = segment->GetFirst(); cluster && !cluster->EOS(); cluster = segment->GetNext(cluster)) {
		const mkvparser::BlockEntry *entry = nullptr;
		for (cluster->GetFirst(entry); entry && !entry->EOS(); cluster->GetNext(entry, entry)) {
			const mkvparser::Block *block = entry->GetBlock();
			if (block->GetTrackNumber() != video_track || block->GetFrameCount() < 1) {
				continue;
			}
			const mkvparser::Block::Frame &frame = block->GetFrame(0);
			sample.resize(frame.len);
			if (frame.Read(&reader, sample.ptrw()) < 0) {
				continue;
			}
			if (decoded.is_empty()) {
				int w = 0;
				int h = 0;
				CFHD_PixelFormat format = CFHD_PIXEL_FORMAT_UNKNOWN;
				if (CFHD_PrepareToDecode(decoder, width, height, CFHD_PIXEL_FORMAT_BGRA, CFHD_DECODED_RESOLUTION_FULL,
							CFHD_DECODING_FLAGS_NONE, sample.ptrw(), sample.size(), &w, &h, &format) != CFHD_ERROR_OKAY) {
					break;
				}
				CFHD_GetImagePitch(w, format, &pitch);
				width = w;
				height = h;
				decoded.resize(pitch * height);
			}
			if (CFHD_DecodeSample(decoder, sample.ptrw(), sample.size(), decoded.ptrw(), pitch) != CFHD_ERROR_OKAY) {
				means.push_back(Color(-1, -1, -1));
				continue;
			}
			// BGRA. Every pixel of a frame is the same color, so the mean is the color.
			double sum[3] = { 0, 0, 0 };
			const uint8_t *src = decoded.ptr();
			for (int y = 0; y < height; y++) {
				const uint8_t *s = src + pitch * y;
				for (int x = 0; x < width; x++) {
					sum[0] += s[2];
					sum[1] += s[1];
					sum[2] += s[0];
					s += 4;
				}
			}
			const double n = double(width) * double(height) * 255.0;
			means.push_back(Color(float(sum[0] / n), float(sum[1] / n), float(sum[2] / n)));
		}
	}
	CFHD_CloseDecoder(decoder);
	delete segment;
	return means;
}

} // namespace TestMovieWriterCineForm
