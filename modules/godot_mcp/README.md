# transport-godot-mcp

Drive the **Godot editor** from an MCP client — the Godot-side counterpart to
[IvanMurzak/Unity-MCP](https://github.com/IvanMurzak/Unity-MCP) (`com.ivanmurzak.unity.mcp`),
so the Godot host can be exercised the same way as the Unity host for the two-implementation
interop check.

## Using it in another project

This repository is a Godot project wrapping the addon, so its `plugin.cfg` sits two levels
down at `addons/vsekai_godot_mcp/plugin.cfg`. Godot only scans `res://addons/*/plugin.cfg`, so
vendoring the repository whole puts the addon one level too deep and the editor never finds it.
The `addon-root` branch is that directory and nothing else:

```sh
git subtree add --prefix addons/vsekai_godot_mcp \
  https://github.com/v-sekai-multiplayer-fabric/transport-godot-mcp.git addon-root --squash
```

It is produced from `main` and carries no history of its own:

```sh
git subtree split --prefix=addons/vsekai_godot_mcp -b addon-root
git push -f origin addon-root
```

**Re-split it after every change to the addon**, or consumers keep vendoring an older one. The
branch is a view of `main` rather than a place to commit: a change made on `addon-root` is lost
by the next split, and nothing warns about it. `transport-client` consumes it this way.

## Two ports

The editor plugin listens on **8788**. The runtime bridge — `mcp_runtime.gd`, autoloaded into
the running game — listens on **8789**, and takes `--mcp-port=NNNN` after `--` or
`GODOT_MCP_PORT`.

They must differ. Pressing play in an open editor is the ordinary case, and when both wanted
8788 the game lost the bind, printed `listen failed`, and an MCP client went on questioning the
editor while believing it had reached the game.
