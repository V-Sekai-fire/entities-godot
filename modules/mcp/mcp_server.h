/**************************************************************************/
/*  mcp_server.h                                                          */
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

#include "mcp_protocol.h"

#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"

// Streamable-HTTP transport for the MCP protocol: a minimal HTTP/1.1 endpoint
// over TCPServer that answers POST /mcp. route() is transport-free so the
// tests can exercise it without sockets.
class MCPServer : public RefCounted {
	GDCLASS(MCPServer, RefCounted);

	struct Client {
		Ref<StreamPeerTCP> peer;
		Vector<uint8_t> buffer;
	};

	Ref<TCPServer> server;
	Ref<MCPProtocol> protocol;
	List<Client> clients;

	static bool _origin_allowed(const String &p_origin);
	void _write(const Ref<StreamPeerTCP> &p_peer, const Dictionary &p_response);

public:
	static const int DEFAULT_PORT = 8788;

	MCPServer();

	Ref<MCPProtocol> get_protocol() const { return protocol; }

	Error start(int p_port = DEFAULT_PORT, const String &p_host = "127.0.0.1");
	void stop();
	bool is_listening() const;
	void poll();

	// Produce { code, ctype, body } for a parsed request. No sockets involved.
	Dictionary route(const String &p_method, const String &p_path, const Dictionary &p_headers, const String &p_body);
};
