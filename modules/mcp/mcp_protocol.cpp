/**************************************************************************/
/*  mcp_protocol.cpp                                                      */
/**************************************************************************/

#include "mcp_protocol.h"

#include "core/io/json.h"

const char *MCPProtocol::PROTOCOL_VERSION = "2025-06-18";
const char *MCPProtocol::SERVER_NAME = "vsekai-godot-mcp";
const char *MCPProtocol::SERVER_VERSION = "0.1.0";

namespace {

struct ToolDef {
	const char *name;
	const char *description;
	// "param:type" pairs; type "any" means an unconstrained schema.
	const char *params;
};

// Tool name == command name; arguments pass straight through to MCPCommands.
const ToolDef TOOL_DEFS[] = {
	{ "ping", "Check the bridge; returns the Godot engine version.", "" },
	{ "get_scene_tree", "Dump the edited scene's node tree.", "max_depth:integer" },
	{ "get_node", "Get a node and its editor properties.", "path:string" },
	{ "get_property", "Read any property of a node by name.", "path:string,property:string" },
	{ "set_property", "Set any property of a node by name.", "path:string,property:string,value:any" },
	{ "call_method", "Call any method on a node by name.", "path:string,method:string,args:array" },
	{ "list_methods", "List a node's callable methods.", "path:string" },
	{ "list_properties", "List a node's editor properties.", "path:string" },
	{ "create_node", "Create a node of a class under a parent.", "parent:string,type:string,name:string" },
	{ "delete_node", "Delete a node (not the root).", "path:string" },
	{ "reparent_node", "Move a node under a new parent.", "path:string,new_parent:string" },
	{ "set_script", "Attach a script (res://…) to a node.", "path:string,script:string" },
	{ "run_script", "Run a GDScript snippet (body of run(editor, root)).", "source:string" },
	{ "open_scene", "Open a scene by res:// path.", "path:string" },
	{ "save_scene", "Save the edited scene.", "" },
	{ "get_open_scene", "Path + root of the edited scene.", "" },
	{ "list_scenes", "List all res:// .tscn scenes.", "" },
	{ "play_scene", "Run the current (or given) scene.", "path:string" },
	{ "play_main", "Run the project's main scene.", "" },
	{ "stop", "Stop the running scene.", "" },
	{ "is_playing", "Whether a scene is playing.", "" },
	{ "get_performance", "Godot Performance monitors (fps, memory, draw calls…).", "monitors:array" },
	{ "read_log", "Read the editor Output console (best-effort).", "lines:integer" },
	{ "screenshot", "Capture the editor viewport to a PNG.", "path:string" },
	{ "list_monitors", "List available Performance monitor keys.", "" },
	{ "get_memory_info", "Static memory usage + peak.", "" },
	{ "get_os_info", "OS name/model/locale/processor/threads.", "" },
	{ "get_video_info", "Video adapter name/vendor/API.", "" },
	{ "get_render_info", "RenderingServer frame stats (objects/primitives/draw calls/mem).", "" },
	{ "copy_file", "Copy a file.", "from:string,to:string" },
	{ "move_file", "Move/rename a file.", "from:string,to:string" },
	{ "make_dir", "Create a directory (recursive).", "path:string" },
	{ "create_resource", "Create + save a Resource of a class.", "class:string,path:string" },
	{ "read_file", "Read a text file (res:// or user://).", "path:string" },
	{ "write_file", "Write text to a file.", "path:string,text:string" },
	{ "file_exists", "Whether a file or dir exists.", "path:string" },
	{ "delete_file", "Delete a file.", "path:string" },
	{ "list_dir", "List files + subdirs of a directory.", "path:string" },
	{ "find_files", "Recursively find files by extension under a root.", "root:string,ext:string" },
	{ "create_script", "Write a new GDScript file.", "path:string,source:string" },
	{ "create_scene", "Create + save a new .tscn with a typed root.", "path:string,root_type:string,root_name:string" },
	{ "instance_scene", "Instance a PackedScene under a parent.", "scene:string,parent:string" },
	{ "save_branch_as_scene", "Pack a node subtree and save it as a .tscn.", "path:string,scene:string" },
	{ "reimport_asset", "Reimport assets after their source changed on disk. Pass 'path' or a 'paths' array of res:// paths.", "path:string,paths:array" },
	{ "rescan_filesystem", "Rescan the project filesystem for added/removed/changed files.", "" },
	{ "get_project_setting", "Read a ProjectSettings value.", "setting:string" },
	{ "set_project_setting", "Set a ProjectSettings value.", "setting:string,value:any" },
	{ "duplicate_node", "Duplicate a node under its parent.", "path:string" },
	{ "move_child", "Reorder a node among its siblings.", "path:string,to_index:integer" },
	{ "add_to_group", "Add a node to a group.", "path:string,group:string" },
	{ "remove_from_group", "Remove a node from a group.", "path:string,group:string" },
	{ "get_nodes_in_group", "Paths of nodes in a group.", "group:string" },
	{ "find_nodes", "Find nodes by name pattern and/or type.", "name:string,type:string" },
	{ "get_node_count", "Count nodes under the scene root.", "" },
	{ "get_transform", "Local and global Transform3D of a Node3D, plus world position, rotation in degrees and scale.", "path:string" },
	{ "get_aabb", "World-space bounding box merged over every VisualInstance3D in a subtree.", "path:string" },
	{ "get_bone_poses", "Dump a Skeleton3D's bones — name, parent, rest, local pose and global pose.", "path:string,bones:array" },
	{ "class_exists", "Whether a Godot class exists.", "class:string" },
	{ "get_class_methods", "Methods of a class (ClassDB).", "class:string,no_inheritance:boolean" },
	{ "get_class_properties", "Properties of a class (ClassDB).", "class:string,no_inheritance:boolean" },
	{ "list_signals", "Signals of a node.", "path:string" },
	{ "emit_signal", "Emit a node signal.", "path:string,signal:string,args:array" },
	{ "connect_signal", "Connect a node signal to a target method.", "path:string,signal:string,target:string,method:string" },
	{ "call_singleton", "Call a method on an Engine singleton (Input, OS…).", "singleton:string,method:string,args:array" },
	{ "get_editor_setting", "Read an EditorSettings value.", "setting:string" },
};

// Arguments that stay optional wherever they appear, and tools whose every
// argument is optional.
const char *OPTIONAL_PARAMS[] = { "max_depth", "name", "args", "monitors", "default", "no_inheritance",
	"lines", "bones", "paths", nullptr };
const char *ALL_OPTIONAL_TOOLS[] = { "play_scene", "screenshot", "get_performance", "find_nodes", "read_log",
	"get_aabb", "reimport_asset", nullptr };

bool in_list(const char *const *p_list, const String &p_value) {
	for (int i = 0; p_list[i] != nullptr; i++) {
		if (p_value == p_list[i]) {
			return true;
		}
	}
	return false;
}

} // namespace

MCPProtocol::MCPProtocol() {
	commands.instantiate();
}

Dictionary MCPProtocol::_result(const Variant &p_id, const Variant &p_value) {
	Dictionary d;
	d["jsonrpc"] = "2.0";
	d["id"] = p_id;
	d["result"] = p_value;
	return d;
}

Dictionary MCPProtocol::_error(const Variant &p_id, int p_code, const String &p_message) {
	Dictionary error;
	error["code"] = p_code;
	error["message"] = p_message;
	Dictionary d;
	d["jsonrpc"] = "2.0";
	d["id"] = p_id;
	d["error"] = error;
	return d;
}

Dictionary MCPProtocol::parse_error_response(int p_code, const String &p_message) {
	return _error(Variant(), p_code, p_message);
}

bool MCPProtocol::has_tool(const String &p_name) {
	for (const ToolDef &tool : TOOL_DEFS) {
		if (p_name == tool.name) {
			return true;
		}
	}
	return false;
}

Array MCPProtocol::tool_schemas() {
	Array out;
	for (const ToolDef &tool : TOOL_DEFS) {
		Dictionary properties;
		Array required;
		const Vector<String> params = String(tool.params).split(",", false);
		for (int i = 0; i < params.size(); i++) {
			const Vector<String> pair = params[i].split(":", false);
			if (pair.size() != 2) {
				continue;
			}
			Dictionary type;
			if (pair[1] != "any") {
				type["type"] = pair[1];
			}
			properties[pair[0]] = type;
			if (!in_list(ALL_OPTIONAL_TOOLS, tool.name) && !in_list(OPTIONAL_PARAMS, pair[0])) {
				required.push_back(pair[0]);
			}
		}
		Dictionary schema;
		schema["type"] = "object";
		schema["properties"] = properties;
		schema["additionalProperties"] = false;
		if (!required.is_empty()) {
			schema["required"] = required;
		}
		Dictionary entry;
		entry["name"] = tool.name;
		entry["description"] = tool.description;
		entry["inputSchema"] = schema;
		out.push_back(entry);
	}
	return out;
}

Dictionary MCPProtocol::_call_tool(const Variant &p_id, const Variant &p_params) {
	if (p_params.get_type() != Variant::DICTIONARY) {
		return _error(p_id, -32602, "invalid params");
	}
	Dictionary params = p_params;
	const String name = params.get("name", "");
	if (!has_tool(name)) {
		return _error(p_id, -32602, "unknown tool: " + name);
	}
	Variant raw_args = params.get("arguments", Dictionary());
	Dictionary args = raw_args.get_type() == Variant::DICTIONARY ? Dictionary(raw_args) : Dictionary();

	const Dictionary result = commands->dispatch(name, args);
	const bool is_error = result.get("__error__", false);

	Dictionary block;
	block["type"] = "text";
	block["text"] = JSON::stringify(is_error ? result.get("msg", "error") : Variant(result));
	Array content;
	content.push_back(block);

	Dictionary payload;
	payload["content"] = content;
	payload["isError"] = is_error;
	return _result(p_id, payload);
}

Dictionary MCPProtocol::handle_rpc(const Variant &p_request, bool &r_has_response) {
	r_has_response = true;

	if (p_request.get_type() != Variant::DICTIONARY) {
		return _error(Variant(), -32600, "invalid request");
	}
	Dictionary request = p_request;

	// A message with no method is a response to a server request. We issue
	// none, so it is accepted without a reply.
	if (!request.has("method")) {
		r_has_response = false;
		return Dictionary();
	}

	const Variant id = request.get("id", Variant());
	const String method = request.get("method", "");

	if (method == "initialize") {
		Dictionary tools;
		tools["listChanged"] = false;
		Dictionary capabilities;
		capabilities["tools"] = tools;
		Dictionary server;
		server["name"] = SERVER_NAME;
		server["version"] = SERVER_VERSION;
		Dictionary payload;
		payload["protocolVersion"] = PROTOCOL_VERSION;
		payload["capabilities"] = capabilities;
		payload["serverInfo"] = server;
		return _result(id, payload);
	}

	if (method == "notifications/initialized" || method == "notifications/cancelled") {
		r_has_response = false;
		return Dictionary();
	}

	if (method == "ping") {
		return _result(id, Dictionary());
	}

	if (method == "tools/list") {
		Dictionary payload;
		payload["tools"] = tool_schemas();
		return _result(id, payload);
	}

	if (method == "tools/call") {
		return _call_tool(id, request.get("params", Dictionary()));
	}

	if (id.get_type() == Variant::NIL) {
		r_has_response = false;
		return Dictionary();
	}
	return _error(id, -32601, "method not found: " + method);
}
