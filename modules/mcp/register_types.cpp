/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#include "register_types.h"

#include "mcp_commands.h"
#include "mcp_protocol.h"
#include "mcp_server.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/mcp_editor_plugin.h"
#endif

void initialize_mcp_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_INTERNAL_CLASS(MCPCommands);
		GDREGISTER_INTERNAL_CLASS(MCPProtocol);
		GDREGISTER_INTERNAL_CLASS(MCPServer);
		// The editor plugin listens here; the runtime bridge takes the next port
		// so a game started from an open editor does not lose the bind.
		GLOBAL_DEF_BASIC(PropertyInfo(Variant::INT, "editor/mcp/port", PROPERTY_HINT_RANGE, "1024,65535,1"), MCPServer::DEFAULT_PORT);
	}
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorPlugins::add_by_type<MCPEditorPlugin>();
	}
#endif
}

void uninitialize_mcp_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
