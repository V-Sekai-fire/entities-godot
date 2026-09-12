/**************************************************************************/
/*  mcp_editor_plugin.cpp                                                 */
/**************************************************************************/

#ifdef TOOLS_ENABLED

#include "mcp_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/string/print_string.h"
#include "editor/editor_interface.h"

MCPEditorPlugin::MCPEditorPlugin() {
	server.instantiate();
	set_process(true);
}

void MCPEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			const int port = GLOBAL_GET_CACHED(int, "editor/mcp/port");
			const Error err = server->start(port);
			if (err != OK) {
				print_error(vformat("MCP: could not listen on port %d (error %d)", port, err));
			} else {
				print_line(vformat("MCP: editor bridge listening on 127.0.0.1:%d", port));
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			server->stop();
		} break;

		case NOTIFICATION_PROCESS: {
			if (!server->is_listening()) {
				return;
			}
			// The commands act on whatever scene is open at the moment of the call.
			server->get_protocol()->get_commands()->set_root(EditorInterface::get_singleton()->get_edited_scene_root());
			server->poll();
		} break;

		default:
			break;
	}
}

#endif // TOOLS_ENABLED
