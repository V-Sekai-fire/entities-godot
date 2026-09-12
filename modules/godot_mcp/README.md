# godot_mcp

Drive the **Godot editor** from an MCP client — the Godot-side counterpart to
[IvanMurzak/Unity-MCP](https://github.com/IvanMurzak/Unity-MCP) (`com.ivanmurzak.unity.mcp`),
so the Godot host can be exercised the same way as the Unity host for the two-implementation
interop check.

## Where it lives

`transport-godot-mcp` was a repository of its own and is archived. This directory is that
repository, history and all, ported here by `git subtree add`; the standalone checkout is
read-only from now on and the addon is maintained at this path.

The directory is not an engine module — no `config.py`, `SCsub` or `register_types.h`, so
`is_module` in `methods.py` skips it and the build never sees it. It sits under `modules/`
because that is where this fork places a thing it owns, the way `modules/game` does.

Godot only scans `res://addons/*/plugin.cfg`, so a project consumes the addon directory
rather than this one:

```sh
git subtree add --prefix addons/vsekai_godot_mcp https://github.com/V-Sekai-fire/entities-godot.git feat/module-godot-mcp --squash
```

## Two ports

The editor plugin listens on **8788**. The runtime bridge — `mcp_runtime.gd`, autoloaded into
the running game — listens on **8789**, and takes `--mcp-port=NNNN` after `--` or
`GODOT_MCP_PORT`.

They must differ. Pressing play in an open editor is the ordinary case, and when both wanted
8788 the game lost the bind, printed `listen failed`, and an MCP client went on questioning the
editor while believing it had reached the game.
