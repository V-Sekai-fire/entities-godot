/**************************************************************************/
/*  mcp_protocol.h                                                        */
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
