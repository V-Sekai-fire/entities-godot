/**************************************************************************/
/*  test_mcp.h                                                            */
/**************************************************************************/

#pragma once

#include "../mcp_commands.h"
#include "../mcp_protocol.h"
#include "../mcp_server.h"

#include "core/io/json.h"
#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

namespace TestMCP {

static Dictionary make_request(const String &p_method, const Variant &p_id) {
	Dictionary request;
	request["jsonrpc"] = "2.0";
	request["method"] = p_method;
	if (p_id.get_type() != Variant::NIL) {
		request["id"] = p_id;
	}
	return request;
}

TEST_CASE("[MCP] initialize reports the protocol version and server identity") {
	Ref<MCPProtocol> protocol;
	protocol.instantiate();

	bool has_response = false;
	const Dictionary response = protocol->handle_rpc(make_request("initialize", 1), has_response);

	CHECK(has_response);
	CHECK(response["jsonrpc"] == Variant("2.0"));
	CHECK(response["id"] == Variant(1));
	const Dictionary result = response["result"];
	CHECK(result["protocolVersion"] == Variant(MCPProtocol::PROTOCOL_VERSION));
	const Dictionary server = result["serverInfo"];
	CHECK(server["name"] == Variant(MCPProtocol::SERVER_NAME));
}

TEST_CASE("[MCP] a notification gets no response and an unknown method does") {
	Ref<MCPProtocol> protocol;
	protocol.instantiate();

	bool has_response = true;
	protocol->handle_rpc(make_request("notifications/initialized", Variant()), has_response);
	CHECK_FALSE(has_response);

	has_response = false;
	const Dictionary response = protocol->handle_rpc(make_request("no/such/method", 7), has_response);
	CHECK(has_response);
	const Dictionary error = response["error"];
	CHECK(int(error["code"]) == -32601);
}

TEST_CASE("[MCP] a message that is not a dictionary is an invalid request") {
	Ref<MCPProtocol> protocol;
	protocol.instantiate();

	bool has_response = false;
	const Dictionary response = protocol->handle_rpc(Variant("not a request"), has_response);
	CHECK(has_response);
	const Dictionary error = response["error"];
	CHECK(int(error["code"]) == -32600);
}

TEST_CASE("[MCP] every advertised tool carries a JSON Schema and is dispatchable") {
	const Array tools = MCPProtocol::tool_schemas();
	CHECK(tools.size() > 0);

	for (int i = 0; i < tools.size(); i++) {
		const Dictionary tool = tools[i];
		const String name = tool["name"];
		CHECK_FALSE(name.is_empty());
		CHECK_FALSE(String(tool["description"]).is_empty());

		const Dictionary schema = tool["inputSchema"];
		CHECK(schema["type"] == Variant("object"));
		CHECK(schema.has("properties"));
		CHECK(schema["additionalProperties"] == Variant(false));

		// The registry and the dispatcher must agree, or a listed tool answers
		// "unknown cmd" when it is called.
		CHECK(MCPProtocol::has_tool(name));
	}
}

TEST_CASE("[MCP] calling an unlisted tool is a parameter error") {
	Ref<MCPProtocol> protocol;
	protocol.instantiate();

	Dictionary params;
	params["name"] = "no_such_tool";
	Dictionary request = make_request("tools/call", 3);
	request["params"] = params;

	bool has_response = false;
	const Dictionary response = protocol->handle_rpc(request, has_response);
	const Dictionary error = response["error"];
	CHECK(int(error["code"]) == -32602);
}

TEST_CASE("[MCP] ping answers through the tool surface") {
	Ref<MCPProtocol> protocol;
	protocol.instantiate();

	Dictionary params;
	params["name"] = "ping";
	params["arguments"] = Dictionary();
	Dictionary request = make_request("tools/call", 4);
	request["params"] = params;

	bool has_response = false;
	const Dictionary response = protocol->handle_rpc(request, has_response);
	const Dictionary result = response["result"];
	CHECK(result["isError"] == Variant(false));

	const Array content = result["content"];
	const Dictionary block = content[0];
	CHECK(block["type"] == Variant("text"));
	const Dictionary payload = JSON::parse_string(block["text"]);
	CHECK(payload["pong"] == Variant(true));
}

TEST_CASE("[MCP] a command that fails reports in band, not as a JSON-RPC error") {
	Ref<MCPProtocol> protocol;
	protocol.instantiate();

	Dictionary arguments;
	arguments["path"] = "Nowhere";
	Dictionary params;
	params["name"] = "get_node";
	params["arguments"] = arguments;
	Dictionary request = make_request("tools/call", 5);
	request["params"] = params;

	bool has_response = false;
	const Dictionary response = protocol->handle_rpc(request, has_response);
	CHECK_FALSE(response.has("error"));
	const Dictionary result = response["result"];
	CHECK(result["isError"] == Variant(true));
}

TEST_CASE("[MCP] HTTP routing answers each method the way the transport expects") {
	Ref<MCPServer> server;
	server.instantiate();
	const Dictionary headers;

	SUBCASE("a path other than /mcp is not found") {
		const Dictionary out = server->route("POST", "/other", headers, "");
		CHECK(out["code"] == Variant(404));
	}
	SUBCASE("preflight is answered with no content") {
		const Dictionary out = server->route("OPTIONS", "/mcp", headers, "");
		CHECK(out["code"] == Variant(204));
	}
	SUBCASE("there is no server-initiated stream") {
		const Dictionary out = server->route("GET", "/mcp", headers, "");
		CHECK(out["code"] == Variant(405));
	}
	SUBCASE("sessions cannot be terminated because there are none") {
		const Dictionary out = server->route("DELETE", "/mcp", headers, "");
		CHECK(out["code"] == Variant(405));
	}
	SUBCASE("an unparseable body is a JSON-RPC parse error, not an HTTP one") {
		const Dictionary out = server->route("POST", "/mcp", headers, "{not json");
		CHECK(out["code"] == Variant(200));
		const Dictionary body = JSON::parse_string(out["body"]);
		const Dictionary error = body["error"];
		CHECK(int(error["code"]) == -32700);
	}
	SUBCASE("a notification is accepted with no body") {
		const Dictionary out = server->route("POST", "/mcp", headers,
				"{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}");
		CHECK(out["code"] == Variant(202));
	}
	SUBCASE("an unsupported protocol version is rejected") {
		Dictionary versioned;
		versioned["mcp-protocol-version"] = "1999-01-01";
		const Dictionary out = server->route("POST", "/mcp", versioned, "");
		CHECK(out["code"] == Variant(400));
	}
	SUBCASE("an event-stream client gets SSE framing") {
		Dictionary sse;
		sse["accept"] = "text/event-stream";
		const Dictionary out = server->route("POST", "/mcp", sse,
				"{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
		CHECK(out["ctype"] == Variant("text/event-stream"));
		CHECK(String(out["body"]).begins_with("event: message"));
	}
}

TEST_CASE("[SceneTree][MCP] commands read and change the scene they are given") {
	Ref<MCPCommands> commands;
	commands.instantiate();

	Node3D *root = memnew(Node3D);
	root->set_name("Root");
	SceneTree::get_singleton()->get_root()->add_child(root);
	commands->set_root(root);

	SUBCASE("the tree comes back rooted at '.'") {
		const Dictionary tree = commands->dispatch("get_scene_tree", Dictionary());
		CHECK(tree["path"] == Variant("."));
		CHECK(tree["name"] == Variant("Root"));
	}

	SUBCASE("a node is created, found, and deleted") {
		Dictionary create;
		create["parent"] = ".";
		create["type"] = "Node3D";
		create["name"] = "Child";
		const Dictionary created = commands->dispatch("create_node", create);
		CHECK(created["created"] == Variant("Child"));

		Dictionary find;
		find["name"] = "Child";
		const Dictionary found = commands->dispatch("find_nodes", find);
		const Array nodes = found["nodes"];
		CHECK(nodes.size() == 1);

		Dictionary del;
		del["path"] = "Child";
		const Dictionary deleted = commands->dispatch("delete_node", del);
		CHECK(deleted["deleted"] == Variant(true));
	}

	SUBCASE("the scene root refuses to be deleted") {
		Dictionary del;
		del["path"] = ".";
		const Dictionary out = commands->dispatch("delete_node", del);
		CHECK(out["__error__"] == Variant(true));
	}

	SUBCASE("a property round-trips through the tagged-dictionary form") {
		Dictionary set;
		set["path"] = ".";
		set["property"] = "position";
		Dictionary value;
		value["__t__"] = "Vector3";
		value["x"] = 1.0;
		value["y"] = 2.0;
		value["z"] = 3.0;
		set["value"] = value;
		commands->dispatch("set_property", set);

		CHECK(root->get_position() == Vector3(1, 2, 3));

		Dictionary get;
		get["path"] = ".";
		get["property"] = "position";
		const Dictionary read = commands->dispatch("get_property", get);
		const Dictionary read_value = read["value"];
		CHECK(read_value["__t__"] == Variant("Vector3"));
		CHECK(double(read_value["y"]) == doctest::Approx(2.0));
	}

	SUBCASE("a method the caller must not reach is blocked") {
		Dictionary call;
		call["path"] = ".";
		call["method"] = "queue_free";
		const Dictionary out = commands->dispatch("call_method", call);
		CHECK(out["__error__"] == Variant(true));
	}

	SUBCASE("an unknown command says so") {
		const Dictionary out = commands->dispatch("no_such_command", Dictionary());
		CHECK(out["__error__"] == Variant(true));
	}

	memdelete(root);
}

} // namespace TestMCP
