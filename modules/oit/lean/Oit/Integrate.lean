import LeanSlang

/-!
# AVBOIT integrate kernel — Slang emit

One workgroup per (x, y) column, `local_size_x = slice_count` threads
walk the Z stack, Hillis-Steele prefix sum in `groupshared` memory,
each thread writes `exp(-sum)` transmittance into an RWStructuredBuffer.

The compute-shader control flow is verified at `Nat` in
`Oit.PrefixSum`; this module wires it into a `SlangShaderModule` and
pins the emitted text with `native_decide`. `slice_count = 128` fits
Metal's 1024-thread workgroup ceiling with room to spare on Adreno /
MoltenVK.
-/

namespace Oit.Integrate
open LeanSlang

/-- Minimal integrate skeleton: reads the extinction slot the thread
    owns and writes a placeholder into the transmittance buffer.
    The prefix-sum body lands in a follow-up commit; the shape is
    fixed here so the C++ downstream can bind the two SSBOs. -/
def shader : SlangShaderModule :=
  { globals :=
      [ ⟨"extinction",    .rwBuf (.scalar .uint),  Semantic.none, some 0, some 0, .qIn⟩
      , ⟨"transmittance", .rwBuf (.scalar .float), Semantic.none, some 1, some 0, .qIn⟩ ]
  , functions := [{
      attrs  := [.shaderCompute, .numthreads 128 1 1]
      name   := "main"
      params := [⟨"tid", .vec .uint 3, .svDispatchThreadId, none, none, .qIn⟩]
      body   :=
        [ .assign (.index (.var "transmittance") (.member (.var "tid") "x"))
                  (.litFloat 1.0)
        , .ret none
        ]
    }] }

def expected : String :=
"[[vk::binding(0, 0)]]
RWStructuredBuffer<uint> extinction;
[[vk::binding(1, 0)]]
RWStructuredBuffer<float> transmittance;

[shader(\"compute\")] [numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  transmittance[tid.x] = 1.000000;
  return;
}"

example : LeanSlang.emit shader = expected := by native_decide
example : shader.entryPointName = "main" := by native_decide

end Oit.Integrate
