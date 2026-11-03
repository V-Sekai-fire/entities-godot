import ReadAhead

/-! # The pipelined two-row read

`page_from_store` in `fdb_vfs.c` does the read `Store.read` says: fetch PIDX to learn the
owner, then fetch DELTA if the log holds the page or SHARD otherwise. That is two round
trips in sequence.

Rung 1 of the roundtable ladder pipelines the common case. The txid of the newest commit
— HEAD — is carried on `FdbFile`, and almost every warm read owns pages under HEAD. So the
VFS issues two FoundationDB futures at once: PIDX for `p`, and DELTA under HEAD for `p`.
If PIDX comes back as `some head`, the speculative DELTA future already holds the answer.
If PIDX names a different txid, the speculative DELTA is discarded and a second DELTA
future is issued under the correct owner; if PIDX is `none`, a SHARD future is issued
instead. The wrong-txid path pays what the sequential path paid; the hot path is one
round trip.

Two properties have to hold, and neither may be assumed:

  1. **The pipelined read agrees with the sequential read.** Whatever HEAD is and whatever
     PIDX turns out to say, the page returned is the page `Store.read` returns.

  2. **The pipelined read-conflict set contains the sequential one.** FDB is
     strict-serialisable in its default mode, and the conflict set is what carries that
     across a transaction. Adding a speculative read to the transaction extends the
     conflict set. Adding one is always safe; dropping one is not, and this file names it
     as the invariant so that a future change that drops a key from the conflict set
     fails Lean rather than YCSB.

`ReadAhead.readVia_fillUpTo` covers a *different* pipelining — one range read of PIDX and
one of SHARD — and it is the analogue this file follows. The prefetch there is a range
read; the pipeline here is a point-read pair.

An earlier draft of this rung proposed a snapshot read in `read_body` to drop the
conflict range. That is wrong under workload F: a snapshot read drops the row from the
transaction's read-conflict set, so a concurrent commit against the same row lands
unnoticed and the modify writes on top of a stale value. Rung 1 keeps every key that the
sequential read consulted in the conflict set, and it adds one more (DELTA under HEAD).

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.ReadAhead

/-- The answer the pipelined read returns.

`head` is the HEAD txid the VFS carries on `FdbFile`; the pipelined read speculates that
the page's owner is HEAD. Whichever branch it lands on, the answer is what `Store.read`
would have given.

Note the `some head` branch matches `Store.read`'s `some tx` branch when `tx = head`; the
model does not distinguish "already had the DELTA future in hand" from "issued a second
DELTA future" because the model is a spec of what value is returned, not of how many
round trips it took to return it. The round-trip win is the operational content of the
rung and the operational content sits in the C. -/
def Store.readPipelined (s : Store) (head : Nat) (p : Nat) : Option Page :=
  match s.owner p with
  | some tx =>
      if tx = head then s.delta head p    -- speculative future was right
      else s.delta tx p                    -- speculative future was wrong; re-issue
  | none =>
      if s.hasShard then s.shard p else none

/-- **Rung 1's correctness theorem.** The pipelined read agrees with the sequential read.
For every store, every HEAD, and every page. -/
theorem readPipelined_agrees_with_read
    (s : Store) (head p : Nat) :
    s.readPipelined head p = s.read p := by
  unfold Store.readPipelined Store.read
  cases ho : s.owner p with
  | some tx =>
      by_cases hh : tx = head
      · subst hh; simp
      · simp [hh]
  | none => simp

/-! ## The conflict set

The keys a read touches. In the model each key is one lookup, so the set is captured as a
predicate; the sequential path consults PIDX(p) always, then DELTA(owner, p) when there is
one and SHARD(p) otherwise, and the pipelined path additionally consults DELTA(head, p).
`ConflictKey` names each site so that the set-containment claim is decidable. -/
inductive ConflictKey where
  | pidx (p : Nat)
  | delta (tx p : Nat)
  | shard (p : Nat)
  deriving Repr, DecidableEq

/-- The keys the sequential `Store.read` sends to the backend when reading page `p`.

`PIDX(p)` is always consulted. Whichever branch PIDX names decides the second key. -/
def sequentialReadSet (s : Store) (p : Nat) : List ConflictKey :=
  ConflictKey.pidx p ::
    (match s.owner p with
     | some tx => [ConflictKey.delta tx p]
     | none => if s.hasShard then [ConflictKey.shard p] else [])

/-- The keys the pipelined read sends. The speculative DELTA(head, p) is unconditional;
the branch on the PIDX answer adds the sequential path's second key on top of it. -/
def pipelinedReadSet (s : Store) (head p : Nat) : List ConflictKey :=
  ConflictKey.pidx p :: ConflictKey.delta head p ::
    (match s.owner p with
     | some tx => if tx = head then [] else [ConflictKey.delta tx p]
     | none => if s.hasShard then [ConflictKey.shard p] else [])

/-- **Rung 1's serialisability theorem.** Every key the sequential read consulted is a
key the pipelined read also consults. So the FDB read-conflict set of the pipelined
transaction contains the sequential one, and no serialisable order the sequential path
already ruled out becomes available to the pipelined path. -/
theorem sequential_read_set_subset_pipelined
    (s : Store) (head p : Nat) :
    ∀ k ∈ sequentialReadSet s p, k ∈ pipelinedReadSet s head p := by
  intro k hk
  unfold sequentialReadSet pipelinedReadSet at *
  cases ho : s.owner p with
  | some tx =>
      simp [ho] at hk
      by_cases hh : tx = head
      · subst hh
        rcases hk with h | h
        · simp [h]
        · simp [h]
      · simp [hh]
        rcases hk with h | h
        · exact Or.inl h
        · exact Or.inr (Or.inr h)
  | none =>
      simp [ho] at hk
      by_cases hs : s.hasShard
      · simp [hs] at hk ⊢
        rcases hk with h | h
        · exact Or.inl h
        · exact Or.inr (Or.inr h)
      · simp [hs] at hk ⊢
        exact Or.inl hk

/-! ## Rule-2 controls

Each theorem above is paired with a planted-wrong step so that a change breaking the
theorem's premise breaks a `decide` here rather than passing silently. -/

/-- A store where the log holds page 0 under txid 7, and HEAD is 3. The pipelined
speculative future for DELTA(3, 0) misses; a second DELTA(7, 0) has to be issued for the
answer to agree with the sequential path. -/
def wrongHead : Store :=
  { owner := fun p => if p = 0 then some 7 else none
    delta := fun tx p => if tx = 7 ∧ p = 0 then some 42 else none
    shard := fun _ => none
    hasShard := false }

example : wrongHead.readPipelined 3 0 = some 42 := by decide
example : wrongHead.readPipelined 3 0 = wrongHead.read 0 := by decide

/-- The wrong implementation: return the speculative DELTA answer even when the txid did
not match. `decide` refutes agreement, so this variant is the negative control. -/
def readPipelinedBroken (s : Store) (head p : Nat) : Option Page :=
  match s.owner p with
  | some _ => s.delta head p    -- wrong: uses the speculative txid always
  | none => if s.hasShard then s.shard p else none

/-- **Rule 2 control.** The broken pipelined read loses the log-held page — the same shape
of failure as `collapsed_loses_a_page` in `ReadAhead.lean`. -/
theorem broken_pipeline_loses_a_page :
    readPipelinedBroken wrongHead 3 0 ≠ wrongHead.read 0 := by
  decide

/-- The other wrong implementation: drop the PIDX key from the conflict set. The
containment theorem above then fails, because `sequentialReadSet` names `pidx p` and this
set does not. -/
def brokenPipelinedReadSet (head p : Nat) : List ConflictKey :=
  [ConflictKey.delta head p]

theorem broken_read_set_drops_pidx :
    ConflictKey.pidx 0 ∈ sequentialReadSet wrongHead 0 ∧
    ConflictKey.pidx 0 ∉ brokenPipelinedReadSet 3 0 := by
  refine ⟨?_, ?_⟩
  · simp [sequentialReadSet]
  · simp [brokenPipelinedReadSet]

end Weft.ReadAhead
