/**************************************************************************/
/*  mcp_protocol.h                                                        */
/**************************************************************************/

#pragma once

#include "mcp_commands.h"

#include "core/object/ref_counted.h"

// JSON-RPC 2.0 / MCP handler with no transport of its own: it turns a decoded
// request into a response Dictionary, or nothing when the message is a
// notification. mcp_server.cpp wraps it in HTTP.
class MCPProtocol : public RefCounted {
	GDCLASS(MCPProtocol, RefCounted);

	Ref<MCPCommands> commands;

	static Dictionary _result(const Variant &p_id, const Variant &p_value);
	static Dictionary _error(const Variant &p_id, int p_code, const String &p_message);
	Dictionary _call_tool(const Variant &p_id, const Variant &p_params);

public:
	static const char *PROTOCOL_VERSION;
	static const char *SERVER_NAME;
	static const char *SERVER_VERSION;

	MCPProtocol();

	Ref<MCPCommands> get_commands() const { return commands; }

	static Array tool_schemas();
	static bool has_tool(const String &p_name);
	static Dictionary parse_error_response(int p_code, const String &p_message);

	// Returns the response, and sets r_has_response false for a notification
	// that needs no reply.
	Dictionary handle_rpc(const Variant &p_request, bool &r_has_response);
};
