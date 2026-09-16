/-!
# AVBOIT log-Z slice curve

`sliceUV near far linearisation viewZ` = the UV-space Z coordinate a
fragment at view-space depth `viewZ` lands on inside the transmittance
3D texture. Both the compute (voxelize) and the fragment (scene shader)
must agree on this function byte-for-byte — the debugging session
attributed the null-modulation to lookup / write disagreement here.

The function is defined on `Float` because the emitted GLSL runs on
IEEE-754 single-precision floats. Real-number reasoning would need a
lifted spec; this file keeps everything at the machine level so a
`native_decide` sample-vs-sample comparison against the C++ helper is
exact.
-/

namespace Oit.SliceCurve

/-- `clamp01 x` clamps `x` into `[0, 1]`. Matches GLSL `clamp(x, 0, 1)`
    and the C++ `oit_clamp01` in `modules/oit/oit_math.h`. -/
@[inline] def clamp01 (x : Float) : Float :=
  if x < 0.0 then 0.0
  else if x > 1.0 then 1.0
  else x

/-- `sliceUV near far linearisation viewZ` — UV-space Z coordinate. -/
@[inline] def sliceUV
    (near far linearisation viewZ : Float) : Float :=
  let k := if linearisation < 0.001 then 0.001 else linearisation
  let span := if far - near < 0.001 then 0.001 else far - near
  let linearZ := clamp01 ((viewZ - near) / span)
  Float.log (1.0 + k * linearZ) / Float.log (1.0 + k)

/-- `depthToSlice` — integer slice index. Matches
    `modules/oit/oit_math.h::oit_depth_to_slice`. -/
@[inline] def depthToSlice
    (near far linearisation : Float) (sliceCount : UInt32) (viewZ : Float) : UInt32 :=
  let t := sliceUV near far linearisation viewZ
  let scaled := t * sliceCount.toFloat
  let upper := sliceCount.toFloat - 1.0
  let clamped := if scaled < 0.0 then 0.0
                 else if scaled > upper then upper
                 else scaled
  clamped.toUInt32

/-! ## Invariants pinned by `native_decide`

`Float =` is not `Decidable` in Lean 4 core, so equality checks
compare `Float.toBits` (a `UInt64`) instead — byte-exact. -/

/-- `sliceUV` sends the near plane to 0. -/
example : (sliceUV 0.1 500.0 0.5 0.1).toBits = (0.0 : Float).toBits := by
  native_decide

/-- `sliceUV` sends the far plane to 1. -/
example : (sliceUV 0.1 500.0 0.5 500.0).toBits = (1.0 : Float).toBits := by
  native_decide

/-- Near plane collapses to slice 0. -/
example : depthToSlice 0.1 500.0 0.5 128 0.05 = 0 := by native_decide

/-- Far plane clamps to slice `sliceCount - 1`. -/
example : depthToSlice 0.1 500.0 0.5 128 500.0 = 127 := by native_decide
example : depthToSlice 0.1 500.0 0.5 128 9999.0 = 127 := by native_decide

/-- Near-field gets more slices than far-field.
    `1 -> 100 -> 495 m` must map to strictly increasing slices. -/
example :
    let s0 := depthToSlice 0.1 500.0 0.5 128 1.0
    let s1 := depthToSlice 0.1 500.0 0.5 128 100.0
    let s2 := depthToSlice 0.1 500.0 0.5 128 495.0
    s0 < s1 ∧ s1 < s2 := by native_decide

end Oit.SliceCurve
