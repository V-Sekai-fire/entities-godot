import Lake
open Lake DSL

package «weft-crash-search» where
  -- Pure Lean. The search drives `prove_crash`, which CMake builds.

require «plausible-witness-dag» from git
  "https://github.com/V-Sekai-fire/plausible-witness-dag" @ "main/main"

@[default_target] lean_exe «crash-search» where
  root := `CrashSearch

lean_exe frontier where
  root := `Frontier
