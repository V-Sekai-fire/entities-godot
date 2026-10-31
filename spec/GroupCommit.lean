import Backend

/-! # Group-committing a run of xSync operations

Under YCSB workload F the JDBC binding runs with `jdbc.autocommit=true`, so each SQL
statement ends with one `xSync`. `fdb_vfs.c`'s `flush` currently answers each `xSync` with
its own FoundationDB transaction — one commit per YCSB op. On the `weftspun-fdb` cluster,
measured `commit_seconds` is 5.8 ms; each op pays that in full, which is what puts the
observed RMW p99 at 114 ms.

Rung 2 of the roundtable ladder coalesces `xSync` calls that arrive inside a small window
into one FoundationDB commit. The dirty-page set of every merged op lands together, the
read-conflict set is the union of the merged ops' read-conflict sets, and one commit pays
for all of them. This file proves the two things that must hold before the C is edited:

  1. `merged_commit_read_set_is_union`: the merged commit's read-conflict set is the
     union of the individual ops' read-conflict sets. No key is dropped; adding keys is
     always safe.
  2. `merged_commit_aborts_on_any_conflict`: a conflict against any merged row aborts the
     whole commit. So merging cannot let a stale value through.

Together these two say that merging preserves FDB's strict-serialisable semantics: the
merged commit is equivalent to the individual commits w.r.t. FDB's read-conflict rule.
An earlier draft's snapshot-read idea would have violated #1 — it would have *dropped*
keys from the conflict set — which is why it is not on the ladder.

`Backend.conflictFree` is the mechanism used here: an FDB transaction may commit only if
the store the reads observed is still the store at commit time on every read key.

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.GroupCommit

open Weft.Backend

/-- One xSync operation: what keys it read, and what it writes.

`reads` is the FDB read-conflict set the op accumulates before `xSync` fires. `writes` is
the list of key/value updates the op applies. This is the shape `flush_ctx` in
`fdb_vfs.c` carries in memory before the commit, one entry per dirty page. -/
structure Op where
  reads : List Key
  writes : List (Key × Val)

/-- Apply one op's writes to a store, in order. -/
def applyOp (b : Backend) (s : Store) (o : Op) : Store :=
  o.writes.foldl (fun s' kv => b.set s' kv.1 kv.2) s

/-- A group commit: several ops issued as one FDB transaction. -/
abbrev Group := List Op

/-- The merged read-conflict set: every read of every op. -/
def readsOf : Group → List Key
  | [] => []
  | o :: rest => o.reads ++ readsOf rest

/-- The merged write list: every write of every op, in the order the ops arrived. -/
def writesOf : Group → List (Key × Val)
  | [] => []
  | o :: rest => o.writes ++ writesOf rest

/-- Apply a whole group's writes to a store. Equivalent to `applyOp` on each op in
sequence, and stated below as `merged_writes_equal_sequential_writes`. -/
def applyGroup (b : Backend) (s : Store) (g : Group) : Store :=
  (writesOf g).foldl (fun s' kv => b.set s' kv.1 kv.2) s

/-- The state after committing a group op-by-op, each op in its own transaction. This is
the current (pre-Rung-2) behaviour. -/
def applyPerOp (b : Backend) (s : Store) (g : Group) : Store :=
  g.foldl (applyOp b) s

/-! ## Rung 2's two theorems -/

/-- **The merged commit's read-conflict set is the union of the ops' read-conflict sets.**

Directly from `readsOf`. This is the invariant a wrong Rung 2 implementation would break
by dropping keys to save cost. No key is dropped: every read every merged op did is a
read the merged transaction did. -/
theorem merged_commit_read_set_is_union (g : Group) :
    ∀ k, (∃ o, o ∈ g ∧ k ∈ o.reads) ↔ k ∈ readsOf g := by
  intro k
  induction g with
  | nil =>
      constructor
      · rintro ⟨_, hmem, _⟩; exact (List.not_mem_nil hmem).elim
      · intro hk; exact (List.not_mem_nil hk).elim
  | cons hd rest ih =>
      constructor
      · rintro ⟨o', ho'g, hk⟩
        rcases List.mem_cons.mp ho'g with h | h
        · rw [h] at hk
          show k ∈ hd.reads ++ readsOf rest
          exact List.mem_append.mpr (Or.inl hk)
        · have hrest : k ∈ readsOf rest := ih.mp ⟨o', h, hk⟩
          show k ∈ hd.reads ++ readsOf rest
          exact List.mem_append.mpr (Or.inr hrest)
      · intro hk
        have hk' : k ∈ hd.reads ++ readsOf rest := hk
        rcases List.mem_append.mp hk' with h | h
        · exact ⟨hd, List.mem_cons.mpr (Or.inl rfl), h⟩
        · rcases ih.mpr h with ⟨o', ho', hk''⟩
          exact ⟨o', List.mem_cons.mpr (Or.inr ho'), hk''⟩

/-- **A conflict against any merged row aborts the whole commit.**

`conflictFree s s' reads` says the store agrees with `s` on every key the transaction
read. If it fails on any read key of any merged op, it fails for the merged commit.
FDB checks `conflictFree` over the merged transaction's read set, so a concurrent commit
against any of those keys aborts one of the two. -/
theorem merged_commit_aborts_on_any_conflict
    (s s' : Store) (g : Group) (o : Op) (k : Key)
    (ho : o ∈ g) (hk : k ∈ o.reads) (hchange : s'.find k ≠ s.find k) :
    ¬ conflictFree s s' (readsOf g) := by
  intro h
  have hmem : k ∈ readsOf g :=
    (merged_commit_read_set_is_union g k).mp ⟨o, ho, hk⟩
  exact hchange (h k hmem)



/-! ## The write side: merging preserves the sequential state -/

/-- **The merged write set equals the per-op sequence of writes.**

So the visible state after a successful merged commit is exactly the state after
committing every op in order. Merging cannot silently swap the order of writes, which is
what the per-file coalesce timer in `flush` relies on: dirty pages are appended in
xSync-arrival order, and the merged FDB commit applies them in that order. -/
theorem merged_writes_equal_sequential_writes (b : Backend) (_hb : Laws b) (s : Store)
    (g : Group) : applyGroup b s g = applyPerOp b s g := by
  induction g generalizing s with
  | nil => rfl
  | cons hd rest ih =>
      -- Both sides fold-left over the same total sequence of writes; on the merged side
      -- the fold is over `hd.writes ++ writesOf rest`, on the per-op side it is
      -- applyOp b s hd = fold over hd.writes, then applyPerOp b _ rest.
      show ((hd.writes ++ writesOf rest).foldl (fun s' kv => b.set s' kv.1 kv.2) s)
        = applyPerOp b (applyOp b s hd) rest
      rw [List.foldl_append]
      exact ih (applyOp b s hd)

/-! ## Rule-2 controls: planted-wrong variants that must fail -/

/-- The wrong implementation that drops a key from the merged read set.
`badMergedReadSet` keeps only the *last* op's reads. -/
def badMergedReadSet (g : Group) : List Key :=
  match g.reverse.head? with
  | some o => o.reads
  | none => []

/-- Two ops, where the first op read a key the second did not. Dropping the first op's
reads is what a naive Rung 2 would do to save conflict-range bytes. -/
def opFirst : Op := { reads := [[7]], writes := [] }
def opSecond : Op := { reads := [[9]], writes := [] }
def twoOps : Group := [opFirst, opSecond]

/-- **Rule-2 control.** The bad merged read set drops `[7]`. -/
theorem bad_merged_read_set_drops_a_key :
    [7] ∈ readsOf twoOps ∧ [7] ∉ badMergedReadSet twoOps := by
  refine ⟨?_, ?_⟩
  · decide
  · decide

/-- **Rule-2 control.** Under the bad set, a concurrent commit against `[7]` would fail
to abort the merged transaction, because `[7]` is not in the checked set. -/
theorem bad_set_hides_a_conflict (s s' : Store)
    (_hne : s'.find [7] ≠ s.find [7])
    (hother : ∀ k, k ≠ [7] → s'.find k = s.find k) :
    conflictFree s s' (badMergedReadSet twoOps) := by
  intro k hk
  have : k = [9] := by
    unfold badMergedReadSet twoOps opFirst opSecond at hk
    simp at hk
    exact hk
  subst this
  exact hother [9] (by decide)

end Weft.GroupCommit
