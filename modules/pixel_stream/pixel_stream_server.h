/**************************************************************************/
/*  pixel_stream_server.h                                                 */
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

#include "scene/main/multiplayer_peer.h"
#include "scene/main/node.h"

// Cuts encoded frames onto the peer's unreliable channel and reads input back
// from its reliable one. The wire layout is contract-pixel-stream's wire.hpp.
class PixelStreamServer : public Node {
	GDCLASS(PixelStreamServer, Node);

	Ref<MultiplayerPeer> peer;
	uint32_t frame_seq = 0;
	uint32_t echoed_input_seq = 0;
	uint64_t frames_sent = 0;
	uint64_t fragments_sent = 0;

protected:
	static void _bind_methods();

public:
	void set_peer(const Ref<MultiplayerPeer> &p_peer);
	Ref<MultiplayerPeer> get_peer() const;

	Error send_frame(const PackedByteArray &p_encoded);
	void poll();

	uint32_t get_echoed_input_seq() const { return echoed_input_seq; }
	uint64_t get_frames_sent() const { return frames_sent; }
	uint64_t get_fragments_sent() const { return fragments_sent; }
	int get_fragments_for(int p_bytes) const;
};
