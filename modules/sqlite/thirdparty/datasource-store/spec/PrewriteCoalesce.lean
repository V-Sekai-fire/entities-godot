import ReadAhead

/-! # Rung 4: coalesce the partial-write prerequisite read into the read-ahead window

`fdb_vfs.c:buffer_page` is called from `fdb_write` whenever SQLite writes part of a page.
The bytes already stored have to sit alongside the new ones, so today `buffer_page` opens
a fresh FoundationDB transaction (`run_txn(read_body, ...)`) just to read that one page.
Under YCSB workload F every RMW triggers this: one whole extra transaction round trip
per partial write.

Rung 4 skips the extra round trip when the read-ahead window already covers the page.
`ra_hit` (in `fdb_vfs.c`) can serve the page from the window that a prior read already
fetched, so the partial-write buffer is seeded from memory rather than from the network.

The property that has to hold before the C is edited is `Store.readVia_fillUpTo`
(`ReadAhead.lean:201`): a window filled from a store never changes what that store reads.
`buffer_page` is a caller of the same read path, so if `ra_hit` answers, it answers with
the same bytes a fresh read would have produced.

Correctness turns on window freshness rather than on FDB conflict-set semantics.
`buffer_page` today runs its `read_body` in a *separate* transaction from the write's
`flush`, and the read-conflict set that transaction accumulates is discarded when the
read-only txn ends. So Rung 4 does not weaken any FDB serialisability guarantee — there
was no read-conflict tracking for that read in the write path to begin with. What the
window has to guarantee is that its bytes are the bytes the store holds; `ra_reset` in
`fdb_vfs.c` drops the window on every event that could change the store (a commit, a
fold, a truncate), which is the property `stale_window_lies` in `ReadAhead.lean` names.

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.ReadAhead

/-- The partial-write pre-read, in the abstract: read page `p` and hand its bytes to the
caller so they can be seeded into a dirty-page buffer. The bytes come from the store the
window was filled from. -/
def prewriteRead (s : Store) (p : Nat) : Option Page := s.read p

/-- The Rung 4 version: try the window first, fall back to the store.

This is exactly `readVia` — the window is consulted, and any page the window did not
cover falls through. So the theorem is the same theorem. -/
def prewriteReadCoalesced (s : Store) (w : Window) (p : Nat) : Option Page :=
  readVia s w p

/-- **Rung 4's correctness theorem.** Whatever the window's `first`/`count` and however
the range read that filled it was truncated, coalescing the pre-write read through the
window agrees with reading it fresh. Directly reduces to `readVia_fillUpTo`. -/
theorem coalesced_prewrite_read_agrees
    (s : Store) (first count : Nat) (covered : Nat → Bool) (p : Nat) :
    prewriteReadCoalesced s (fillUpTo s first count covered) p = prewriteRead s p := by
  unfold prewriteReadCoalesced prewriteRead
  exact readVia_fillUpTo s first count covered p

/-- The same result via `fill`, for the `ra_fill` path that does not truncate. -/
theorem coalesced_prewrite_read_agrees_via_fill
    (s : Store) (first count p : Nat) :
    prewriteReadCoalesced s (fill s first count) p = prewriteRead s p := by
  unfold prewriteReadCoalesced prewriteRead
  exact readVia_fill s first count p

/-- The Rung 4 change never *widens* the truth: a page absent from the window is a page
the caller has to fall through for. Equivalent to saying `readVia`'s `unfetched` case is
handled by the store, which is what the definition already does — proved here as a named
theorem so a future refactor cannot silently swap `unfetched` for `absent` on the write
side. Same shape as `ReadAhead.collapsed_loses_a_page` on the read side. -/
theorem coalesced_prewrite_leaves_unfetched_to_the_store (s : Store) (p : Nat) :
    prewriteReadCoalesced s empty p = prewriteRead s p := by
  unfold prewriteReadCoalesced prewriteRead
  exact dropped_window_is_honest s p

/-! ## Rule-2 controls -/

/-- The wrong version: assume the window covers everything the caller wants, and answer
`none` for anything it does not cover. This is the same category error as
`ReadAhead.fillCollapsed`, but on the write side — where it would seed a partial-write
with zeroes and then commit them. -/
def prewriteReadBroken (_s : Store) (w : Window) (p : Nat) : Option Page :=
  match w.slot p with
  | .present pg => some pg
  | .absent => none
  | .unfetched => none    -- wrong: skips the store fall-through

/-- **Rule-2 control.** The broken coalesced pre-read loses a log-held page. Reuses
`logHeld` from `ReadAhead.lean` — the same store the analogous read-side control
uses. -/
theorem broken_prewrite_loses_a_page :
    prewriteReadBroken logHeld (fillUpTo logHeld 0 8 (fun _ => true)) 0 ≠ prewriteRead logHeld 0 := by
  decide

/-- **Rule-2 control** for the stale-window case. If `ra_reset` failed to fire after a
commit, using an old window against the new store lies — same failure as
`ReadAhead.stale_window_lies`, on the write side. -/
theorem stale_window_lies_on_the_write_side :
    prewriteReadCoalesced afterCommit (fill beforeCommit 0 8) 0 ≠ prewriteRead afterCommit 0 := by
  decide

end Weft.ReadAhead
