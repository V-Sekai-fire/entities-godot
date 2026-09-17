import Oit.Extinction
import Oit.Lookup

/-!
# AVBOIT compositing

The paper's transparency pass (slides 13-14) does not blend over the
background. Every event `i` with premultiplied colour `c_i`, alpha
`a_i` and the transmittance `T_i` in front of it adds, in any order,

    colour += c_i * T_i        alpha += a_i * T_i        ext += -log(1 - a_i)

and a fullscreen resolve composites

    C_f = colour * (1 - exp(-ext)) / alpha + C_0 * exp(-ext)

With exact `T_i` the weights telescope, `alpha = 1 - exp(-ext)`, and
the resolve is the sorted over-blend. With froxel `T_i` the
normalisation keeps the total transmittance exact whatever the slice
lookup got wrong, which is what the third target buys.

The control is what the engine does today: `a_i * T_i` fed into a
sorted over-blend attenuates each event once by the lookup and again
by the blend, so a surface behind another is under-weighted.
-/

namespace Oit.Resolve

open Oit.Extinction (packExtinction)
open Oit.Lookup (transmittance lookup near)

/-- One transparent event: straight colour, alpha, and the slice it
    splats into. Colour is one channel; RGB is three of these. -/
structure Event where
  colour : Float32
  alpha : Float32
  slice : UInt32

/-- Sorted front-to-back over-blend: the reference. -/
def reference (background : Float32) (sorted : Array Event) : Float32 :=
  let (colour, t) := sorted.foldl (init := (0.0, 1.0)) fun (acc, t) e =>
    (acc + e.colour * e.alpha * t, t * (1.0 - e.alpha))
  colour + background * t

/-- What one event adds to the three accumulation targets. -/
structure Accum where
  colour : Float32
  alpha : Float32
  ext : Float32

def Accum.zero : Accum := ⟨0.0, 0.0, 0.0⟩

def accumulate (tIn : Event → Float32) (events : Array Event) : Accum :=
  events.foldl (init := Accum.zero) fun acc e =>
    let t := tIn e
    ⟨acc.colour + e.colour * e.alpha * t, acc.alpha + e.alpha * t,
      acc.ext + -Float32.log (1.0 - e.alpha)⟩

def resolve (background : Float32) (a : Accum) : Float32 :=
  let total := Float32.exp (-a.ext)
  let trans := if a.alpha > 0.0 then a.colour * (1.0 - total) / a.alpha else 0.0
  trans + background * total

/-- The exact transmittance in front of an event: the product over
    the events in strictly nearer slices. -/
def exactFront (events : Array Event) (e : Event) : Float32 :=
  events.foldl (init := 1.0) fun t o =>
    if o.slice < e.slice then t * (1.0 - o.alpha) else t

/-- The froxel column the events splat into, and the lookup through it. -/
def column (slices : Nat) (events : Array Event) : Array UInt32 :=
  events.foldl (init := Array.replicate slices 0) fun col e =>
    col.modify e.slice.toNat (· + packExtinction e.alpha)

def froxelFront (slices : Nat) (events : Array Event) (e : Event) : Float32 :=
  lookup (transmittance (column slices events)) e.slice

/-- Control: the engine today. The lookup scales alpha and a sorted
    back-to-front over-blend composites the result. -/
def overBlend (tIn : Event → Float32) (background : Float32) (sorted : Array Event) : Float32 :=
  sorted.foldr (init := background) fun e acc =>
    let a := e.alpha * tIn e
    e.colour * a + acc * (1.0 - a)

def sortBySlice (events : Array Event) : Array Event :=
  events.qsort (·.slice < ·.slice)

/-- Three half-alpha surfaces of distinct colour in slices 2, 3 and 5
    of 8, over a background of 1. -/
def stack : Array Event := #[⟨0.8, 0.5, 5⟩, ⟨0.2, 0.5, 2⟩, ⟨0.6, 0.5, 3⟩]

def bg : Float32 := 1.0

/-- Sorted reference: `0.5 * 0.2 + 0.25 * 0.6 + 0.125 * 0.8 + 0.125`. -/
example : near (reference bg (sortBySlice stack)) 0.475 := by native_decide

/-- Exact weights reproduce the reference in any order. -/
example : near (resolve bg (accumulate (exactFront stack) stack)) 0.475 := by native_decide
example :
    near (resolve bg (accumulate (exactFront stack) stack.reverse)) 0.475 := by native_decide

/-- Froxel weights reproduce it too when every event has its own slice. -/
example : near (resolve bg (accumulate (froxelFront 8 stack) stack)) 0.475 := by native_decide

/-- Exact weights telescope: the alpha target equals `1 - exp(-ext)`. -/
example :
    let a := accumulate (exactFront stack) stack
    near a.alpha (1.0 - Float32.exp (-a.ext)) := by native_decide

/-- Control: the over-blend with lookup-scaled alpha weights the rear
    surfaces at `0.125` and `0.046875` where the reference has `0.25`
    and `0.125`: `0.5 * 0.2 + 0.125 * 0.6 + 0.046875 * 0.8 + 0.328125`. -/
example :
    near (overBlend (exactFront stack) bg (sortBySlice stack)) 0.540625 := by native_decide
example : !near (overBlend (exactFront stack) bg (sortBySlice stack)) 0.475 := by native_decide

/-- Two events sharing slice 4: the lookup cannot separate them, and
    the paper accepts the weighted mix (slide 85), but the background
    still arrives through the exact total transmittance `0.25`. -/
def shared : Array Event := #[⟨1.0, 0.5, 4⟩, ⟨0.0, 0.5, 4⟩]

example :
    let a := accumulate (froxelFront 8 shared) shared
    near (Float32.exp (-a.ext)) 0.25 := by native_decide

/-- Both orders of the shared pair resolve to the same mix. -/
example :
    near (resolve 0.0 (accumulate (froxelFront 8 shared) shared))
      (resolve 0.0 (accumulate (froxelFront 8 shared) shared.reverse)) := by native_decide

/-- A lone surface resolves to the over-blend: `c * a + bg * (1 - a)`. -/
example :
    near (resolve bg (accumulate (froxelFront 8 #[⟨0.2, 0.5, 3⟩]) #[⟨0.2, 0.5, 3⟩])) 0.6 := by
  native_decide

end Oit.Resolve
