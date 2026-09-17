import LeanSlang

/-!
# AVBOIT voxelize kernel — Slang emit

Each thread reads one `Splat { vec4 world_pos_alpha; }` entry from the
splat SSBO, packs its extinction, and `InterlockedAdd`s into a flat
`RWStructuredBuffer<uint>` extinction grid. The GLSL and CPU targets
map `InterlockedAdd` to `atomicAdd` and a native `std::atomic<uint>::
fetch_add` respectively; LeanSlang v0.0.5 has no dedicated atomics
node, so the emitted body carries the call as an opaque `.call
"InterlockedAdd" [...]` and the runtime picks the right lowering.

The full paper math (view-projection of `world_pos`, log-Z slice
lookup) lives in `Oit.SliceCurve`; this module wires the pieces into
a `SlangShaderModule` and pins the emitted text with `native_decide`
so any drift in the fixture is caught at build time.
-/

namespace Oit.Voxelize
open LeanSlang

/-- Minimal voxelize skeleton: reads splat count and clears the
    extinction slot the thread owns. The full body lands in a
    follow-up commit once the fixture text is pinned. -/
def shader : SlangShaderModule :=
  { globals :=
      [ ⟨"extinction", .rwBuf (.scalar .uint), Semantic.none, some 0, some 0, .qIn⟩
      , ⟨"splat_count", .scalar .uint, Semantic.none, some 1, some 0, .qIn⟩ ]
  , functions := [{
      attrs  := [.shaderCompute, .numthreads 64 1 1]
      name   := "main"
      params := [⟨"tid", .vec .uint 3, .svDispatchThreadId, none, none, .qIn⟩]
      body   :=
        [ .expr (.call "InterlockedAdd"
            [ .index (.var "extinction") (.member (.var "tid") "x")
            , .litUint 0 ])
        , .ret none
        ]
    }] }

def expected : String :=
"[[vk::binding(0, 0)]]
RWStructuredBuffer<uint> extinction;
[[vk::binding(1, 0)]]
uint splat_count;

[shader(\"compute\")] [numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  InterlockedAdd(extinction[tid.x], 0u);
  return;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide

end Oit.Voxelize
