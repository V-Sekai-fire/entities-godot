import Lake
open Lake DSL

package «oit» where
  leanOptions := #[⟨`autoImplicit, false⟩]

require LeanSlang from git
  "https://github.com/V-Sekai-fire/lean-slang.git" @ "v0.0.5"

@[default_target]
lean_lib Oit where
  roots := #[`Oit]

lean_exe emit_shaders where
  root := `EmitShaders
