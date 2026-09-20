/**************************************************************************/
/*  pixel_stream_server.cpp                                               */
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

#include "pixel_stream_server.h"

#include "core/io/marshalls.h"
#include "core/object/class_db.h"

#ifdef PIXEL_STREAM_HAS_CONTRACT
#include "pixel_stream/wire.hpp"
#endif

void PixelStreamServer::set_peer(const Ref<MultiplayerPeer> &p_peer) {
	peer = p_peer;
}

Ref<MultiplayerPeer> PixelStreamServer::get_peer() const {
	return peer;
}

int PixelStreamServer::get_fragments_for(int p_bytes) const {
#ifdef PIXEL_STREAM_HAS_CONTRACT
	return int(pixel_stream::fragment_count_for(size_t(MAX(p_bytes, 0))));
#else
	return 0;
#endif
}

Error PixelStreamServer::send_frame(const PackedByteArray &p_encoded) {
#ifdef PIXEL_STREAM_HAS_CONTRACT
	ERR_FAIL_COND_V_MSG(peer.is_null(), ERR_UNCONFIGURED, "PixelStreamServer has no peer.");

	const size_t total = size_t(p_encoded.size());
	const size_t count = pixel_stream::fragment_count_for(total);
	ERR_FAIL_COND_V_MSG(count > 65535, ERR_INVALID_PARAMETER,
			vformat("Frame of %d bytes needs %d fragments, past the 65535 the wire allows.",
					p_encoded.size(), int(count)));

	peer->set_target_peer(0);
	peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);

	LocalVector<uint8_t> datagram;
	datagram.resize(pixel_stream::kDatagramBytes);

	const uint8_t *src = p_encoded.ptr();
	for (size_t i = 0; i < count; i++) {
		const size_t offset = i * pixel_stream::kMaxPayloadBytes;
		const size_t chunk = MIN(pixel_stream::kMaxPayloadBytes, total - offset);

		pixel_stream::FragmentHeader header{};
		header.version = pixel_stream::kWireVersion;
		header.frame_seq = frame_seq;
		header.frag_index = uint16_t(i);
		header.frag_count = uint16_t(count);
		header.echoed_input_seq = echoed_input_seq;
		header.payload_bytes = uint16_t(chunk);

		const size_t wrote = pixel_stream::encode(header, total == 0 ? nullptr : src + offset,
				datagram.ptr(), datagram.size());
		ERR_FAIL_COND_V(wrote == 0, ERR_BUG);

		const Error err = peer->put_packet(datagram.ptr(), int(wrote));
		if (err != OK) {
			return err;
		}
		fragments_sent++;
	}

	frame_seq++;
	frames_sent++;
	return OK;
#else
	return ERR_UNAVAILABLE;
#endif
}

void PixelStreamServer::poll() {
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
		PackedByteArray input;
		input.resize(size);
		if (size > 0) {
			memcpy(input.ptrw(), buffer, size);
		}
		// The client stamps its input counter first, so a frame can carry the
		// input it was rendered from.
		if (size >= 4) {
			echoed_input_seq = decode_uint32(buffer);
		}
		emit_signal("input_received", input);
	}
}

void PixelStreamServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_peer", "peer"), &PixelStreamServer::set_peer);
	ClassDB::bind_method(D_METHOD("get_peer"), &PixelStreamServer::get_peer);
	ClassDB::bind_method(D_METHOD("send_frame", "encoded"), &PixelStreamServer::send_frame);
	ClassDB::bind_method(D_METHOD("poll"), &PixelStreamServer::poll);
	ClassDB::bind_method(D_METHOD("get_echoed_input_seq"), &PixelStreamServer::get_echoed_input_seq);
	ClassDB::bind_method(D_METHOD("get_frames_sent"), &PixelStreamServer::get_frames_sent);
	ClassDB::bind_method(D_METHOD("get_fragments_sent"), &PixelStreamServer::get_fragments_sent);
	ClassDB::bind_method(D_METHOD("get_fragments_for", "bytes"), &PixelStreamServer::get_fragments_for);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "peer", PROPERTY_HINT_RESOURCE_TYPE, "MultiplayerPeer"),
			"set_peer", "get_peer");

	ADD_SIGNAL(MethodInfo("input_received", PropertyInfo(Variant::PACKED_BYTE_ARRAY, "input")));
}
