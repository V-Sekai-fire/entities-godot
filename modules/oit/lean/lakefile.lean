import Lake
open Lake DSL

package «oit» where
  leanOptions := #[⟨`autoImplicit, false⟩]

@[default_target]
lean_lib Oit where
  roots := #[`Oit]
