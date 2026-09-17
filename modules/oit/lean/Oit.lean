-- AVBOIT formal specification root.
-- Kernels emit to modules/oit/shaders/*.gen.body via `lake exe emit_shaders`.

import Oit.SliceCurve
import Oit.Extinction
import Oit.PrefixSum
import Oit.Voxelize
import Oit.Integrate

/-!
# Adaptive Volumetric Boundary OIT (Drobot, SIGGRAPH 2025)

The invariants (monotonic slice curve, order-independent packed sum,
Hillis-Steele prefix sum agreement) live under `Oit.SliceCurve`,
`Oit.Extinction`, `Oit.PrefixSum` and are pinned by `native_decide`.

Kernel emitters (`Oit.Voxelize`, `Oit.Integrate`) land in a follow-up
commit once the LeanSlang dependency is fetched and the AST shape is
pinned; the specs above stand alone so `lake build Oit` runs against
just the Lean toolchain.
-/
