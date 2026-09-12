/**************************************************************************/
/*  mcp_commands.cpp                                                      */
/**************************************************************************/

#include "mcp_commands.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "main/performance.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/gui/rich_text_label.h"
#include "scene/main/viewport.h"
#include "scene/resources/packed_scene.h"
#include "servers/rendering/rendering_server.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_interface.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"
#endif

Dictionary MCPCommands::_err(const String &p_msg) {
	Dictionary d;
	d["__error__"] = true;
	d["msg"] = p_msg;
	return d;
}

Dictionary MCPCommands::_ok() {
	Dictionary d;
	d["ok"] = true;
	return d;
}

Node *MCPCommands::_resolve(const String &p_path) const {
	if (root == nullptr) {
		return nullptr;
	}
	if (p_path.is_empty() || p_path == "." || p_path == "/root" || p_path == String(root->get_name())) {
		return root;
	}
	return root->get_node_or_null(NodePath(p_path));
}

Dictionary MCPCommands::_node_brief(Node *p_node) const {
	Dictionary d;
	if (p_node == nullptr) {
		return d;
	}
	d["name"] = String(p_node->get_name());
	d["type"] = p_node->get_class();
	Ref<Script> script = p_node->get_script();
	d["script"] = script.is_valid() ? script->get_path() : String();
	return d;
}

Dictionary MCPCommands::_node_tree(Node *p_node, int p_depth) const {
	Dictionary d = _node_brief(p_node);
	d["path"] = p_node == root ? String(".") : String(root->get_path_to(p_node));
	Array kids;
	if (p_depth > 0) {
		for (int i = 0; i < p_node->get_child_count(); i++) {
			kids.push_back(_node_tree(p_node->get_child(i), p_depth - 1));
		}
	}
	d["children"] = kids;
	return d;
}

// Variants that JSON cannot carry travel as tagged dictionaries; matrix and
// bounds types go through JSON.from_native so the layout cannot drift.
Variant MCPCommands::to_json(const Variant &p_value, int p_depth) const {
	if (p_depth > 32) {
		return String("<max-depth>");
	}

	switch (p_value.get_type()) {
		case Variant::NIL:
		case Variant::BOOL:
		case Variant::INT:
		case Variant::FLOAT:
		case Variant::STRING:
			return p_value;

		case Variant::STRING_NAME:
		case Variant::NODE_PATH:
			return String(p_value);

		case Variant::VECTOR2: {
			Vector2 v = p_value;
			Dictionary d;
			d["__t__"] = "Vector2";
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR3: {
			Vector3 v = p_value;
			Dictionary d;
			d["__t__"] = "Vector3";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::VECTOR4: {
			Vector4 v = p_value;
			Dictionary d;
			d["__t__"] = "Vector4";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			d["w"] = v.w;
			return d;
		}
		case Variant::VECTOR2I: {
			Vector2i v = p_value;
			Dictionary d;
			d["__t__"] = "Vector2i";
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR3I: {
			Vector3i v = p_value;
			Dictionary d;
			d["__t__"] = "Vector3i";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::COLOR: {
			Color c = p_value;
			Dictionary d;
			d["__t__"] = "Color";
			d["r"] = c.r;
			d["g"] = c.g;
			d["b"] = c.b;
			d["a"] = c.a;
			return d;
		}
		case Variant::QUATERNION: {
			Quaternion q = p_value;
			Dictionary d;
			d["__t__"] = "Quaternion";
			d["x"] = q.x;
			d["y"] = q.y;
			d["z"] = q.z;
			d["w"] = q.w;
			return d;
		}

		case Variant::BASIS:
		case Variant::TRANSFORM2D:
		case Variant::TRANSFORM3D:
		case Variant::AABB:
		case Variant::PLANE:
		case Variant::RECT2:
		case Variant::PROJECTION:
			return JSON::from_native(p_value);

		case Variant::ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY: {
			Array in = p_value;
			Array out;
			for (int i = 0; i < in.size(); i++) {
				out.push_back(to_json(in[i], p_depth + 1));
			}
			return out;
		}

		case Variant::DICTIONARY: {
			Dictionary in = p_value;
			Dictionary out;
			for (const Variant &key : in.keys()) {
				out[String(key)] = to_json(in[key], p_depth + 1);
			}
			return out;
		}

		case Variant::OBJECT: {
			Object *obj = p_value;
			if (obj == nullptr) {
				return Variant();
			}
			Dictionary d;
			Node *node = Object::cast_to<Node>(obj);
			if (node != nullptr) {
				d["__t__"] = "Node";
				d["path"] = root != nullptr ? String(root->get_path_to(node)) : String();
				d["class"] = node->get_class();
				return d;
			}
			Resource *res = Object::cast_to<Resource>(obj);
			if (res != nullptr) {
				d["__t__"] = "Resource";
				d["path"] = res->get_path();
				d["class"] = res->get_class();
				return d;
			}
			d["__t__"] = "Object";
			d["class"] = obj->get_class();
			return d;
		}

		default:
			return String(p_value);
	}
}

Variant MCPCommands::coerce(const Variant &p_value, Variant::Type p_target) {
	if (p_value.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_value;
		if (d.has("__t__")) {
			const String tag = d["__t__"];
			if (tag == "Vector2") {
				return Vector2(d.get("x", 0), d.get("y", 0));
			}
			if (tag == "Vector3") {
				return Vector3(d.get("x", 0), d.get("y", 0), d.get("z", 0));
			}
			if (tag == "Vector4") {
				return Vector4(d.get("x", 0), d.get("y", 0), d.get("z", 0), d.get("w", 0));
			}
			if (tag == "Vector2i") {
				return Vector2i(d.get("x", 0), d.get("y", 0));
			}
			if (tag == "Vector3i") {
				return Vector3i(d.get("x", 0), d.get("y", 0), d.get("z", 0));
			}
			if (tag == "Color") {
				return Color(d.get("r", 0), d.get("g", 0), d.get("b", 0), d.get("a", 1));
			}
			if (tag == "Quaternion") {
				return Quaternion(d.get("x", 0), d.get("y", 0), d.get("z", 0), d.get("w", 1));
			}
		}
		if (d.has("type") && d.has("args")) {
			return JSON::to_native(d);
		}
	}

	if (p_value.get_type() == Variant::ARRAY) {
		Array a = p_value;
		if (p_target == Variant::VECTOR3 && a.size() == 3) {
			return Vector3(a[0], a[1], a[2]);
		}
		if (p_target == Variant::VECTOR2 && a.size() == 2) {
			return Vector2(a[0], a[1]);
		}
		if (p_target == Variant::COLOR && a.size() >= 3) {
			return Color(a[0], a[1], a[2], a.size() > 3 ? (real_t)a[3] : 1.0f);
		}
	}
	return p_value;
}

PackedStringArray MCPCommands::_scan_ext(const String &p_dir, const String &p_ext, PackedStringArray p_out) {
	Ref<DirAccess> dir = DirAccess::open(p_dir);
	if (dir.is_null()) {
		return p_out;
	}
	dir->list_dir_begin();
	String name = dir->get_next();
	while (!name.is_empty()) {
		if (name != "." && name != "..") {
			const String full = p_dir.path_join(name);
			if (dir->current_is_dir()) {
				p_out = _scan_ext(full, p_ext, p_out);
			} else if (name.ends_with(p_ext)) {
				p_out.push_back(full);
			}
		}
		name = dir->get_next();
	}
	dir->list_dir_end();
	return p_out;
}

Dictionary MCPCommands::dispatch(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "ping") {
		Dictionary d;
		d["pong"] = true;
		d["engine"] = Engine::get_singleton()->get_version_info();
		return d;
	}

	Dictionary out = _scene_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	out = _reflection_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	out = _spatial_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	out = _file_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	out = _diagnostic_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	out = _script_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	out = _editor_commands(p_cmd, p_args);
	if (!out.is_empty()) {
		return out;
	}
	return _err("unknown cmd: " + p_cmd);
}

Dictionary MCPCommands::_scene_commands(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "get_scene_tree") {
		if (root == nullptr) {
			return _err("no scene root");
		}
		int depth = p_args.has("max_depth") ? (int)p_args["max_depth"] : 64;
		return _node_tree(root, CLAMP(depth, 0, 256));
	}

	if (p_cmd == "get_node") {
		const String path = p_args.get("path", "");
		Node *node = _resolve(path);
		if (node == nullptr) {
			return _err("node not found: " + path);
		}
		List<PropertyInfo> props;
		node->get_property_list(&props);
		Dictionary values;
		for (const PropertyInfo &info : props) {
			if (info.usage & PROPERTY_USAGE_EDITOR) {
				values[info.name] = to_json(node->get(info.name));
			}
		}
		Dictionary brief = _node_brief(node);
		brief["properties"] = values;
		return brief;
	}

	if (p_cmd == "get_property") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Dictionary d;
		d["value"] = to_json(node->get(StringName(p_args.get("property", ""))));
		return d;
	}

	if (p_cmd == "set_property") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		const StringName prop = StringName(p_args.get("property", ""));
		node->set(prop, coerce(p_args.get("value", Variant()), node->get(prop).get_type()));
		Dictionary d;
		d["value"] = to_json(node->get(prop));
		return d;
	}

	if (p_cmd == "call_method") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		const String method = p_args.get("method", "");
		if (method == "free" || method == "queue_free" || method == "set_owner") {
			return _err("method '" + method + "' is blocked via call_method (use delete_node)");
		}
		if (!node->has_method(StringName(method))) {
			return _err("no such method: " + method);
		}
		Array raw = p_args.get("args", Array());
		Array args;
		for (int i = 0; i < raw.size(); i++) {
			args.push_back(coerce(raw[i], Variant::NIL));
		}
		Dictionary d;
		d["value"] = to_json(node->callv(StringName(method), args));
		return d;
	}

	if (p_cmd == "list_methods" || p_cmd == "list_properties") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Array members;
		if (p_cmd == "list_methods") {
			List<MethodInfo> methods;
			node->get_method_list(&methods);
			for (const MethodInfo &info : methods) {
				Dictionary m;
				m["name"] = info.name;
				m["args"] = info.arguments.size();
				members.push_back(m);
			}
		} else {
			List<PropertyInfo> props;
			node->get_property_list(&props);
			for (const PropertyInfo &info : props) {
				if (info.usage & PROPERTY_USAGE_EDITOR) {
					Dictionary m;
					m["name"] = info.name;
					m["type"] = info.type;
					members.push_back(m);
				}
			}
		}
		Dictionary d;
		d["members"] = members;
		return d;
	}

	if (p_cmd == "create_node") {
		Node *parent = _resolve(p_args.get("parent", ""));
		if (parent == nullptr) {
			return _err("parent not found");
		}
		const String type = p_args.get("type", "Node");
		if (!ClassDB::class_exists(type) || !ClassDB::can_instantiate(type)) {
			return _err("cannot instantiate type: " + type);
		}
		if (!ClassDB::is_parent_class(type, "Node")) {
			return _err("type is not a Node: " + type);
		}
		Node *node = Object::cast_to<Node>(ClassDB::instantiate(type));
		if (node == nullptr) {
			return _err("instantiate returned no Node: " + type);
		}
		node->set_name(String(p_args.get("name", type)));
		parent->add_child(node);
		node->set_owner(root);
		Dictionary d;
		d["created"] = String(root->get_path_to(node));
		return d;
	}

	if (p_cmd == "delete_node") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		if (node == root) {
			return _err("refusing to delete the scene root");
		}
		node->get_parent()->remove_child(node);
		node->queue_free();
		Dictionary d;
		d["deleted"] = true;
		return d;
	}

	if (p_cmd == "reparent_node") {
		Node *node = _resolve(p_args.get("path", ""));
		Node *new_parent = _resolve(p_args.get("new_parent", ""));
		if (node == nullptr || new_parent == nullptr) {
			return _err("node or new_parent not found");
		}
		Node3D *spatial = Object::cast_to<Node3D>(node);
		if (spatial != nullptr && (bool)p_args.get("keep_global_transform", true)) {
			spatial->reparent(new_parent, true);
		} else {
			node->get_parent()->remove_child(node);
			new_parent->add_child(node);
			node->set_owner(root);
		}
		Dictionary d;
		d["reparented"] = String(root->get_path_to(node));
		return d;
	}

	if (p_cmd == "duplicate_node") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		if (node == root) {
			return _err("cannot duplicate the scene root");
		}
		Node *copy = node->duplicate();
		node->get_parent()->add_child(copy);
		copy->set_owner(root);
		Dictionary d;
		d["duplicated"] = String(root->get_path_to(copy));
		return d;
	}

	if (p_cmd == "move_child") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Node *parent = node->get_parent();
		if (parent == nullptr) {
			return _err("node has no parent");
		}
		const int index = CLAMP((int)p_args.get("to_index", 0), 0, MAX(0, parent->get_child_count() - 1));
		parent->move_child(node, index);
		Dictionary d;
		d["index"] = node->get_index();
		return d;
	}

	if (p_cmd == "add_to_group" || p_cmd == "remove_from_group") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		const StringName group = StringName(p_args.get("group", ""));
		if (p_cmd == "add_to_group") {
			node->add_to_group(group, true);
		} else {
			node->remove_from_group(group);
		}
		return _ok();
	}

	if (p_cmd == "get_nodes_in_group") {
		if (root == nullptr) {
			return _err("no scene root");
		}
		const StringName group = StringName(p_args.get("group", ""));
		Array nodes;
		if (root->is_in_group(group)) {
			nodes.push_back(".");
		}
		TypedArray<Node> found = root->find_children("*", "", true, false);
		for (int i = 0; i < found.size(); i++) {
			Node *node = Object::cast_to<Node>(found[i]);
			if (node != nullptr && node->is_in_group(group)) {
				nodes.push_back(String(root->get_path_to(node)));
			}
		}
		Dictionary d;
		d["nodes"] = nodes;
		return d;
	}

	if (p_cmd == "find_nodes") {
		if (root == nullptr) {
			return _err("no scene root");
		}
		String pattern = p_args.get("name", "*");
		if (pattern.is_empty()) {
			pattern = "*";
		}
		TypedArray<Node> found = root->find_children(pattern, String(p_args.get("type", "")), true, false);
		Array nodes;
		for (int i = 0; i < found.size(); i++) {
			Node *node = Object::cast_to<Node>(found[i]);
			if (node == nullptr) {
				continue;
			}
			Dictionary entry;
			entry["path"] = String(root->get_path_to(node));
			entry["type"] = node->get_class();
			entry["name"] = String(node->get_name());
			nodes.push_back(entry);
		}
		Dictionary d;
		d["nodes"] = nodes;
		return d;
	}

	if (p_cmd == "get_node_count") {
		Dictionary d;
		d["count"] = root == nullptr ? 0 : root->get_child_count(true) + 1;
		return d;
	}

	if (p_cmd == "set_script") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Ref<Resource> script = ResourceLoader::load(String(p_args.get("script", "")));
		if (script.is_null()) {
			return _err("could not load script: " + String(p_args.get("script", "")));
		}
		node->set_script(script);
		return _ok();
	}

	return Dictionary();
}

Dictionary MCPCommands::_reflection_commands(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "class_exists") {
		Dictionary d;
		d["exists"] = ClassDB::class_exists(StringName(p_args.get("class", "")));
		return d;
	}

	if (p_cmd == "get_class_methods" || p_cmd == "get_class_properties") {
		const StringName cls = StringName(p_args.get("class", ""));
		if (!ClassDB::class_exists(cls)) {
			return _err("no such class: " + String(cls));
		}
		const bool no_inherit = p_args.get("no_inheritance", false);
		Array members;
		if (p_cmd == "get_class_methods") {
			List<MethodInfo> methods;
			ClassDB::get_method_list(cls, &methods, no_inherit);
			for (const MethodInfo &info : methods) {
				members.push_back(info.name);
			}
		} else {
			List<PropertyInfo> props;
			ClassDB::get_property_list(cls, &props, no_inherit);
			for (const PropertyInfo &info : props) {
				members.push_back(info.name);
			}
		}
		Dictionary d;
		d["members"] = members;
		return d;
	}

	if (p_cmd == "list_signals") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		List<MethodInfo> signals;
		node->get_signal_list(&signals);
		Array names;
		for (const MethodInfo &info : signals) {
			names.push_back(info.name);
		}
		Dictionary d;
		d["signals"] = names;
		return d;
	}

	if (p_cmd == "emit_signal") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Array args;
		args.push_back(String(p_args.get("signal", "")));
		Array raw = p_args.get("args", Array());
		for (int i = 0; i < raw.size(); i++) {
			args.push_back(coerce(raw[i], Variant::NIL));
		}
		node->callv(SNAME("emit_signal"), args);
		return _ok();
	}

	if (p_cmd == "connect_signal") {
		Node *node = _resolve(p_args.get("path", ""));
		Node *target = _resolve(p_args.get("target", ""));
		if (node == nullptr || target == nullptr) {
			return _err("node or target not found");
		}
		const Error err = node->connect(StringName(p_args.get("signal", "")),
				Callable(target, StringName(p_args.get("method", ""))));
		return err == OK ? _ok() : _err("connect failed (" + itos(err) + ")");
	}

	if (p_cmd == "call_singleton") {
		const String name = p_args.get("singleton", "");
		if (!Engine::get_singleton()->has_singleton(name)) {
			return _err("no such singleton: " + name);
		}
		Object *singleton = Engine::get_singleton()->get_singleton_object(name);
		const StringName method = StringName(p_args.get("method", ""));
		if (singleton == nullptr || !singleton->has_method(method)) {
			return _err("singleton has no method: " + String(method));
		}
		Array raw = p_args.get("args", Array());
		Array args;
		for (int i = 0; i < raw.size(); i++) {
			args.push_back(coerce(raw[i], Variant::NIL));
		}
		Dictionary d;
		d["value"] = to_json(singleton->callv(method, args));
		return d;
	}

	return Dictionary();
}

Dictionary MCPCommands::_spatial_commands(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "get_transform") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Node3D *spatial = Object::cast_to<Node3D>(node);
		if (spatial == nullptr) {
			return _err("node is not a Node3D: " + node->get_class());
		}
		Dictionary d;
		d["local"] = to_json(spatial->get_transform());
		d["global"] = to_json(spatial->get_global_transform());
		d["global_position"] = to_json(spatial->get_global_position());
		d["global_rotation_deg"] = to_json(spatial->get_global_rotation_degrees());
		d["scale"] = to_json(spatial->get_scale());
		return d;
	}

	if (p_cmd == "get_aabb") {
		Node *node = _resolve(p_args.get("path", "."));
		if (node == nullptr) {
			return _err("node not found");
		}
		AABB merged;
		bool have = false;
		List<Node *> stack;
		stack.push_back(node);
		while (!stack.is_empty()) {
			Node *current = stack.back()->get();
			stack.pop_back();
			VisualInstance3D *visual = Object::cast_to<VisualInstance3D>(current);
			if (visual != nullptr) {
				const AABB world = visual->get_global_transform().xform(visual->get_aabb());
				merged = have ? merged.merge(world) : world;
				have = true;
			}
			for (int i = 0; i < current->get_child_count(); i++) {
				stack.push_back(current->get_child(i));
			}
		}
		if (!have) {
			return _err("no VisualInstance3D under: " + String(p_args.get("path", ".")));
		}
		Dictionary d;
		d["min"] = to_json(merged.position);
		d["max"] = to_json(merged.get_end());
		d["size"] = to_json(merged.size);
		d["center"] = to_json(merged.get_center());
		return d;
	}

	if (p_cmd == "get_bone_poses") {
		Node *node = _resolve(p_args.get("path", ""));
		if (node == nullptr) {
			return _err("node not found");
		}
		Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
		if (skeleton == nullptr) {
			return _err("node is not a Skeleton3D: " + node->get_class());
		}
		Array only = p_args.get("bones", Array());
		Array bones;
		for (int i = 0; i < skeleton->get_bone_count(); i++) {
			const String name = skeleton->get_bone_name(i);
			if (only.size() > 0 && !only.has(name)) {
				continue;
			}
			Dictionary bone;
			bone["index"] = i;
			bone["name"] = name;
			bone["parent"] = skeleton->get_bone_parent(i);
			bone["rest"] = to_json(skeleton->get_bone_rest(i));
			bone["pose_local"] = to_json(skeleton->get_bone_pose(i));
			bone["global_pose"] = to_json(skeleton->get_bone_global_pose(i));
			bones.push_back(bone);
		}
		Dictionary d;
		d["skeleton"] = String(skeleton->get_name());
		d["bone_count"] = skeleton->get_bone_count();
		d["bones"] = bones;
		return d;
	}

	return Dictionary();
}

Dictionary MCPCommands::_file_commands(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "read_file") {
		const String path = p_args.get("path", "");
		if (!FileAccess::exists(path)) {
			return _err("file not found: " + path);
		}
		Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
		if (file.is_null()) {
			return _err("cannot open: " + path);
		}
		Dictionary d;
		d["text"] = file->get_as_text();
		return d;
	}

	if (p_cmd == "write_file" || p_cmd == "create_script") {
		const String path = p_args.get("path", "");
		Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
		if (file.is_null()) {
			return _err("cannot open for write: " + path);
		}
		file->store_string(String(p_cmd == "write_file" ? p_args.get("text", "") : p_args.get("source", "extends Node\n")));
		file->close();
		if (p_cmd == "write_file") {
			return _ok();
		}
		Dictionary d;
		d["created"] = path;
		return d;
	}

	if (p_cmd == "file_exists") {
		const String path = p_args.get("path", "");
		Dictionary d;
		d["exists"] = FileAccess::exists(path) || DirAccess::exists(path);
		return d;
	}

	if (p_cmd == "delete_file") {
		const Error err = DirAccess::remove_absolute(ProjectSettings::get_singleton()->globalize_path(String(p_args.get("path", ""))));
		return err == OK ? _ok() : _err("delete failed (" + itos(err) + ")");
	}

	if (p_cmd == "copy_file" || p_cmd == "move_file") {
		const String from = ProjectSettings::get_singleton()->globalize_path(String(p_args.get("from", "")));
		const String to = ProjectSettings::get_singleton()->globalize_path(String(p_args.get("to", "")));
		const Error err = p_cmd == "copy_file" ? DirAccess::copy_absolute(from, to) : DirAccess::rename_absolute(from, to);
		return err == OK ? _ok() : _err(p_cmd + " failed (" + itos(err) + ")");
	}

	if (p_cmd == "make_dir") {
		const Error err = DirAccess::make_dir_recursive_absolute(
				ProjectSettings::get_singleton()->globalize_path(String(p_args.get("path", ""))));
		return err == OK ? _ok() : _err("mkdir failed (" + itos(err) + ")");
	}

	if (p_cmd == "list_dir") {
		const String path = p_args.get("path", "res://");
		Ref<DirAccess> dir = DirAccess::open(path);
		if (dir.is_null()) {
			return _err("cannot open dir: " + path);
		}
		Array files;
		Array dirs;
		dir->list_dir_begin();
		String name = dir->get_next();
		while (!name.is_empty()) {
			if (name != "." && name != "..") {
				if (dir->current_is_dir()) {
					dirs.push_back(name);
				} else {
					files.push_back(name);
				}
			}
			name = dir->get_next();
		}
		dir->list_dir_end();
		Dictionary d;
		d["files"] = files;
		d["dirs"] = dirs;
		return d;
	}

	if (p_cmd == "find_files" || p_cmd == "list_scenes") {
		const String dir = p_cmd == "list_scenes" ? String("res://") : String(p_args.get("root", "res://"));
		const String ext = p_cmd == "list_scenes" ? String(".tscn") : String(p_args.get("ext", ""));
		Dictionary d;
		d[p_cmd == "list_scenes" ? "scenes" : "files"] = _scan_ext(dir, ext);
		return d;
	}

	if (p_cmd == "create_resource") {
		const StringName cls = StringName(p_args.get("class", ""));
		if (!ClassDB::class_exists(cls) || !ClassDB::can_instantiate(cls) || !ClassDB::is_parent_class(cls, "Resource")) {
			return _err("not an instantiable Resource: " + String(cls));
		}
		Ref<Resource> res = Object::cast_to<Resource>(ClassDB::instantiate(cls));
		const Error err = ResourceSaver::save(res, String(p_args.get("path", "")));
		if (err != OK) {
			return _err("save failed (" + itos(err) + ")");
		}
		Dictionary d;
		d["created"] = String(p_args.get("path", ""));
		return d;
	}

	if (p_cmd == "create_scene") {
		const StringName type = StringName(p_args.get("root_type", "Node"));
		if (!ClassDB::can_instantiate(type) || !ClassDB::is_parent_class(type, "Node")) {
			return _err("cannot instantiate as a Node: " + String(type));
		}
		Node *scene_root = Object::cast_to<Node>(ClassDB::instantiate(type));
		if (scene_root == nullptr) {
			return _err("instantiate returned no Node: " + String(type));
		}
		scene_root->set_name(String(p_args.get("root_name", String(type))));
		Ref<PackedScene> packed;
		packed.instantiate();
		if (packed->pack(scene_root) != OK) {
			memdelete(scene_root);
			return _err("pack failed");
		}
		const Error err = ResourceSaver::save(packed, String(p_args.get("path", "")));
		memdelete(scene_root);
		if (err != OK) {
			return _err("save failed (" + itos(err) + ")");
		}
		Dictionary d;
		d["created"] = String(p_args.get("path", ""));
		return d;
	}

	if (p_cmd == "instance_scene") {
		if (root == nullptr) {
			return _err("no scene root");
		}
		Ref<PackedScene> scene = ResourceLoader::load(String(p_args.get("scene", "")));
		if (scene.is_null()) {
			return _err("not a PackedScene: " + String(p_args.get("scene", "")));
		}
		Node *parent = _resolve(p_args.get("parent", "."));
		if (parent == nullptr) {
			return _err("parent not found");
		}
		Node *instance = scene->instantiate();
		parent->add_child(instance);
		instance->set_owner(root);
		Dictionary d;
		d["instanced"] = String(root->get_path_to(instance));
		return d;
	}

	if (p_cmd == "save_branch_as_scene") {
		Node *node = _resolve(p_args.get("path", "."));
		if (node == nullptr) {
			return _err("node not found");
		}
		Ref<PackedScene> packed;
		packed.instantiate();
		if (packed->pack(node) != OK) {
			return _err("pack failed");
		}
		const Error err = ResourceSaver::save(packed, String(p_args.get("scene", "")));
		if (err != OK) {
			return _err("save failed (" + itos(err) + ")");
		}
		Dictionary d;
		d["saved"] = String(p_args.get("scene", ""));
		return d;
	}

	if (p_cmd == "get_project_setting") {
		Dictionary d;
		d["value"] = to_json(ProjectSettings::get_singleton()->get_setting(String(p_args.get("setting", "")), p_args.get("default", Variant())));
		return d;
	}

	if (p_cmd == "set_project_setting") {
		ProjectSettings::get_singleton()->set_setting(String(p_args.get("setting", "")), coerce(p_args.get("value", Variant()), Variant::NIL));
		return _ok();
	}

	return Dictionary();
}

Dictionary MCPCommands::_diagnostic_commands(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "get_performance") {
		if (Performance::get_singleton() == nullptr) {
			return _err("no Performance singleton in this context");
		}
		struct MonitorName {
			const char *key;
			Performance::Monitor monitor;
		};
		static const MonitorName monitors[] = {
			{ "fps", Performance::TIME_FPS },
			{ "frame_time", Performance::TIME_PROCESS },
			{ "physics_frame_time", Performance::TIME_PHYSICS_PROCESS },
			{ "static_memory", Performance::MEMORY_STATIC },
			{ "object_count", Performance::OBJECT_COUNT },
			{ "node_count", Performance::OBJECT_NODE_COUNT },
			{ "resource_count", Performance::OBJECT_RESOURCE_COUNT },
			{ "draw_calls", Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME },
			{ "video_memory", Performance::RENDER_VIDEO_MEM_USED },
		};
		Array want = p_args.get("monitors", Array());
		Dictionary values;
		for (const MonitorName &entry : monitors) {
			if (want.size() > 0 && !want.has(String(entry.key))) {
				continue;
			}
			values[entry.key] = Performance::get_singleton()->get_monitor(entry.monitor);
		}
		Dictionary d;
		d["monitors"] = values;
		return d;
	}

	if (p_cmd == "list_monitors") {
		Array names;
		names.push_back("fps");
		names.push_back("frame_time");
		names.push_back("physics_frame_time");
		names.push_back("static_memory");
		names.push_back("object_count");
		names.push_back("node_count");
		names.push_back("resource_count");
		names.push_back("draw_calls");
		names.push_back("video_memory");
		Dictionary d;
		d["monitors"] = names;
		return d;
	}

	if (p_cmd == "get_memory_info") {
		Dictionary d;
		d["static"] = (int64_t)OS::get_singleton()->get_static_memory_usage();
		d["static_peak"] = (int64_t)OS::get_singleton()->get_static_memory_peak_usage();
		return d;
	}

	if (p_cmd == "get_os_info") {
		Dictionary d;
		d["name"] = OS::get_singleton()->get_name();
		d["model"] = OS::get_singleton()->get_model_name();
		d["locale"] = OS::get_singleton()->get_locale();
		d["processor"] = OS::get_singleton()->get_processor_name();
		d["threads"] = OS::get_singleton()->get_processor_count();
		return d;
	}

	if (p_cmd == "get_video_info") {
		if (RenderingServer::get_singleton() == nullptr) {
			return _err("no RenderingServer in this context");
		}
		Dictionary d;
		d["adapter"] = RenderingServer::get_singleton()->get_video_adapter_name();
		d["vendor"] = RenderingServer::get_singleton()->get_video_adapter_vendor();
		d["api"] = RenderingServer::get_singleton()->get_video_adapter_api_version();
		return d;
	}

	if (p_cmd == "get_render_info") {
		RenderingServer *server = RenderingServer::get_singleton();
		if (server == nullptr) {
			return _err("no RenderingServer in this context");
		}
		Dictionary d;
		d["objects"] = server->get_rendering_info(RenderingServerEnums::RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME);
		d["primitives"] = server->get_rendering_info(RenderingServerEnums::RENDERING_INFO_TOTAL_PRIMITIVES_IN_FRAME);
		d["draw_calls"] = server->get_rendering_info(RenderingServerEnums::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME);
		d["texture_mem"] = server->get_rendering_info(RenderingServerEnums::RENDERING_INFO_TEXTURE_MEM_USED);
		d["buffer_mem"] = server->get_rendering_info(RenderingServerEnums::RENDERING_INFO_BUFFER_MEM_USED);
		d["video_mem"] = server->get_rendering_info(RenderingServerEnums::RENDERING_INFO_VIDEO_MEM_USED);
		return d;
	}

	return Dictionary();
}

Dictionary MCPCommands::_editor_commands(const String &p_cmd, const Dictionary &p_args) {
	static const char *editor_only[] = { "open_scene", "save_scene", "get_open_scene", "play_scene", "play_main",
		"stop", "is_playing", "reimport_asset", "rescan_filesystem", "get_editor_setting", nullptr };

	bool known = false;
	for (int i = 0; editor_only[i] != nullptr; i++) {
		if (p_cmd == editor_only[i]) {
			known = true;
			break;
		}
	}
	if (!known) {
		return Dictionary();
	}

#ifdef TOOLS_ENABLED
	// The singleton exists in a plain engine run too; only the editor hint says
	// there is an editor behind it, and calling through it without one crashes.
	EditorInterface *editor = EditorInterface::get_singleton();
	if (editor == nullptr || !Engine::get_singleton()->is_editor_hint()) {
		return _err("EditorInterface not available in this context");
	}

	if (p_cmd == "open_scene") {
		editor->open_scene_from_path(String(p_args.get("path", "")));
		return _ok();
	}
	if (p_cmd == "save_scene") {
		editor->save_scene();
		return _ok();
	}
	if (p_cmd == "get_open_scene") {
		Node *edited = editor->get_edited_scene_root();
		Dictionary d;
		d["path"] = edited != nullptr ? edited->get_scene_file_path() : String();
		d["root"] = edited != nullptr ? Variant(_node_brief(edited)) : Variant();
		return d;
	}
	if (p_cmd == "play_scene") {
		const String path = p_args.get("path", "");
		if (path.is_empty()) {
			editor->play_current_scene();
		} else {
			editor->play_custom_scene(path);
		}
		Dictionary d;
		d["playing"] = true;
		return d;
	}
	if (p_cmd == "play_main") {
		editor->play_main_scene();
		Dictionary d;
		d["playing"] = true;
		return d;
	}
	if (p_cmd == "stop") {
		editor->stop_playing_scene();
		Dictionary d;
		d["playing"] = false;
		return d;
	}
	if (p_cmd == "is_playing") {
		Dictionary d;
		d["playing"] = editor->is_playing_scene();
		return d;
	}
	if (p_cmd == "reimport_asset") {
		PackedStringArray paths;
		Array raw = p_args.get("paths", Array());
		for (int i = 0; i < raw.size(); i++) {
			paths.push_back(String(raw[i]));
		}
		const String single = p_args.get("path", "");
		if (!single.is_empty()) {
			paths.push_back(single);
		}
		if (paths.is_empty()) {
			return _err("no paths given (use 'path' or 'paths')");
		}
		Vector<String> as_vector;
		for (int i = 0; i < paths.size(); i++) {
			as_vector.push_back(paths[i]);
		}
		EditorFileSystem::get_singleton()->reimport_files(as_vector);
		Dictionary d;
		d["reimported"] = paths;
		return d;
	}
	if (p_cmd == "rescan_filesystem") {
		EditorFileSystem::get_singleton()->scan();
		return _ok();
	}
	if (p_cmd == "get_editor_setting") {
		Dictionary d;
		d["value"] = to_json(EditorSettings::get_singleton()->get_setting(String(p_args.get("setting", ""))));
		return d;
	}
#endif

	return _err("command not implemented in the native module: " + p_cmd);
}

// Walk a control tree for the editor's log, which is an EditorLog holding one
// RichTextLabel. Found by class name because EditorLog is not exported.
static RichTextLabel *_find_log_label(Node *p_node, bool p_inside_log) {
	const bool inside = p_inside_log || p_node->get_class() == "EditorLog";
	if (inside) {
		RichTextLabel *label = Object::cast_to<RichTextLabel>(p_node);
		if (label != nullptr) {
			return label;
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		RichTextLabel *found = _find_log_label(p_node->get_child(i), inside);
		if (found != nullptr) {
			return found;
		}
	}
	return nullptr;
}

Dictionary MCPCommands::_script_commands(const String &p_cmd, const Dictionary &p_args) {
	if (p_cmd == "run_script") {
		// A script resource with its source set as text: the snippet becomes the
		// body of run(editor, root) on a throwaway object. It is marked @tool
		// because the editor hands a non-tool script a placeholder instance,
		// whose run() returns null however good the snippet is.
		Object *instantiated = ClassDB::instantiate("GDScript");
		Ref<Script> script = Object::cast_to<Script>(instantiated);
		if (script.is_null()) {
			if (instantiated != nullptr) {
				memdelete(instantiated);
			}
			return _err("no GDScript in this build; cannot run a snippet");
		}

		String indented;
		const Vector<String> lines = String(p_args.get("source", "")).split("\n");
		for (int i = 0; i < lines.size(); i++) {
			indented += "\t" + lines[i] + "\n";
		}
		script->set_source_code("@tool\nextends RefCounted\nfunc run(editor, root):\n" + indented + "\tpass\n");
		if (script->reload() != OK) {
			return _err("script failed to compile");
		}

		Ref<RefCounted> holder;
		holder.instantiate();
		holder->set_script(script);
		if (!holder->has_method("run")) {
			return _err("script defines no run(editor, root)");
		}

		Variant editor_variant;
#ifdef TOOLS_ENABLED
		if (Engine::get_singleton()->is_editor_hint()) {
			editor_variant = EditorInterface::get_singleton();
		}
#endif
		Dictionary d;
		d["value"] = to_json(holder->call("run", editor_variant, root));
		return d;
	}

	if (p_cmd == "read_log") {
		const int wanted = p_args.has("lines") ? (int)p_args["lines"] : 100;
#ifdef TOOLS_ENABLED
		if (EditorInterface::get_singleton() != nullptr && Engine::get_singleton()->is_editor_hint()) {
			RichTextLabel *label = _find_log_label(EditorInterface::get_singleton()->get_base_control(), false);
			if (label != nullptr) {
				const Vector<String> log_lines = label->get_parsed_text().split("\n");
				Array tail;
				for (int i = MAX(0, log_lines.size() - wanted); i < log_lines.size(); i++) {
					tail.push_back(log_lines[i]);
				}
				Dictionary d;
				d["source"] = "editor_output";
				d["lines"] = tail;
				return d;
			}
		}
#endif
		const String path = "user://logs/godot.log";
		if (FileAccess::exists(path)) {
			Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
			if (file.is_valid()) {
				const Vector<String> log_lines = file->get_as_text().split("\n");
				Array tail;
				for (int i = MAX(0, log_lines.size() - wanted); i < log_lines.size(); i++) {
					tail.push_back(log_lines[i]);
				}
				Dictionary d;
				d["source"] = "log_file";
				d["lines"] = tail;
				return d;
			}
		}
		Dictionary d;
		d["lines"] = Array();
		d["note"] = "no EditorLog and file logging disabled";
		return d;
	}

	if (p_cmd == "screenshot") {
		Viewport *viewport = nullptr;
#ifdef TOOLS_ENABLED
		if (EditorInterface::get_singleton() != nullptr && Engine::get_singleton()->is_editor_hint()) {
			viewport = EditorInterface::get_singleton()->get_base_control()->get_viewport();
		}
#endif
		if (viewport == nullptr && root != nullptr) {
			viewport = root->get_viewport();
		}
		if (viewport == nullptr) {
			return _err("no viewport to capture");
		}
		Ref<ViewportTexture> texture = viewport->get_texture();
		Ref<Image> image = texture.is_valid() ? texture->get_image() : Ref<Image>();
		if (image.is_null()) {
			return _err("could not capture viewport");
		}
		const String path = p_args.get("path", "user://godot_mcp_screenshot.png");
		if (image->save_png(path) != OK) {
			return _err("save_png failed");
		}
		Dictionary d;
		d["path"] = ProjectSettings::get_singleton()->globalize_path(path);
		d["width"] = image->get_width();
		d["height"] = image->get_height();
		return d;
	}

	return Dictionary();
}
