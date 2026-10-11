/**************************************************************************/
/*  cineform_test_helpers.h                                               */
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

// Helpers for test_movie_writer_cineform.h. Their definitions are in cineform_test_helpers.cpp,
// which builds in the module's own environment: only that one sees the CineForm SDK and
// libwebm headers. This header holds no test cases, so including it twice registers nothing.

#include "core/math/color.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"

namespace TestMovieWriterCineForm {

void set_writer_settings(int p_threads);
Color frame_color(int p_index);
// Writes p_frames solid frames on a worker. A writer still stuck after p_seconds ends the
// test process with exit 1, because a thread blocked inside the SDK cannot be stopped.
bool write_movie_or_die(const String &p_path, int p_width, int p_height, int p_frames, int p_seconds);
// The mean color of every decoded video frame, in file order.
Vector<Color> decode_means(const String &p_path);

} // namespace TestMovieWriterCineForm
