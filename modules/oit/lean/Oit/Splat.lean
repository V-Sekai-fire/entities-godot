import Oit.SliceCurve
import Oit.Extinction

/-!
# AVBOIT raster-into-froxel splat

The paper's slide 51 rasterises every transparent surface into the
froxel grid: the scene shader's depth-only variant runs over the
transparent list into an empty framebuffer sized to the grid's `xy`,
and each fragment `atomicAdd`s its packed extinction into the froxel
owning `(gl_FragCoord.xy, depthToSlice(-vertex.z))`.

Two facts carry the design and are pinned here:

1. A froxel's total is the sum, over the fragments the rasteriser
   put there, of `packExtinction alpha`; nothing is counted twice,
   and the order fragments arrive in does not matter.
2. The framebuffer is at froxel resolution, not screen resolution.
   Drawing at screen resolution puts `tile.x * tile.y` fragments in
   every covered column and over-counts the surface by that factor.
-/

namespace Oit.Splat

open Oit.SliceCurve (depthToSlice)
open Oit.Extinction (Dims flatIndex packExtinction)

structure Curve where
  near : Float32
  far : Float32
  k : Float32

/-- One rasterised fragment: `gl_FragCoord.xy` at froxel resolution,
    view-space depth, and the material alpha after the fragment
    stage's scissor, hash and edge processing. -/
structure Fragment where
  x : UInt32
  y : UInt32
  viewZ : Float32
  alpha : Float32

def cellCount (d : Dims) : Nat := (d.x * d.y * d.z).toNat

def froxelOf (c : Curve) (d : Dims) (f : Fragment) : UInt32 :=
  flatIndex d f.x f.y (depthToSlice c.near c.far c.k d.z f.viewZ)

/-- Whether the fragment reaches the buffer: the shader early-outs
    behind the eye. -/
def inFront (f : Fragment) : Bool := f.viewZ > 0.0

/-- The raster pass as a fold: each fragment adds once, atomically. -/
def splat (c : Curve) (d : Dims) (frags : Array Fragment) : Array UInt32 :=
  frags.foldl (init := Array.replicate (cellCount d) 0) fun grid f =>
    if inFront f then
      grid.modify (froxelOf c d f).toNat (· + packExtinction f.alpha)
    else
      grid

/-- The reference: for every froxel, the sum over the fragments that
    land in it. -/
def reference (c : Curve) (d : Dims) (frags : Array Fragment) : Array UInt32 :=
  (Array.range (cellCount d)).map fun i =>
    frags.foldl (init := 0) fun acc f =>
      if inFront f && (froxelOf c d f).toNat == i then
        acc + packExtinction f.alpha
      else
        acc

def curve : Curve := ⟨0.1, 100.0, 1000.0⟩
def dims : Dims := ⟨4, 3, 8⟩

/-- Fragments from three overlapping half-alpha boxes seen through the
    same column, one column shared by two boxes at the same depth, a
    fragment behind the eye, and one at the far corner. -/
def frags : Array Fragment := #[
  ⟨1, 1, 1.0, 0.5⟩, ⟨1, 1, 3.0, 0.5⟩, ⟨1, 1, 9.0, 0.5⟩,
  ⟨2, 0, 3.0, 0.25⟩, ⟨2, 0, 3.0, 0.75⟩,
  ⟨0, 2, -1.0, 0.9⟩,
  ⟨3, 2, 99.0, 0.1⟩]

example : splat curve dims frags = reference curve dims frags := by native_decide

/-- Order independence: the reverse list lands the same grid. -/
example : splat curve dims frags.reverse = splat curve dims frags := by native_decide

/-- A shared froxel holds the packed sum of both fragments. -/
example :
    (splat curve dims frags)[(froxelOf curve dims ⟨2, 0, 3.0, 0.0⟩).toNat]!
      = packExtinction 0.25 + packExtinction 0.75 := by native_decide

/-- The three boxes in column `(1, 1)` land in three distinct slices. -/
example :
    let s := fun z => depthToSlice 0.1 100.0 1000.0 8 z
    s 1.0 = 2 ∧ s 3.0 = 3 ∧ s 9.0 = 5 := by native_decide

/-- A fragment behind the eye contributes nothing. -/
example :
    splat curve dims (frags.filter inFront) = splat curve dims frags := by native_decide

/-- The grid total equals the packed sum over the fragments in front. -/
example :
    (splat curve dims frags).foldl (· + ·) 0
      = (frags.filter inFront).foldl (fun acc f => acc + packExtinction f.alpha) 0 := by
  native_decide

/-- Control: a pass that adds each fragment twice disagrees with the
    reference. -/
def splatTwice (c : Curve) (d : Dims) (frags : Array Fragment) : Array UInt32 :=
  splat c d (frags ++ frags)

example : splatTwice curve dims frags ≠ reference curve dims frags := by native_decide

/-! ## Framebuffer resolution

A surface covering a set of froxel columns produces one fragment per
column when the framebuffer is `froxel_dims.xy`. At screen resolution
the same surface produces `tile.x * tile.y` fragments per column, and
the column's extinction is that many times too large: at `alpha 0.5`
under a `6 x 6` tile the column reads as transmittance `0.5 ^ 36`
rather than `0.5`. -/

def columnFragments (tile : Dims) (x y : UInt32) (viewZ alpha : Float32) : Array Fragment :=
  (Array.range (tile.x * tile.y).toNat).map fun _ => ⟨x, y, viewZ, alpha⟩

def tile : Dims := ⟨6, 6, 1⟩

example :
    (splat curve dims (columnFragments tile 1 1 3.0 0.5))[(froxelOf curve dims ⟨1, 1, 3.0, 0.0⟩).toNat]!
      = 36 * packExtinction 0.5 := by native_decide

example :
    (splat curve dims #[⟨1, 1, 3.0, 0.5⟩])[(froxelOf curve dims ⟨1, 1, 3.0, 0.0⟩).toNat]!
      = packExtinction 0.5 := by native_decide

end Oit.Splat
