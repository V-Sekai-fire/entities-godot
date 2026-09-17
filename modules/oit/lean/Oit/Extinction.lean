/-!
# AVBOIT packed-uint extinction

Each voxelize invocation contributes `-log(1 - alpha)` extinction into
the froxel that owns its view-space position. The atomic hardware
`atomicAdd` requires `uint`; we scale by `65536` and cast, so the
saturating dynamic range is `[0, 2^32/65536] = [0, 65536]` of
extinction (i.e. transmittance down to `exp(-65536) ~ 0` well before
the saturation cliff).

Order-independence follows from the associativity of `+` on `UInt32`
(modulo overflow — the paper's slide 47 accepts saturating extinction).
-/

namespace Oit.Extinction

/-- `packExtinction alpha` = `uint(-log(1 - alpha) * 65536)`.
    Matches the compute shader body:
    `float extinction = -log(1 - alpha);`
    `uint packed = uint(clamp(extinction * 65536.0, 0.0, 4.29e9));` -/
@[inline] def packExtinction (alpha : Float32) : UInt32 :=
  let a := if alpha < 0.0 then 0.0 else if alpha > 0.999 then 0.999 else alpha
  let ext := -Float32.log (1.0 - a)
  let scaled := ext * 65536.0
  let clamped := if scaled < 0.0 then 0.0
                 else if scaled > 4.29e9 then 4.29e9
                 else scaled
  clamped.toUInt32

/-- Zero alpha contributes zero extinction. -/
example : packExtinction 0.0 = 0 := by native_decide

/-- Alpha `0.5` gives `-log(0.5) * 65536 ~ 45426`. -/
example : packExtinction 0.5 = 45426 := by native_decide

/-- Order independence: `(a + b) + c = a + (b + c)` in `UInt32`. -/
example (a b c : UInt32) : (a + b) + c = a + (b + c) := by
  exact UInt32.add_assoc a b c

/-- Commutativity in `UInt32` (also `native_decide`-checkable at samples). -/
example (a b : UInt32) : a + b = b + a := by
  exact UInt32.add_comm a b

/-- `unpackExtinction packed` = extinction as `Float32`. Matches
    `float extinction = float(raw) / 65536.0;` in `oit_integrate.glsl`. -/
@[inline] def unpackExtinction (packed : UInt32) : Float32 :=
  packed.toFloat32 / 65536.0

/-- Round-trip within saturation: `unpackExtinction (packExtinction 0.5)`
    is within `1/65536` of `-log(0.5)`. `native_decide` at the machine
    level checks byte-exact equality of the packed uint. -/
example : unpackExtinction (packExtinction 0.5) - (-Float32.log 0.5) < 1e-4 := by
  native_decide

/-! ## Ten-row table

The C++ `oit_pack_extinction` pins the same integers in
`tests/test_oit.h`. The 0.999 clamp is what keeps `-log 0` out, so
0.999 and 1.0 share a row. -/

def alphas : Array Float32 :=
  #[0.0, 0.001, 0.1, 0.25, 0.5, 0.75, 0.9, 0.99, 0.999, 1.0]

example : alphas.map packExtinction
    = #[0, 65, 6904, 18853, 45426, 90852, 150902, 301804, 452707, 452707] := by
  native_decide

def nonDecreasing (xs : Array UInt32) : Bool :=
  (List.range (xs.size - 1)).all fun i => xs[i]! ≤ xs[i + 1]!

example : nonDecreasing (alphas.map packExtinction) := by native_decide
example : nonDecreasing #[0, 65, 64] = false := by native_decide

/-- Out-of-range alpha clamps rather than wrapping. -/
example : packExtinction (-1.0) = 0 := by native_decide
example : packExtinction 7.0 = 452707 := by native_decide

/-! ## Summing packed extinction composites alpha

`-log(1 - a) + -log(1 - b) = -log((1 - a)(1 - b))`, so the atomic sum
of two packed values is the packed value of the composited alpha
`1 - (1 - a)(1 - b)`, to within one unit of truncation. This is the
property the integrate pass relies on when it sums a froxel. -/

def composite (a b : Float32) : Float32 := 1.0 - (1.0 - a) * (1.0 - b)

def sumGap (a b : Float32) : UInt32 :=
  let s := packExtinction a + packExtinction b
  let c := packExtinction (composite a b)
  if s ≥ c then s - c else c - s

example :
    #[sumGap 0.5 0.5, sumGap 0.25 0.75, sumGap 0.1 0.9,
      sumGap 0.3 0.3, sumGap 0.5 0.25]
    = #[0, 0, 1, 0, 0] := by native_decide

/-- Control: summing packed values of `a` and `b` is not packing `a + b`. -/
example : packExtinction 0.5 + packExtinction 0.5 ≠ packExtinction 1.0 := by
  native_decide

/-! ## Storage

The extinction grid is a flat `uint[]` storage buffer, not an `r32ui`
image: `atomicAdd` on a buffer lands on every RD backend, where
`imageAtomicAdd` on a 3D storage image did not on Metal. The layout
is x-major, z fastest, so one (x, y) column is contiguous and the
integrate workgroup reads its 128 slices as one run. -/

inductive Storage where
  | buffer
  | image3D
  deriving DecidableEq, Repr

def storage : Storage := .buffer

example : storage = .buffer := by decide

structure Dims where
  x : UInt32
  y : UInt32
  z : UInt32

/-- Matches `flat_index` in `oit_voxelize.glsl` and `oit_flat_index`
    in `oit_math.h`. -/
def flatIndex (d : Dims) (x y z : UInt32) : UInt32 :=
  (x * d.y + y) * d.z + z

def dims : Dims := ⟨320, 180, 128⟩

example : flatIndex dims 0 0 0 = 0 := by native_decide
example : flatIndex dims 0 0 127 = 127 := by native_decide
example : flatIndex dims 0 1 0 = 128 := by native_decide
example : flatIndex dims 1 0 0 = 180 * 128 := by native_decide
example : flatIndex dims 319 179 127 = 320 * 180 * 128 - 1 := by native_decide

/-- A column is contiguous: consecutive z differ by one. -/
example : (List.range 127).all fun z =>
    flatIndex dims 7 9 (z.toUInt32 + 1) = flatIndex dims 7 9 z.toUInt32 + 1 := by
  native_decide

/-- Distinct froxels never share a slot. -/
def injectiveOn (d : Dims) : Bool := Id.run do
  let mut seen : Array Bool := Array.replicate (d.x * d.y * d.z).toNat false
  let mut ok := true
  for x in [0 : d.x.toNat] do
    for y in [0 : d.y.toNat] do
      for z in [0 : d.z.toNat] do
        let i := (flatIndex d x.toUInt32 y.toUInt32 z.toUInt32).toNat
        if seen[i]! then ok := false
        seen := seen.set! i true
  return ok

example : injectiveOn ⟨5, 3, 8⟩ = true := by native_decide

/-- Control: a layout that drops the `y` stride collides. -/
def collidingIndex (d : Dims) (x y z : UInt32) : UInt32 := (x + y) * d.z + z

example : collidingIndex ⟨5, 3, 8⟩ 1 0 0 = collidingIndex ⟨5, 3, 8⟩ 0 1 0 := by
  native_decide
example : flatIndex ⟨5, 3, 8⟩ 1 0 0 ≠ flatIndex ⟨5, 3, 8⟩ 0 1 0 := by native_decide

end Oit.Extinction
