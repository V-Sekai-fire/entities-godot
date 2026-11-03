import CassieAvbd.CycleDetect.Arrangement
import CassieAvbd.CycleDetect.Walk
import CassieAvbd.CycleDetect.Fixtures.HatStrokes

/-!
Witness fixture: run `findCyclesPort` on the hat stroke fixture and
pin the current count. The Unity reference cumulative count is 234;
this fixture asserts the Lean walk lands strictly above the legacy
single-plane count (38) and above the pre-Phase-B.0 baseline (50), so
regressions on items 1-4 (per-step parallel transport, cross-node
transport, sharp `GetInPlane` branch, `ShouldReverse` flip at cos60)
fail native_decide instead of drifting silently.
-/
namespace CassieAvbd.CycleDetect.Fixtures

open CassieAvbd.CycleDetect

private def hatArrangement : Graph :=
  buildArrangementAugmented hatStrokes #[] 0.0017 0.0017 8

def hatCyclePortCount : Nat := (findCyclesPort hatArrangement).size

/-- Cycle count `findCyclesPort` returns on the hat fixture. Pinned at
    65 — legacy PCA-plane walk lands at 38, the pre-Phase-B.0 baseline
    landed at 50, and Unity's cumulative reference is 234. -/
theorem hatCyclePortCount_pinned : hatCyclePortCount = 65 := by
  native_decide

/-- Progress witness: the port walk exceeds the pre-Phase-B.0
    baseline. Rule 2 (`check that passes on known-broken input is
    decoration`) is satisfied by pinning the exact value above; this
    is the readable form. -/
theorem hatCyclePortCount_beats_baseline : hatCyclePortCount > 50 := by
  native_decide

end CassieAvbd.CycleDetect.Fixtures
