/**************************************************************************/
/*  mcp_editor_plugin.cpp                                                 */
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
