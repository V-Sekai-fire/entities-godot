/**************************************************************************/
/*  mcp_editor_plugin.h                                                   */
/**************************************************************************/

#pragma once

#ifdef TOOLS_ENABLED

#include "../mcp_server.h"

#include "editor/plugins/editor_plugin.h"

// Owns the MCP server inside the editor: starts it on entering the tree and
// pumps it every frame with the edited scene as the command root.
class MCPEditorPlugin : public EditorPlugin {
	GDCLASS(MCPEditorPlugin, EditorPlugin);

	Ref<MCPServer> server;

protected:
	void _notification(int p_what);

public:
	MCPEditorPlugin();
};

#endif // TOOLS_ENABLED
