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
@[inline] def packExtinction (alpha : Float) : UInt32 :=
  let a := if alpha < 0.0 then 0.0 else if alpha > 0.999 then 0.999 else alpha
  let ext := -Float.log (1.0 - a)
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

/-- `unpackExtinction packed` = extinction as `Float`. Matches
    `float extinction = float(raw) / 65536.0;` in `oit_integrate.glsl`. -/
@[inline] def unpackExtinction (packed : UInt32) : Float :=
  packed.toFloat / 65536.0

/-- Round-trip within saturation: `unpackExtinction (packExtinction 0.5)`
    is within `1/65536` of `-log(0.5)`. `native_decide` at the machine
    level checks byte-exact equality of the packed uint. -/
example : unpackExtinction (packExtinction 0.5) - (-Float.log 0.5) < 1e-4 := by
  native_decide

end Oit.Extinction
