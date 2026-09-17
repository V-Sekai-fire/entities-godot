/-!
# AVBOIT log-Z slice curve

`sliceUV near far linearisation viewZ` = the UV-space Z coordinate a
fragment at view-space depth `viewZ` lands on inside the transmittance
3D texture. Both the compute (voxelize) and the fragment (scene shader)
must agree on this function byte-for-byte — the debugging session
attributed the null-modulation to lookup / write disagreement here.

The function is defined on `Float32` because the emitted GLSL runs on
IEEE-754 single-precision floats. Real-number reasoning would need a
lifted spec; this file keeps everything at the machine level so a
`native_decide` sample-vs-sample comparison against the C++ helper is
exact.
-/

namespace Oit.SliceCurve

/-- `clamp01 x` clamps `x` into `[0, 1]`. Matches GLSL `clamp(x, 0, 1)`
    and the C++ `oit_clamp01` in `modules/oit/oit_math.h`. -/
@[inline] def clamp01 (x : Float32) : Float32 :=
  if x < 0.0 then 0.0
  else if x > 1.0 then 1.0
  else x

/-- `sliceUV near far linearisation viewZ` — UV-space Z coordinate. -/
@[inline] def sliceUV
    (near far linearisation viewZ : Float32) : Float32 :=
  let k := if linearisation < 0.001 then 0.001 else linearisation
  let span := if far - near < 0.001 then 0.001 else far - near
  let linearZ := clamp01 ((viewZ - near) / span)
  Float32.log (1.0 + k * linearZ) / Float32.log (1.0 + k)

/-- `depthToSlice` — integer slice index. Matches
    `modules/oit/oit_math.h::oit_depth_to_slice`. -/
@[inline] def depthToSlice
    (near far linearisation : Float32) (sliceCount : UInt32) (viewZ : Float32) : UInt32 :=
  let t := sliceUV near far linearisation viewZ
  let scaled := t * sliceCount.toFloat32
  let upper := sliceCount.toFloat32 - 1.0
  let clamped := if scaled < 0.0 then 0.0
                 else if scaled > upper then upper
                 else scaled
  clamped.toUInt32

/-! ## Invariants pinned by `native_decide`

`Float32 =` is not `Decidable` in Lean 4 core, so equality checks
compare `Float32.toBits` (a `UInt32`) instead — byte-exact. -/

/-- `sliceUV` sends the near plane to 0. -/
example : (sliceUV 0.1 500.0 0.5 0.1).toBits = (0.0 : Float32).toBits := by
  native_decide

/-- `sliceUV` sends the far plane to 1. -/
example : (sliceUV 0.1 500.0 0.5 500.0).toBits = (1.0 : Float32).toBits := by
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

/-! ## Sixteen-point table

Sixteen `viewZ` samples through the frustum at each linearisation
the project setting is likely to see. The C++ `oit_depth_to_slice`
pins the same integers in `tests/test_oit.h`. -/

def table16 : Array Float32 :=
  #[0.15, 0.3, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0,
    32.0, 64.0, 128.0, 256.0, 350.0, 420.0, 470.0, 495.0]

def slices (k : Float32) : Array UInt32 :=
  table16.map (depthToSlice 0.1 500.0 k 128)

/-- k = 0.5, the value first shipped as the default. Everything inside
    4 m lands in slice 0: at this `k` the curve is linear to within
    one slice, which is why the default moved to 1000. -/
example : slices 0.5
    = #[0, 0, 0, 0, 0, 1, 2, 4, 9, 19, 38, 71, 94, 110, 121, 126] := by
  native_decide

/-- k = 1000, the shipped default. -/
example : slices 1000.0
    = #[1, 6, 10, 19, 29, 40, 52, 64, 77, 90, 102, 115, 121, 124, 126, 127] := by
  native_decide

/-- k = 100. -/
example : slices 100.0
    = #[0, 1, 2, 4, 8, 15, 26, 39, 55, 72, 90, 109, 118, 123, 126, 127] := by
  native_decide

/-- k = 10000. -/
example : slices 10000.0
    = #[9, 22, 30, 40, 50, 60, 70, 80, 89, 99, 109, 118, 123, 125, 127, 127] := by
  native_decide

/-- Every table is non-decreasing in `viewZ`. -/
def nonDecreasing (xs : Array UInt32) : Bool :=
  (List.range (xs.size - 1)).all fun i => xs[i]! ≤ xs[i + 1]!

example : nonDecreasing (slices 0.5) ∧ nonDecreasing (slices 100.0)
        ∧ nonDecreasing (slices 1000.0) ∧ nonDecreasing (slices 10000.0) := by
  native_decide

/-- Control: the flat curve is a pinned failure, not a silent default.
    At k = 0.5, 1 m and 2 m share a slice; at k = 1000 they do not. -/
example : depthToSlice 0.1 500.0 0.5 128 1.0 = depthToSlice 0.1 500.0 0.5 128 2.0 := by
  native_decide
example : depthToSlice 0.1 500.0 1000.0 128 1.0 < depthToSlice 0.1 500.0 1000.0 128 2.0 := by
  native_decide

/-- Control: a table with one swapped pair is rejected by `nonDecreasing`. -/
example : nonDecreasing #[0, 5, 4, 9] = false := by native_decide

/-! ## Linearisation sweep at seven depths -/

example :
    let d (z : Float32) := depthToSlice 0.1 500.0 0.05 128 z
    #[ d 1.0, d 10.0, d 50.0, d 100.0, d 250.0, d 400.0, d 495.0 ]
    = #[0, 2, 13, 26, 64, 102, 126] := by native_decide

example :
    let d (z : Float32) := depthToSlice 0.1 500.0 2.0 128 z
    #[ d 1.0, d 10.0, d 50.0, d 100.0, d 250.0, d 400.0, d 495.0 ]
    = #[0, 4, 21, 39, 80, 111, 127] := by native_decide

/-! ## Slice-count doubling

Same `viewZ` and `(near, far, k)`; doubling `sliceCount` doubles the
index exactly when `t * n` has no fractional carry, which it does not
at these points. -/

example :
    let s64  := depthToSlice 0.1 500.0 0.5 64  100.0
    let s128 := depthToSlice 0.1 500.0 0.5 128 100.0
    let s256 := depthToSlice 0.1 500.0 0.5 256 100.0
    s64 = 15 ∧ s128 = 30 ∧ s256 = 60 := by native_decide

/-! ## Adversarial edge cases

Negative viewZ, zero linearisation and a near = far frustum resolve
to a bounded slice, never NaN and never a wrapped unsigned value. -/

example : depthToSlice 0.1 500.0 0.5 128 (-1.0) = 0 := by native_decide
example : depthToSlice 0.1 500.0 0.5 128 (-100.0) = 0 := by native_decide

example : depthToSlice 0.1 500.0 0.0 128 0.1 = 0 := by native_decide
example : depthToSlice 0.1 500.0 0.0 128 500.0 = 127 := by native_decide

/-- near = far: `viewZ - near = 0` so `linearZ = 0` and the slice is 0. -/
example : depthToSlice 5.0 5.0 0.5 128 5.0 = 0 := by native_decide
example : depthToSlice 5.0 5.0 0.5 128 6.0 = 127 := by native_decide

end Oit.SliceCurve
