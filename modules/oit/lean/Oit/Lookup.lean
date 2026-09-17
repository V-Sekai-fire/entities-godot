import Oit.Extinction
import Oit.PrefixSum

/-!
# AVBOIT transmittance lookup

The integrate pass stores an inclusive prefix along each column: slice
`z` holds the transmittance through slices `0..z`. A fragment in slice
`z` reads slice `z - 1`, the transmittance through everything in
front of its own slice, and reads `1` in slice `0`. Reading its own
slice would fold the fragment's own extinction into the result: a lone
half-alpha surface would render at a quarter.

This is what `oit_apply` in `scene_forward_mobile_inc.glsl` samples at
`(float(slice) - 0.5) / slice_count`, the centre of texel `z - 1`. It is
the paper's point-sampling rung (slide 69: a `-1` slice bias); the
linear-splat rung with its `-2` bias is not taken.
-/

namespace Oit.Lookup

open Oit.Extinction (packExtinction unpackExtinction)
open Oit.PrefixSum (prefixSum)

/-- The stored column: `exp(-sum)` of the inclusive prefix. -/
def transmittance (column : Array UInt32) : Array Float32 :=
  (prefixSum (column.map UInt32.toNat)).map fun s => Float32.exp (-(unpackExtinction s.toUInt32))

def lookup (t : Array Float32) (z : UInt32) : Float32 :=
  if z == 0 then 1.0 else t[z.toNat - 1]!

/-- Control: reading the fragment's own slice. -/
def lookupInclusive (t : Array Float32) (z : UInt32) : Float32 := t[z.toNat]!

def near (a b : Float32) : Bool :=
  let d := a - b
  (if d < 0.0 then -d else d) < 1e-4

/-- A lone surface in slice 3 of 8. -/
def lone : Array UInt32 :=
  (Array.replicate 8 0).set! 3 (packExtinction 0.5)

example : near (lookup (transmittance lone) 3) 1.0 := by native_decide
example : near (lookupInclusive (transmittance lone) 3) 0.5 := by native_decide

/-- A surface in slice 2 in front of one in slice 5: the back one sees
    `1 - 0.5`, the front one sees `1`. -/
def pair : Array UInt32 :=
  ((Array.replicate 8 0).set! 2 (packExtinction 0.5)).set! 5 (packExtinction 0.5)

example : near (lookup (transmittance pair) 5) 0.5 := by native_decide
example : near (lookup (transmittance pair) 2) 1.0 := by native_decide

/-- Two surfaces sharing a slice do not see each other. -/
def shared : Array UInt32 :=
  (Array.replicate 8 0).set! 4 (packExtinction 0.5 + packExtinction 0.5)

example : near (lookup (transmittance shared) 4) 1.0 := by native_decide
example : near (lookup (transmittance shared) 5) 0.25 := by native_decide

/-- Slice `0` reads `1` whatever the column holds. -/
example : lookup (transmittance shared) 0 == 1.0 := by native_decide

end Oit.Lookup
