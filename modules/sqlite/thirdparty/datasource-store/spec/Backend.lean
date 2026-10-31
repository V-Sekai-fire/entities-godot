import ParallelCommit
import ReadAhead

/-! # What the store underneath has to give

`ParallelCommit.lean` and `ReadAhead.lean` both say "the store" and mean FoundationDB. The
dependence is never written down, so it cannot be checked, and a second backend would have
to re-argue the whole protocol to be trusted.

This file writes it down. It states the store as an ordered byte keyspace with a handful of
laws, and then shows the two things the other specs actually need are consequences of the
laws and not of FoundationDB. A backend that meets the laws is substitutable; that is the
point, and it is why `substitutable` below is the theorem worth having.

The laws are deliberately few. Every `fdb_` call site in `fdb_vfs.c` is one of `get`,
`get_range`, `set`, `clear_range`, `commit`, `on_error` — no snapshot reads, no atomic
operations, no versionstamps, no watches, and no transaction options set anywhere. So the
surface below is not a simplification of what the VFS uses. It is what the VFS uses.

## Read-your-writes is not here, and should not be

No `*_body` in `fdb_vfs.c` reads a key it wrote in the same transaction. The only
read-after-write pairs are on disjoint keys — `resolve_body` writes PIDX and SIZE and then
reads LOGN — and no clear overlaps a preceding set: `finish_body` writes `SHARDN/<as_of>`
and clears `[SHARDN/0, SHARDN/<as_of>)`, an exclusive upper bound.

Leaving it out is not an oversight. The dirty-page buffer already is read-your-writes, at
the layer that owns uncommitted pages; providing it again in the store would be two sources
of truth for one page. And a body that could read its own writes is a body tempted to stay
open across a SQLite statement, which is fatal under FoundationDB's five second limit — a
limit handled nowhere in the tree, because every body is short instead.

## What this does not claim

It does not prove FoundationDB correct, and it does not prove SQLite correct, exactly as
`ParallelCommit.lean` proves neither. It reduces trusting a backend to arguing the laws
below, instead of re-arguing the protocol above them.

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.Backend

/-! ## The keyspace

A key is bytes, and the order is the order of the bytes. `fdb_keys.h` puts every number in
a key big endian precisely so that this order is the order of the numbers, and
`keys-witness/keys_test.cc` holds that property. So the ordering below is the one assumption this
file makes about keys, and it is checked elsewhere rather than here. -/

abbrev Key := List UInt8
abbrev Val := List UInt8

/-- Strict lexicographic order on bytes: the order `memcmp` gives, which is the order
FoundationDB gives its keyspace and the order SQLite gives a `BLOB` primary key. A shorter
key that is a prefix of a longer one comes first. -/
def lexLt : Key → Key → Bool
  | [], [] => false
  | [], _ :: _ => true
  | _ :: _, [] => false
  | a :: as, b :: bs => if a == b then lexLt as bs else a < b

def lexLe (x y : Key) : Bool := x == y || lexLt x y

/-- Half open, the way every range in `fdb_vfs.c` is: `[lo, hi)`. -/
def inRange (lo hi k : Key) : Bool := lexLe lo k && lexLt k hi

/-! The ordering is the one place this spec touches something a real store must agree with,
so it is worth pinning to the check that was actually run against SQLite. These are the same
six keys, in the same order `SELECT ... ORDER BY k` returned them. -/

example : lexLt [0] [0, 0] = true := by decide
example : lexLt [0, 0] [1] = true := by decide
example : lexLt [1] [0x7f, 0x80] = true := by decide
example : lexLt [0x7f, 0x80] [0xff] = true := by decide
example : lexLt [0xff] [0xff, 0xff] = true := by decide

/-- `key_after`, which FoundationDB calls `strinc`, exists so that a prefix scan is a range.
Nothing below needs its definition, only that a prefix range is a range. -/
example : inRange [1] [2] [1, 0, 0] = true := by decide

/-! ## The store

A function from keys to values, in the shape `ReadAhead.Store` already uses: a real store is
finite, but nothing here counts keys, and a function keeps the proofs to `simp`. -/

structure Store where
  find : Key → Option Val

/-- What a range read answers.

`covered` is which keys came back, not which keys exist. That distinction is the whole
content of `ReadAhead.Slot`: a key the read did not return is not a key that is not there.
`more` is FoundationDB's own flag, and it is the only honest way to say which of the two a
caller is looking at. -/
structure Ranged where
  covered : Key → Bool
  more : Bool

/-- The operations, and there are only four. `commit` and `on_error` are about transactions
rather than the keyspace, and they appear in `Commit` below. -/
structure Backend where
  get : Store → Key → Option Val
  getRange : Store → Key → Key → Nat → Bool → Ranged
  set : Store → Key → Val → Store
  clearRange : Store → Key → Key → Store

/-! ## The laws

One field for each obligation, named for the code that depends on it. -/

structure Laws (b : Backend) : Prop where
  /-- A read is a read. -/
  get_is_the_store : ∀ s k, b.get s k = s.find k
  /-- A write is visible at the key it wrote. -/
  set_writes : ∀ s k v, (b.set s k v).find k = some v
  /-- And nowhere else. -/
  set_is_local : ∀ s k v j, j ≠ k → (b.set s k v).find j = s.find j
  /-- A clear empties its range. -/
  clear_empties : ∀ s lo hi k, inRange lo hi k = true → (b.clearRange s lo hi).find k = none
  /-- And leaves everything outside it alone, or `finish_body` would take the shard it
  just wrote. -/
  clear_is_local : ∀ s lo hi k, inRange lo hi k = false → (b.clearRange s lo hi).find k = s.find k
  /-- Only keys in the range come back. -/
  range_is_sound : ∀ s lo hi n rev k,
    (b.getRange s lo hi n rev).covered k = true → inRange lo hi k = true
  /-- **`more = false` means the range was covered.** This is the law `ReadAhead.lean` is
  about and the one `query_intent_body` does not honour. -/
  range_is_complete : ∀ s lo hi n rev k,
    (b.getRange s lo hi n rev).more = false →
    inRange lo hi k = true → s.find k ≠ none →
    (b.getRange s lo hi n rev).covered k = true

/-! ## A commit is all of it or none of it

The property `prove_crash.c` looks for, and the one `PRAGMA integrity_check` cannot see: a
database holding pages from two commits is structurally perfect and still wrong.

Stating it as a relation with exactly two constructors is the statement. There is no third
outcome, so there is no torn store to reason about. -/

inductive Op where
  | put (k : Key) (v : Val)
  | clear (lo hi : Key)

def apply (b : Backend) (s : Store) : List Op → Store
  | [] => s
  | .put k v :: rest => apply b (b.set s k v) rest
  | .clear lo hi :: rest => apply b (b.clearRange s lo hi) rest

/-- What a process that may die at any moment can leave behind. -/
inductive Commit (b : Backend) (s : Store) (ops : List Op) : Store → Prop where
  /-- The commit landed. -/
  | landed : Commit b s ops (apply b s ops)
  /-- It did not, and losing the round in flight is correct: `Weft.Actor.Store` accepts
  that a crash loses the last few commits. -/
  | lost : Commit b s ops s

/-- **No torn commit exists.** Whatever a crash leaves, it is one of the two ends. -/
theorem no_torn_commit {b : Backend} {s s' : Store} {ops : List Op}
    (h : Commit b s ops s') : s' = apply b s ops ∨ s' = s := by
  cases h with
  | landed => exact Or.inl rfl
  | lost => exact Or.inr rfl

/-! ## The fence only ever rises

`TemporalTSCacheProperties`, and `ParallelCommit.fence_never_falls` is the same fact about
the protocol. Here it is the obligation on the store that makes it true: a write transaction
reads the fence, and it may only commit if the fence did not move meanwhile.

`fdb_vfs.c` gets this from a plain non-snapshot `get`, which registers a read conflict.
A local backend gets it from `BEGIN IMMEDIATE`, which takes the write lock before the read,
so no other writer exists to move it. Different mechanisms, one law. -/

/-- The keys a transaction read, and the rule for whether it may commit. -/
def conflictFree (s s' : Store) (reads : List Key) : Prop :=
  ∀ k ∈ reads, s'.find k = s.find k

/-- A transaction that read a key and committed saw that key unchanged. Immediate from the
definition, and it is the definition that matters: it is what `check_fence` is buying. -/
theorem fence_was_not_moved {s s' : Store} {reads : List Key} {f : Key}
    (h : conflictFree s s' reads) (hf : f ∈ reads) : s'.find f = s.find f := by
  unfold conflictFree at h
  exact h f hf

/-! ## Two backends

Both are stated as what the system provides, not as an implementation. The difference
between them is exactly one thing: whether a range read can come back short. -/

/-- A backend whose range reads always cover the range they were asked for.

This is the local SQLite store. A range read is one `SELECT` over a `BLOB` primary key with
no transaction size cap above it, so there is nothing to truncate against. -/
def complete : Backend where
  get := fun s k => s.find k
  getRange := fun _ lo hi _ _ => { covered := inRange lo hi, more := false }
  set := fun s k v => ⟨fun j => if j = k then some v else s.find j⟩
  clearRange := fun s lo hi => ⟨fun j => if inRange lo hi j then none else s.find j⟩

/-- A backend whose range reads may come back short.

This is FoundationDB. A transaction is capped at 10 MB, so a range over a megabyte of pages
answers with what fitted and sets `more`. `fits` is which keys came back, and `cutShort` is
whether this particular range was cut — the flag the C actually reads.

An earlier version of this model set `more` unconditionally. That made `range_is_complete`
vacuous, and it said something false: FoundationDB *does* promise it covered the range when
it does not set `more`, and the C is entitled to rely on that. Modelling the flag as always
true would have let a backend that never covers anything satisfy the laws, which is not a
store the layout could run on. `honest` below ties the two together, so the law is carried
rather than dodged. -/
def truncating (fits : Key → Bool) (cutShort : Key → Key → Bool) : Backend where
  get := complete.get
  getRange := fun _ lo hi _ _ =>
    { covered := fun k => inRange lo hi k && fits k, more := cutShort lo hi }
  set := complete.set
  clearRange := complete.clearRange

/-- What a range read has to mean by not setting `more`: everything in the range came back.
That is the whole content of the flag, and it is the obligation `query_intent_body` drops
when it stops at 4096 rows without looking. -/
def honest (fits : Key → Bool) (cutShort : Key → Key → Bool) : Prop :=
  ∀ lo hi k, cutShort lo hi = false → inRange lo hi k = true → fits k = true

theorem complete_is_lawful : Laws complete := by
  constructor
  · intro _ _; rfl
  · intro _ k _; simp [complete]
  · intro _ k _ j hj; simp [complete, hj]
  · intro _ _ _ k hk; simp [complete, hk]
  · intro _ _ _ k hk; simp [complete, hk]
  · intro _ _ _ _ _ _ h; exact h
  · intro _ _ _ _ _ _ _ h _; exact h

theorem truncating_is_lawful (fits : Key → Bool) (cutShort : Key → Key → Bool)
    (h : honest fits cutShort) : Laws (truncating fits cutShort) := by
  constructor
  · intro _ _; rfl
  · intro _ k _; simp [truncating, complete]
  · intro _ k _ j hj; simp [truncating, complete, hj]
  · intro _ _ _ k hk; simp [truncating, complete, hk]
  · intro _ _ _ k hk; simp [truncating, complete, hk]
  · intro _ _ _ _ _ _ hk; simp [truncating] at hk; exact hk.1
  -- Not vacuous any more: `honest` is what carries it.
  · intro _ lo hi _ _ k hm hin _
    simp only [truncating] at hm ⊢
    simp [hin, h lo hi k hm hin]

/-- **The flag is not decoration.** When the read did cut short, a key that is in the range
and in the store can still be missing from what came back — which is exactly the state
`ReadAhead.ignoring_more_loses_a_page` turns into a wrong page.

`fitsBelowFive` is a read that got through the first few keys and stopped. -/
def fitsBelowFive : Key → Bool := fun k => lexLt k [5]

theorem cut_short_really_loses_a_key :
    ((truncating fitsBelowFive (fun _ _ => true)).getRange
      ⟨fun _ => some []⟩ [0] [9] 0 false).covered [7] = false := by
  decide

/-- And the same read, uncut, covers it. So the two backends differ on this one fact and
agree on everything else, which is what makes the hypothesis in `resolve_is_honest` the
whole content of the difference. -/
theorem uncut_covers_it :
    ((truncating (fun _ => true) (fun _ _ => false)).getRange
      ⟨fun _ => some []⟩ [0] [9] 0 false).covered [7] = true := by
  decide

/-! ## Transport: the store's laws are what the protocol was assuming

`ParallelCommit.lean` records a live defect. `query_intent_body` enumerates a participant's
staged pages with a range read, stops at 4096, and ignores `more`; `resolve_body` writes a
PIDX row for exactly the pages it was handed and then advances the head regardless. A page
left out has no index row, so a read falls through to the shard and finds the version from
before the commit, and `truncated_resolve_reads_the_old_page` is that.

The obligation it left is `indexesEverything`. Below, that obligation is discharged by
`range_is_complete` — so it was never a fact about FoundationDB, it was a fact about
whether the enumeration finished. -/

/-- Where a participant's staged pages live: one key for each page, all inside one range,
because DELTA is keyed by txid and then page number. -/
structure Layout where
  deltaKey : Nat → Key
  lo : Key
  hi : Key
  in_range : ∀ p, inRange lo hi (deltaKey p) = true

/-- The covered set of `ReadAhead.fillUpTo` and of `resolvePaged`, read off a range read. -/
def coveredPages (L : Layout) (r : Ranged) : Nat → Bool := fun p => r.covered (L.deltaKey p)

/-- **A finished enumeration covers every page that is staged.** -/
theorem staged_pages_are_covered (b : Backend) (hb : Laws b) (L : Layout)
    (s : Store) (n : Nat) (rev : Bool)
    (hm : (b.getRange s L.lo L.hi n rev).more = false)
    (p : Nat) (hp : s.find (L.deltaKey p) ≠ none) :
    coveredPages L (b.getRange s L.lo L.hi n rev) p = true :=
  hb.range_is_complete s L.lo L.hi n rev (L.deltaKey p) hm (L.in_range p) hp

/-- The same fact in the words `ParallelCommit.lean` uses, so it discharges the obligation
that spec states rather than merely resembling it.

`holds` is the bridge: a page is staged in the model exactly when its key is in the store. -/
theorem indexes_everything (b : Backend) (hb : Laws b) (L : Layout)
    (s : Store) (n : Nat) (rev : Bool)
    (hm : (b.getRange s L.lo L.hi n rev).more = false)
    (d : Weft.ParallelCommit.Paged)
    (holds : ∀ p, (d.staged p).isSome → s.find (L.deltaKey p) ≠ none) :
    Weft.ParallelCommit.indexesEverything d
      (coveredPages L (b.getRange s L.lo L.hi n rev)) := by
  intro p hp
  exact staged_pages_are_covered b hb L s n rev hm p (holds p hp)

/-- **And then the read is honest.** A resolve that moved the head over an enumeration that
finished returns the committed page for every page the commit staged.

This is `resolve_is_honest_when_it_covers_everything`, with its hypothesis now supplied by
the backend's laws instead of assumed. -/
theorem resolve_is_honest (b : Backend) (hb : Laws b) (L : Layout)
    (s : Store) (n : Nat) (rev : Bool)
    (hm : (b.getRange s L.lo L.hi n rev).more = false)
    (d : Weft.ParallelCommit.Paged) (tx : Nat)
    (holds : ∀ p, (d.staged p).isSome → s.find (L.deltaKey p) ≠ none)
    (p : Nat) (hp : (d.staged p).isSome) :
    (Weft.ParallelCommit.resolvePaged d tx
      (coveredPages L (b.getRange s L.lo L.hi n rev))).read p = d.staged p :=
  Weft.ParallelCommit.resolve_is_honest_when_it_covers_everything d tx _
    (indexes_everything b hb L s n rev hm d holds) p hp

/-! ## What the local backend earns

On FoundationDB the hypothesis `more = false` is a real obligation: the 10 MB cap means a
range over a large commit can come back short, and the C has to notice. On a backend whose
range reads always finish, the hypothesis is discharged once and for all, and the defect
`truncated_resolve_reads_the_old_page` describes cannot arise.

That is not a claim about tidier code. It is a class of bug that stops existing. -/

theorem complete_never_truncates (s : Store) (lo hi : Key) (n : Nat) (rev : Bool) :
    (complete.getRange s lo hi n rev).more = false := rfl

theorem resolve_is_honest_on_complete (L : Layout) (s : Store) (n : Nat) (rev : Bool)
    (d : Weft.ParallelCommit.Paged) (tx : Nat)
    (holds : ∀ p, (d.staged p).isSome → s.find (L.deltaKey p) ≠ none)
    (p : Nat) (hp : (d.staged p).isSome) :
    (Weft.ParallelCommit.resolvePaged d tx
      (coveredPages L (complete.getRange s L.lo L.hi n rev))).read p = d.staged p :=
  resolve_is_honest complete complete_is_lawful L s n rev
    (complete_never_truncates s L.lo L.hi n rev) d tx holds p hp

/-! ## Transport: the read side

`ReadAhead.readVia_fillUpTo` proves a window is safe for *any* covered set, so read-ahead
needs nothing from the store beyond soundness — a prefetch may waste a round trip and may
not change an answer. What it does need is that the covered set is honest about what it
covers, and that is `range_is_sound`: a key the read did not return is `unfetched`, never
`absent`.

So the obligation the store carries on the read side is only this: do not report a key as
covered when it was not in the range asked for. -/

theorem covered_stays_in_range (b : Backend) (hb : Laws b) (s : Store)
    (lo hi : Key) (n : Nat) (rev : Bool) (k : Key)
    (h : (b.getRange s lo hi n rev).covered k = true) : inRange lo hi k = true :=
  hb.range_is_sound s lo hi n rev k h

/-- Read-ahead is safe over any backend, because it is safe over any covered set. The window
`fillUpTo` builds from a range read cannot change an answer, whichever backend filled it. -/
theorem read_ahead_is_safe_on_any_backend
    (rs : Weft.ReadAhead.Store) (first count : Nat) (covered : Nat → Bool) (p : Nat) :
    Weft.ReadAhead.readVia rs (Weft.ReadAhead.fillUpTo rs first count covered) p = rs.read p :=
  Weft.ReadAhead.readVia_fillUpTo rs first count covered p

/-! ## The theorem this file exists for -/

/-- **Substitutable.** A backend meeting the laws carries the store: an enumeration
that finished indexes every staged page, a resolve over it is honest, a covered key really
was in the range, a commit is one of its two ends, and read-ahead cannot change an answer.

Nothing in the list mentions FoundationDB. That is the content: what the layout above needs
is the laws, and a store that meets them is the same store as far as anything here can tell.
Choosing between them is then a deployment question — cross-machine handoff against a commit
that costs an fsync rather than a round trip — and not a correctness one. -/
theorem substitutable (b : Backend) (hb : Laws b) :
    (∀ (L : Layout) (s : Store) (n : Nat) (rev : Bool),
        (b.getRange s L.lo L.hi n rev).more = false →
        ∀ (d : Weft.ParallelCommit.Paged) (tx p : Nat),
          (∀ q, (d.staged q).isSome → s.find (L.deltaKey q) ≠ none) →
          (d.staged p).isSome →
          (Weft.ParallelCommit.resolvePaged d tx
            (coveredPages L (b.getRange s L.lo L.hi n rev))).read p = d.staged p)
    ∧ (∀ s lo hi n rev k,
        (b.getRange s lo hi n rev).covered k = true → inRange lo hi k = true)
    ∧ (∀ (s s' : Store) (ops : List Op),
        Commit b s ops s' → s' = apply b s ops ∨ s' = s) := by
  refine ⟨?_, ?_, ?_⟩
  · intro L s n rev hm d tx p holds hp
    exact resolve_is_honest b hb L s n rev hm d tx holds p hp
  · intro s lo hi n rev k h
    exact covered_stays_in_range b hb s lo hi n rev k h
  · intro _ _ _ h
    exact no_torn_commit h

end Weft.Backend
