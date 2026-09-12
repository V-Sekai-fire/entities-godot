extends Node
## Runtime MCP bridge — the SAME MCP server as the editor plugin (mcp_bridge.gd),
## but running INSIDE the deployed game (e.g. on the Quest 3) over its live
## SceneTree instead of the editor. Reach it over `adb forward`. Editor-only
## commands return an error; scene/node/eval/screenshot/performance work on the
## running game.
const MCPCommandsLib = preload("mcp_commands.gd")
const MCPHttpServerLib = preload("mcp_http_server.gd")

const HOST := "127.0.0.1"   # adb forward reaches the device loopback

## Not 8788. The editor plugin listens there, and the ordinary way to run a game is to press
## play in an editor that is already open — so sharing the number meant the runtime bridge lost
## the bind every time it was most wanted, and said only "listen failed" while an MCP client
## went on talking to the editor and believing it was talking to the game.
const HTTP_PORT_DEFAULT := 8789

## `--mcp-port=NNNN` after `--`, or GODOT_MCP_PORT. Two games at once need two ports, and a
## device reached over `adb forward` may have neither free.
static func _resolve_port() -> int:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--mcp-port="):
			return int(arg.split("=", true, 1)[1])
	var env := OS.get_environment("GODOT_MCP_PORT")
	if not env.is_empty():
		return int(env)
	return HTTP_PORT_DEFAULT

var _cmds = MCPCommandsLib.new()
var _http = MCPHttpServerLib.new()

func _ready() -> void:
	var port := _resolve_port()
	_http.protocol.commands = _cmds
	if _http.start(port, HOST) == OK:
		print("[godot_mcp] RUNTIME MCP on http://%s:%d/mcp" % [HOST, port])
	else:
		# Say what is probably holding it. A bare "listen failed" sent everyone to the firewall,
		# and it was almost always the editor plugin or a previous run of the same game.
		push_error("[godot_mcp] runtime HTTP listen failed on :%d — something else holds it"
			% port + " (the editor plugin uses 8788). Set --mcp-port= or GODOT_MCP_PORT.")
	set_process(true)

func _process(_delta: float) -> void:
	_cmds.editor = null
	_cmds.root = get_tree().current_scene if get_tree().current_scene else get_tree().root
	_http.poll()

func _exit_tree() -> void:
	_http.stop()
