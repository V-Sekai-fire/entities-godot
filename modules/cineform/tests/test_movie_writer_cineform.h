/**************************************************************************/
/*  test_movie_writer_cineform.h                                          */
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

// The CineForm writer, driven frame by frame and read back through the decoder SDK.
//
// Two failures, each invisible from a script:
//  - The encoder pool frees a job slot only when the caller takes a sample out. A writer that
//    submits into a full queue waits for a slot only its own thread could free, so Movie
//    Maker stops with no error. "Full queue" runs one encoder thread at 4K so the queue fills.
//  - CFHD_EncodeAsyncSample does not copy. A staging buffer reused for the next frame is one
//    the pool may still be reading, and the file decodes to frames mixed with their
//    successors. "Each frame" writes a different solid color per frame and checks that every
//    decoded frame is its own color.

#include "cineform_test_helpers.h"

#include "core/io/dir_access.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestMovieWriterCineForm {

TEST_CASE("[MovieWriterCineForm] A full job queue does not stop the writer") {
	set_writer_settings(1); // one encoder thread, so encoding at 4K falls behind and the queue fills
	const String path = TestUtils::get_temp_path("cineform_full_queue.mkv");
	constexpr int FRAMES = 48; // six times the writer's job queue of 8

	CHECK(write_movie_or_die(path, 3840, 2160, FRAMES, 120));
	const Vector<Color> means = decode_means(path);
	CHECK_MESSAGE(means.size() == FRAMES, vformat("%d of %d frames are in the file.", means.size(), FRAMES));
	DirAccess::remove_absolute(path);
}

TEST_CASE("[MovieWriterCineForm] Each frame decodes to its own picture") {
	// One thread and no more frames than the job queue holds: every frame is still waiting to
	// be encoded when the next one is submitted, and the queue never fills, so only the
	// buffer each frame is read from can make this fail.
	set_writer_settings(1);
	const String path = TestUtils::get_temp_path("cineform_each_frame.mkv");
	constexpr int FRAMES = 8;

	REQUIRE(write_movie_or_die(path, 3840, 2160, FRAMES, 120));
	const Vector<Color> means = decode_means(path);
	REQUIRE_MESSAGE(means.size() == FRAMES, vformat("%d of %d frames are in the file.", means.size(), FRAMES));

	// A frame read from another frame's buffer moves every channel: any two of the colors
	// differ by at least 0.12 in each of red, green and blue. So a frame is its own when at
	// least two of its channels are within 0.03 of the color written. The unfixed writer
	// failed 6 of 8 frames this way.
	//
	// Not every channel, because the decoder this test reads through is not deterministic on
	// arm64: decoding the same file twice gave a different first frame in 4 of 40 runs, one
	// channel off by up to 0.12 with the others exact. That is a decoder fault, not the
	// writer's, and it moves one channel only.
	int wrong = 0;
	for (int i = 0; i < FRAMES; i++) {
		const Color want = frame_color(i);
		const int close = int(Math::abs(means[i].r - want.r) <= 0.03f) + int(Math::abs(means[i].g - want.g) <= 0.03f) +
				int(Math::abs(means[i].b - want.b) <= 0.03f);
		if (close < 2) {
			wrong++;
			MESSAGE(vformat("frame %d decodes to %s, written as %s", i, means[i], want));
		}
	}
	CHECK_MESSAGE(wrong == 0, vformat("%d of %d frames do not decode to the picture written for them.", wrong, FRAMES));
	DirAccess::remove_absolute(path);
}

} // namespace TestMovieWriterCineForm
