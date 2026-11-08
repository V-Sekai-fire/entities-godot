/**************************************************************************/
/*  quic_client.cpp                                                       */
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

#include "quic_client.h"

// Backend function pointers default to nullptr. The picoquic backend
// (native platforms) or the web WebTransport shim assigns them in
// initialize_quic_module. See modules/meshoptimizer/register_types.cpp
// for the canonical pattern.
void *(*QUICClient::create_backend_func)(QUICClient *, const char *, int, const char *) = nullptr;
void (*QUICClient::destroy_backend_func)(void *) = nullptr;
int (*QUICClient::backend_cnx_count_func)(void *) = nullptr;
int (*QUICClient::backend_cnx_state_func)(void *) = nullptr;
void (*QUICClient::dispatch_cnx_event_func)(QUICClient *, uint64_t, const uint8_t *, size_t, int) = nullptr;
Error (*QUICClient::backend_send_h3_get_func)(void *, const char *, const char *) = nullptr;

void QUICClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("connect_to_host", "host", "port"), &QUICClient::connect_to_host);
	ClassDB::bind_method(D_METHOD("close"), &QUICClient::close);
	ClassDB::bind_method(D_METHOD("poll"), &QUICClient::poll);
	ClassDB::bind_method(D_METHOD("get_status"), &QUICClient::get_status);
	ClassDB::bind_method(D_METHOD("get_connected_host"), &QUICClient::get_connected_host);
	ClassDB::bind_method(D_METHOD("get_connected_port"), &QUICClient::get_connected_port);
	ClassDB::bind_method(D_METHOD("set_blocking_mode", "enabled"), &QUICClient::set_blocking_mode);
	ClassDB::bind_method(D_METHOD("is_blocking_mode_enabled"), &QUICClient::is_blocking_mode_enabled);
	ClassDB::bind_method(D_METHOD("set_read_chunk_size", "size"), &QUICClient::set_read_chunk_size);
	ClassDB::bind_method(D_METHOD("get_read_chunk_size"), &QUICClient::get_read_chunk_size);
	ClassDB::bind_method(D_METHOD("set_alpn", "alpn"), &QUICClient::set_alpn);
	ClassDB::bind_method(D_METHOD("get_alpn"), &QUICClient::get_alpn);
	ClassDB::bind_method(D_METHOD("set_tls_options", "options"), &QUICClient::set_tls_options);
	ClassDB::bind_method(D_METHOD("get_tls_options"), &QUICClient::get_tls_options);
	ClassDB::bind_method(D_METHOD("send_datagram", "payload"), &QUICClient::send_datagram);
	ClassDB::bind_method(D_METHOD("receive_datagram"), &QUICClient::receive_datagram);
	ClassDB::bind_method(D_METHOD("open_stream", "bidirectional"), &QUICClient::open_stream);
	ClassDB::bind_method(D_METHOD("stream_send", "stream_id", "payload"), &QUICClient::stream_send);
	ClassDB::bind_method(D_METHOD("stream_read", "stream_id"), &QUICClient::stream_read);
	ClassDB::bind_method(D_METHOD("stream_close", "stream_id"), &QUICClient::stream_close);

	BIND_ENUM_CONSTANT(STATUS_DISCONNECTED);
	BIND_ENUM_CONSTANT(STATUS_RESOLVING);
	BIND_ENUM_CONSTANT(STATUS_CONNECTING);
	BIND_ENUM_CONSTANT(STATUS_HANDSHAKING);
	BIND_ENUM_CONSTANT(STATUS_CONNECTED);
	BIND_ENUM_CONSTANT(STATUS_BUSY);
	BIND_ENUM_CONSTANT(STATUS_CONNECTION_ERROR);
}
