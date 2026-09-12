/**************************************************************************/
/*  mcp_commands.h                                                        */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"
#include "scene/main/node.h"

// Transport-free command logic for the MCP bridge. Every command takes a
// Dictionary of arguments and returns a JSON-able Dictionary; an error is
// { "__error__": true, "msg": ... }, which the protocol turns into an
// isError tool result.
class MCPCommands : public RefCounted {
	GDCLASS(MCPCommands, RefCounted);

	Node *root = nullptr;

	Node *_resolve(const String &p_path) const;
	Dictionary _node_brief(Node *p_node) const;
	Dictionary _node_tree(Node *p_node, int p_depth) const;
	static PackedStringArray _scan_ext(const String &p_dir, const String &p_ext, PackedStringArray p_out = PackedStringArray());
	static Dictionary _err(const String &p_msg);
	static Dictionary _ok();

	Dictionary _scene_commands(const String &p_cmd, const Dictionary &p_args);
	Dictionary _reflection_commands(const String &p_cmd, const Dictionary &p_args);
	Dictionary _file_commands(const String &p_cmd, const Dictionary &p_args);
	Dictionary _spatial_commands(const String &p_cmd, const Dictionary &p_args);
	Dictionary _diagnostic_commands(const String &p_cmd, const Dictionary &p_args);
	Dictionary _script_commands(const String &p_cmd, const Dictionary &p_args);
	Dictionary _editor_commands(const String &p_cmd, const Dictionary &p_args);

public:
	// Public because the property tests round-trip arbitrary Variants through them.
	Variant to_json(const Variant &p_value, int p_depth = 0) const;
	static Variant coerce(const Variant &p_value, Variant::Type p_target);

	void set_root(Node *p_root) { root = p_root; }
	Node *get_root() const { return root; }

	Dictionary dispatch(const String &p_cmd, const Dictionary &p_args);
};
