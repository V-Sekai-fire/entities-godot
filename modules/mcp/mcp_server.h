/**************************************************************************/
/*  mcp_server.h                                                          */
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
