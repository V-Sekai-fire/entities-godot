/**************************************************************************/
/*  pixel_stream_client.h                                                 */
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

#ifdef PIXEL_STREAM_HAS_CONTRACT
#include "pixel_stream/reassembler.hpp"
#endif

// Reassembles frames from the peer's unreliable channel and sends input back on
// its reliable one. A frame with any fragment missing is dropped, never shown.
class PixelStreamClient : public Node {
	GDCLASS(PixelStreamClient, Node);

	Ref<MultiplayerPeer> peer;
	int capacity = 4;
	uint64_t frames_presented = 0;
	uint64_t fragments_received = 0;
	uint64_t fragments_rejected = 0;
	uint32_t input_seq = 0;

#ifdef PIXEL_STREAM_HAS_CONTRACT
	pixel_stream::Reassembler reassembler{ 4 };
#endif

protected:
	static void _bind_methods();

public:
	void set_peer(const Ref<MultiplayerPeer> &p_peer);
	Ref<MultiplayerPeer> get_peer() const;

	void set_capacity(int p_capacity);
	int get_capacity() const { return capacity; }

	Error send_input(const PackedByteArray &p_input);
	void poll();

	uint64_t get_frames_presented() const { return frames_presented; }
	uint64_t get_fragments_received() const { return fragments_received; }
	uint64_t get_fragments_rejected() const { return fragments_rejected; }
};
