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
/* SPDX-License-Identifier: MIT                                           */
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
