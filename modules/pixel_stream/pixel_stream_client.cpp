/**************************************************************************/
/*  pixel_stream_client.cpp                                               */
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

#include "pixel_stream_client.h"

#include "core/io/marshalls.h"
#include "core/object/class_db.h"

#ifdef PIXEL_STREAM_HAS_CONTRACT
#include "pixel_stream/wire.hpp"
#endif

void PixelStreamClient::set_peer(const Ref<MultiplayerPeer> &p_peer) {
	peer = p_peer;
}

Ref<MultiplayerPeer> PixelStreamClient::get_peer() const {
	return peer;
}

void PixelStreamClient::set_capacity(int p_capacity) {
	capacity = MAX(1, p_capacity);
#ifdef PIXEL_STREAM_HAS_CONTRACT
	reassembler = pixel_stream::Reassembler(size_t(capacity));
#endif
}

Error PixelStreamClient::send_input(const PackedByteArray &p_input) {
	ERR_FAIL_COND_V_MSG(peer.is_null(), ERR_UNCONFIGURED, "PixelStreamClient has no peer.");

	// The counter leads the payload so the server can echo it back on a frame.
	PackedByteArray stamped;
	stamped.resize(4 + p_input.size());
	encode_uint32(input_seq, stamped.ptrw());
	if (p_input.size() > 0) {
		memcpy(stamped.ptrw() + 4, p_input.ptr(), p_input.size());
	}
	input_seq++;

	peer->set_target_peer(1);
	peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	return peer->put_packet(stamped.ptr(), stamped.size());
}

void PixelStreamClient::poll() {
#ifdef PIXEL_STREAM_HAS_CONTRACT
	if (peer.is_null()) {
		return;
	}
	peer->poll();
	while (peer->get_available_packet_count() > 0) {
		const uint8_t *buffer = nullptr;
		int size = 0;
		if (peer->get_packet(&buffer, size) != OK) {
			break;
		}

		pixel_stream::FragmentHeader header{};
		const uint8_t *payload = nullptr;
		if (!pixel_stream::decode(buffer, size_t(size), header, payload)) {
			fragments_rejected++;
			continue;
		}
		fragments_received++;

		std::optional<pixel_stream::Frame> out = reassembler.step(header, payload);
		if (!out.has_value()) {
			continue;
		}

		PackedByteArray frame;
		frame.resize(int(out->payload.size()));
		if (!out->payload.empty()) {
			memcpy(frame.ptrw(), out->payload.data(), out->payload.size());
		}
		frames_presented++;
		emit_signal("frame_received", frame, int64_t(out->frame_seq), int64_t(out->echoed_input_seq));
	}
#endif
}

void PixelStreamClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_peer", "peer"), &PixelStreamClient::set_peer);
	ClassDB::bind_method(D_METHOD("get_peer"), &PixelStreamClient::get_peer);
	ClassDB::bind_method(D_METHOD("set_capacity", "capacity"), &PixelStreamClient::set_capacity);
	ClassDB::bind_method(D_METHOD("get_capacity"), &PixelStreamClient::get_capacity);
	ClassDB::bind_method(D_METHOD("send_input", "input"), &PixelStreamClient::send_input);
	ClassDB::bind_method(D_METHOD("poll"), &PixelStreamClient::poll);
	ClassDB::bind_method(D_METHOD("get_frames_presented"), &PixelStreamClient::get_frames_presented);
	ClassDB::bind_method(D_METHOD("get_fragments_received"), &PixelStreamClient::get_fragments_received);
	ClassDB::bind_method(D_METHOD("get_fragments_rejected"), &PixelStreamClient::get_fragments_rejected);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "peer", PROPERTY_HINT_RESOURCE_TYPE, "MultiplayerPeer"),
			"set_peer", "get_peer");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "capacity", PROPERTY_HINT_RANGE, "1,64,1"),
			"set_capacity", "get_capacity");

	ADD_SIGNAL(MethodInfo("frame_received",
			PropertyInfo(Variant::PACKED_BYTE_ARRAY, "encoded"),
			PropertyInfo(Variant::INT, "frame_seq"),
			PropertyInfo(Variant::INT, "echoed_input_seq")));
}
