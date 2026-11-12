/**************************************************************************/
/*  xr_grid_zone_scene_tree.cpp                                           */
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

#include "xr_grid_zone_scene_tree.h"

#include "core/io/marshalls.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/variant/variant.h"

void XRGridZoneSceneTree::_parse_cmdline() {
	const List<String> args = OS::get_singleton()->get_cmdline_args();
	for (const String &arg : args) {
		if (arg.begins_with("--zone-port=")) {
			const String spec = arg.split("=", true, 1)[1];
			const int parsed = spec.to_int();
			if (parsed > 0 && parsed < 65536) {
				zone_port = parsed;
			}
		}
	}
}

bool XRGridZoneSceneTree::_try_start_server() {
	// Resolve ENetMultiplayerPeer via ClassDB so the engine builds
	// without modules/enet still link; the failure is surfaced at boot.
	Object *peer_obj = ClassDB::instantiate("ENetMultiplayerPeer");
	if (peer_obj == nullptr) {
		print_error("XRGridZoneSceneTree: ENetMultiplayerPeer is not "
					"registered. Build with modules/enet enabled.");
		return false;
	}
	MultiplayerPeer *peer = Object::cast_to<MultiplayerPeer>(peer_obj);
	if (peer == nullptr) {
		memdelete(peer_obj);
		print_error("XRGridZoneSceneTree: ENetMultiplayerPeer is "
					"registered but is not a MultiplayerPeer subclass.");
		return false;
	}
	Ref<MultiplayerPeer> peer_ref;
	peer_ref.reference_ptr(peer);

	Array server_args;
	server_args.push_back(zone_port);
	const Variant err = peer->callv("create_server", server_args);
	const int err_code = int(err);
	if (err_code != OK) {
		print_error(vformat("XRGridZoneSceneTree: create_server failed on port %d (err=%d).",
				zone_port, err_code));
		return false;
	}
	server_peer = peer_ref;
	print_line(vformat("XRGridZoneSceneTree: ENet listening on port %d", zone_port));
	return true;
}

void XRGridZoneSceneTree::initialize() {
	SceneTree::initialize();
	_parse_cmdline();
	started = _try_start_server();
	if (!started) {
		aborted = true;
	}
}

bool XRGridZoneSceneTree::process(double p_time) {
	if (aborted) {
		// Returning true asks the main loop to exit. Honors the
		// "boot fails fast" rule when the server couldn't start.
		return true;
	}
	if (SceneTree::process(p_time)) {
		return true;
	}
	if (server_peer.is_null()) {
		return false;
	}
	server_peer->poll();
	while (server_peer->get_available_packet_count() > 0) {
		const uint8_t *data = nullptr;
		int len = 0;
		if (server_peer->get_packet(&data, len) != OK || len <= 0) {
			break;
		}
		// Mirror zone_server.gd: only relay our 100-byte entity packets.
		// Stroke packets (CSP1 magic) currently go through the same
		// path as entity packets; once a separate reliable channel is
		// wired the relay can stop sniffing.
		if (len == 100) {
			server_peer->set_target_peer(0);
			server_peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);
			server_peer->put_packet(data, len);
			++relay_count;
		}
	}
	return false;
}

void XRGridZoneSceneTree::finalize() {
	if (server_peer.is_valid()) {
		server_peer.unref();
	}
	SceneTree::finalize();
}

void XRGridZoneSceneTree::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_zone_port"),
			&XRGridZoneSceneTree::get_zone_port);
	ClassDB::bind_method(D_METHOD("get_relay_count"),
			&XRGridZoneSceneTree::get_relay_count);
	ClassDB::bind_method(D_METHOD("is_running"),
			&XRGridZoneSceneTree::is_running);
}
