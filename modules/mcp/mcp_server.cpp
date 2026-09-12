/**************************************************************************/
/*  mcp_server.cpp                                                        */
/**************************************************************************/

#include "mcp_server.h"

#include "core/io/ip.h"
#include "core/io/json.h"

MCPServer::MCPServer() {
	server.instantiate();
	protocol.instantiate();
}

Error MCPServer::start(int p_port, const String &p_host) {
	return server->listen(p_port, IPAddress(p_host));
}

void MCPServer::stop() {
	for (Client &client : clients) {
		if (client.peer.is_valid()) {
			client.peer->disconnect_from_host();
		}
	}
	clients.clear();
	server->stop();
}

bool MCPServer::is_listening() const {
	return server->is_listening();
}

bool MCPServer::_origin_allowed(const String &p_origin) {
	const String origin = p_origin.to_lower();
	return origin.begins_with("http://localhost") || origin.begins_with("https://localhost") ||
			origin.begins_with("http://127.0.0.1") || origin.begins_with("https://127.0.0.1") ||
			origin.begins_with("http://[::1]") || origin.begins_with("https://[::1]");
}

Dictionary MCPServer::route(const String &p_method, const String &p_path, const Dictionary &p_headers, const String &p_body) {
	Dictionary out;

	if (p_method == "OPTIONS") {
		out["code"] = 204;
		out["ctype"] = "text/plain";
		out["body"] = "";
		return out;
	}
	if (!p_path.begins_with("/mcp")) {
		out["code"] = 404;
		out["ctype"] = "text/plain";
		out["body"] = "not found";
		return out;
	}
	if (p_method == "DELETE") {
		// Stateless: no sessions to terminate.
		out["code"] = 405;
		out["ctype"] = "text/plain";
		out["body"] = "session termination not supported";
		return out;
	}
	if (p_method == "GET") {
		out["code"] = 405;
		out["ctype"] = "text/plain";
		out["body"] = "no server stream";
		return out;
	}
	if (p_method != "POST") {
		out["code"] = 405;
		out["ctype"] = "text/plain";
		out["body"] = "method not allowed";
		return out;
	}

	const String version = p_headers.get("mcp-protocol-version", "");
	if (!version.is_empty() && version != "2025-06-18" && version != "2025-03-26") {
		out["code"] = 400;
		out["ctype"] = "application/json";
		out["body"] = JSON::stringify(MCPProtocol::parse_error_response(-32600, "unsupported MCP-Protocol-Version: " + version));
		return out;
	}

	const Variant request = JSON::parse_string(p_body);
	if (request.get_type() == Variant::NIL && !p_body.strip_edges().is_empty()) {
		out["code"] = 200;
		out["ctype"] = "application/json";
		out["body"] = JSON::stringify(MCPProtocol::parse_error_response(-32700, "parse error"));
		return out;
	}

	bool has_response = false;
	const Dictionary response = protocol->handle_rpc(request, has_response);
	if (!has_response) {
		out["code"] = 202;
		out["ctype"] = "text/plain";
		out["body"] = "";
		return out;
	}

	const String text = JSON::stringify(response);
	if (String(p_headers.get("accept", "")).contains("text/event-stream")) {
		out["code"] = 200;
		out["ctype"] = "text/event-stream";
		out["body"] = "event: message\ndata: " + text + "\n\n";
		return out;
	}
	out["code"] = 200;
	out["ctype"] = "application/json";
	out["body"] = text;
	return out;
}

void MCPServer::_write(const Ref<StreamPeerTCP> &p_peer, const Dictionary &p_response) {
	const int code = p_response.get("code", 200);
	const String ctype = p_response.get("ctype", "text/plain");
	const CharString body = String(p_response.get("body", "")).utf8();

	String status = "200 OK";
	switch (code) {
		case 202:
			status = "202 Accepted";
			break;
		case 204:
			status = "204 No Content";
			break;
		case 400:
			status = "400 Bad Request";
			break;
		case 403:
			status = "403 Forbidden";
			break;
		case 404:
			status = "404 Not Found";
			break;
		case 405:
			status = "405 Method Not Allowed";
			break;
		default:
			break;
	}

	String header = "HTTP/1.1 " + status + "\r\n";
	header += "Content-Type: " + ctype + "\r\n";
	header += "Content-Length: " + itos(body.length()) + "\r\n";
	header += "Access-Control-Allow-Origin: *\r\n";
	header += "Access-Control-Allow-Headers: *\r\n";
	header += "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n";
	header += "Connection: close\r\n\r\n";

	const CharString header_utf8 = header.utf8();
	p_peer->put_data((const uint8_t *)header_utf8.get_data(), header_utf8.length());
	if (body.length() > 0) {
		p_peer->put_data((const uint8_t *)body.get_data(), body.length());
	}
	p_peer->disconnect_from_host();
}

void MCPServer::poll() {
	while (server->is_connection_available()) {
		Client client;
		client.peer = server->take_connection();
		clients.push_back(client);
	}

	List<Client>::Element *next = nullptr;
	for (List<Client>::Element *element = clients.front(); element != nullptr; element = next) {
		next = element->next();
		Client &client = element->get();
		if (client.peer.is_null()) {
			clients.erase(element);
			continue;
		}
		client.peer->poll();
		if (client.peer->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			clients.erase(element);
			continue;
		}

		const int available = client.peer->get_available_bytes();
		if (available > 0) {
			Vector<uint8_t> chunk;
			chunk.resize(available);
			if (client.peer->get_data(chunk.ptrw(), available) == OK) {
				client.buffer.append_array(chunk);
			}
		}

		// A request is ready once the header block has arrived and the body has
		// reached the length the header declares.
		const String text = String::utf8((const char *)client.buffer.ptr(), client.buffer.size());
		const int split = text.find("\r\n\r\n");
		if (split < 0) {
			continue;
		}

		const String head = text.substr(0, split);
		const String body_so_far = text.substr(split + 4);
		const Vector<String> lines = head.split("\r\n", false);
		if (lines.is_empty()) {
			clients.erase(element);
			continue;
		}

		const Vector<String> request_line = lines[0].split(" ", false);
		if (request_line.size() < 2) {
			clients.erase(element);
			continue;
		}

		Dictionary headers;
		for (int i = 1; i < lines.size(); i++) {
			const int colon = lines[i].find_char(':');
			if (colon < 0) {
				continue;
			}
			headers[lines[i].substr(0, colon).strip_edges().to_lower()] = lines[i].substr(colon + 1).strip_edges();
		}

		const int content_length = headers.has("content-length") ? (int)String(headers["content-length"]).to_int() : 0;
		if (body_so_far.utf8().length() < content_length) {
			continue;
		}

		_write(client.peer, route(request_line[0], request_line[1], headers, body_so_far));
		clients.erase(element);
	}
}
