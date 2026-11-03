/-! # The read-ahead window

`Prefetch.lean` in weft models *when* to read ahead: the score that decides whether an
access pattern is a scan. It does not model what a prefetch may then answer with, and that
is where the bug lives.

`ParallelCommits.tla` does not cover this either. That spec governs the write side —
intents, the record, recovery — and says nothing about a cache in front of a read. So the
read side needs its own argument, and this is it.

The property is the only one that matters for a cache: **a read through the window returns
exactly what a read without one would.** A prefetch may cost a round trip it did not need.
It may not change an answer.

There are two obligations, and they are different:

* `readVia_fill`, below, is safety for a window filled from the store it is read against.
* `stale_window_lies` is the reason `ra_reset` exists: a window read against a *different*
  store is free to be wrong, so anything that changes the store must drop it.
-/

namespace Weft.ReadAhead

/-- A page's contents. Nothing here depends on what is in one. -/
abbrev Page := Nat

/-- What the store holds, in the shape `page_from_store` reads it.

`owner` is PIDX: the txid that owns a page, if the log holds it. A page with no owner comes
from the shard, and a database with no shard has no fallback at all. -/
structure Store where
  owner : Nat → Option Nat
  delta : Nat → Nat → Option Page
  shard : Nat → Option Page
  hasShard : Bool

/-- One page, read the ordinary way: the index, then the log or the shard. -/
def Store.read (s : Store) (p : Nat) : Option Page :=
  match s.owner p with
  | some tx => s.delta tx p
  | none => if s.hasShard then s.shard p else none

/-- What a prefetch learned about one page.

The three cases are the whole point. `absent` and `unfetched` are not the same claim:
`absent` says the prefetch looked and there was nothing, and `unfetched` says it did not
look. Collapsing them into one flag is what makes a skipped page read as a hole. -/
inductive Slot where
  /-- The prefetch did not cover this page, so the window says nothing about it. -/
  | unfetched
  /-- The prefetch read this page. -/
  | present (page : Page)
  /-- The prefetch covered this page and there was nothing there. -/
  | absent
  deriving Repr, DecidableEq

/-- A window is what one prefetch brought back. -/
structure Window where
  first : Nat
  count : Nat
  slot : Nat → Slot

/-- What `ra_fill` does.

Two range reads. PIDX is keyed by page number, so the owners of a run are contiguous. A
compacted database keeps its pages in SHARD, also keyed by page number, so those are
contiguous too.

A page the log owns is deliberately skipped: DELTA is keyed by txid first, so those are
scattered and are not one range. Skipped means `unfetched`, never `absent`. -/
def fill (s : Store) (first count : Nat) : Window :=
  { first := first
    count := count
    slot := fun p =>
      if p < first ∨ first + count ≤ p then .unfetched
      else match s.owner p with
        | some _ => .unfetched
        | none =>
          if s.hasShard then
            match s.shard p with
            | some pg => .present pg
            | none => .absent
          else .unfetched }

/-- What `page_from_store` does once a window exists: use it when it has an answer, and
fall through to the store when it does not. -/
def readVia (s : Store) (w : Window) (p : Nat) : Option Page :=
  match w.slot p with
  | .present pg => some pg
  | .absent => none
  | .unfetched => s.read p

/-- **Safety.** A window filled from a store never changes what that store reads.

This is what makes read-ahead free to get wrong: guessing badly costs a range read, and
never an answer. -/
theorem readVia_fill (s : Store) (first count p : Nat) :
    readVia s (fill s first count) p = s.read p := by
  unfold readVia fill Store.read
  by_cases hr : p < first ∨ first + count ≤ p
  · simp [hr]
  · simp only [hr, if_false]
    cases ho : s.owner p with
    | some tx => simp [ho]
    | none =>
      cases hs : s.hasShard with
      | false => simp [hs]
      | true =>
        cases hp : s.shard p with
        | some pg => simp [hs, hp]
        | none => simp [hs, hp]

/-- The bug this file was written after.

Answering `absent` for a page the prefetch skipped, rather than `unfetched`. It looks like
a tidier flag and it hands SQLite a zeroed page where the log held the real one. -/
def fillCollapsed (s : Store) (first count : Nat) : Window :=
  { first := first
    count := count
    slot := fun p =>
      if p < first ∨ first + count ≤ p then .unfetched
      else match s.shard p with
        | some pg => .present pg
        | none => .absent }   -- wrong: a log-owned page reaches here and is not a hole

/-- A store holding exactly one page, and holding it in the log rather than the shard.
This is an ordinary database between compactions. -/
def logHeld : Store :=
  { owner := fun p => if p = 0 then some 7 else none
    delta := fun tx p => if tx = 7 ∧ p = 0 then some 42 else none
    shard := fun _ => none
    hasShard := true }

/-- **The bug, as a counterexample.** Collapsing the two cases loses a page that is there.

The store reads page 0 as 42, and a read through the collapsed window reads it as nothing,
which is the zeroed page SQLite then tries to parse. -/
theorem collapsed_loses_a_page :
    readVia logHeld (fillCollapsed logHeld 0 8) 0 ≠ logHeld.read 0 := by
  decide

/-- The same page, through the window this file argues for. -/
example : readVia logHeld (fill logHeld 0 8) 0 = some 42 := by decide

/-- **Staleness.** A window is only safe against the store it was filled from.

Read against a store that has moved on, it is free to be wrong — so every write has to drop
it. That obligation is `ra_reset`, and nothing in `ParallelCommits.tla` implies it, because
that spec governs writes and this is a cache in front of a read.

The witness: fill from a store whose shard holds a page, then read against one where the
log has since overwritten it. -/
def beforeCommit : Store :=
  { owner := fun _ => none
    delta := fun _ _ => none
    shard := fun p => if p = 0 then some 1 else none
    hasShard := true }

def afterCommit : Store :=
  { owner := fun p => if p = 0 then some 9 else none
    delta := fun tx p => if tx = 9 ∧ p = 0 then some 2 else none
    shard := fun p => if p = 0 then some 1 else none
    hasShard := true }

theorem stale_window_lies :
    readVia afterCommit (fill beforeCommit 0 8) 0 ≠ afterCommit.read 0 := by
  decide

/-- And dropping it is enough: with no window, the read is the store's own. -/
def empty : Window := { first := 0, count := 0, slot := fun _ => .unfetched }

theorem dropped_window_is_honest (s : Store) (p : Nat) : readVia s empty p = s.read p := by
  rfl

/-! ## A range read may return less than it was asked for

`fill` above assumes the two range reads cover every page in the window. FoundationDB does
not promise that: a range read answers with what fitted and sets `more`, and a window of 256
pages is a megabyte, which is exactly the size that gets truncated.

So the covered set is a parameter, not the whole range. `covered` is which pages the reads
actually came back with. -/
def fillUpTo (s : Store) (first count : Nat) (covered : Nat → Bool) : Window :=
  { first := first
    count := count
    slot := fun p =>
      if p < first ∨ first + count ≤ p then .unfetched
      else if !covered p then .unfetched      -- the read stopped before this page
      else match s.owner p with
        | some _ => .unfetched
        | none =>
          if s.hasShard then
            match s.shard p with
            | some pg => .present pg
            | none => .absent
          else .unfetched }

/-- **Safety survives truncation, but only because uncovered means `unfetched`.**

Whatever the range read managed to return, the window still cannot change an answer. -/
theorem readVia_fillUpTo (s : Store) (first count : Nat) (covered : Nat → Bool) (p : Nat) :
    readVia s (fillUpTo s first count covered) p = s.read p := by
  unfold readVia fillUpTo Store.read
  by_cases hr : p < first ∨ first + count ≤ p
  · simp [hr]
  · by_cases hc : covered p
    · simp only [hr, hc, if_false, if_true, Bool.not_eq_true']
      cases ho : s.owner p with
      | some tx => simp [ho]
      | none =>
        cases hs : s.hasShard with
        | false => simp [hs]
        | true =>
          cases hp : s.shard p with
          | some pg => simp [hs, hp]
          | none => simp [hs, hp]
    · simp [hr, hc]

/-- The bug a truncated range read causes if `more` is ignored.

Treating "the read did not return this page" as "there is no such page" is the same mistake
as `fillCollapsed`, arriving by a different route: the page is in the shard, the range read
stopped early, and the window calls it a hole. -/
def fillIgnoringMore (s : Store) (first count : Nat) (covered : Nat → Bool) : Window :=
  { first := first
    count := count
    slot := fun p =>
      if p < first ∨ first + count ≤ p then .unfetched
      else match s.owner p with
        | some _ => .unfetched
        | none => if covered p then
                    match s.shard p with
                    | some pg => .present pg
                    | none => .absent
                  else .absent }   -- wrong: not returned is not the same as not there

/-- A shard holding one page, and a read that stopped before reaching it. -/
def shardHeld : Store :=
  { owner := fun _ => none
    delta := fun _ _ => none
    shard := fun p => if p = 3 then some 99 else none
    hasShard := true }

theorem ignoring_more_loses_a_page :
    readVia shardHeld (fillIgnoringMore shardHeld 0 8 (fun p => p < 2)) 3
      ≠ shardHeld.read 3 := by
  decide

end Weft.ReadAhead
