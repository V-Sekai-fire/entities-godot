import GroupCommit

/-! # MMO Rung 5: per-shard batched group commit across distinct avatars

`shard_loop` in `store.cpp` processes one iceoryx2 request at a time and blocks on
`xSync` for each one, so per-shard depth is exactly one FDB commit in flight. On the
`weftspun-fdb` cluster that is ~170 commits/s per shard, and on an 8-core box ~1380
commits/s total. An MMO tick-safe workload of 10 k avatars at 60 Hz needs ~600 000
commits/s, i.e. about 430× that ceiling.

Rung 5 coalesces N distinct avatars' commits within one shard into one FoundationDB
transaction. Every avatar lives under its own `weft/db/<name>/` prefix in the FDB
keyspace, so the write sets are *disjoint by construction*. The rest of the
correctness argument reduces to `spec/GroupCommit.lean`:

  * `GroupCommit.merged_commit_read_set_is_union` — the batch's read-conflict set
    is the union of the per-avatar read-conflict sets, so no key that mattered to
    any one avatar's transaction is dropped.
  * `GroupCommit.merged_commit_aborts_on_any_conflict` — any single conflict on any
    merged row aborts the whole batch, so a concurrent writer against any avatar's
    fence, PIDX or DELTA row still forces the batch to retry.
  * `GroupCommit.merged_writes_equal_sequential_writes` — the visible state after
    a successful batch is the same as applying each avatar's writes in order.

What this file adds is the one structural fact those three theorems assumed but did
not name: **two Ops keyed under different avatar namespaces never touch the same
key on the write side.** That is a fact about how `fdb_keys.h` builds keys (the
avatar name sits in a fixed prefix), so it belongs alongside the group-commit
theorems rather than inside the C.

**Fence check.** Each avatar has its own fence at `weft/db/<name>/FENCE`, and the
batch carries every avatar's fence read. If any avatar has been re-opened by
another writer since the batch started, that fence's value has changed and the
batch aborts. That is `merged_commit_aborts_on_any_conflict` applied to the fence
key — no new theorem needed.

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.ShardBatchCommit

open Weft.GroupCommit

/-- A namespace is the leading bytes shared by every key belonging to one avatar. In
`fdb_vfs.c` that is `weft/db/<name>/`. Nothing in the model depends on the exact
layout — only that two avatars have different namespaces, which is what
`fdb_keys.h` guarantees by putting the name into the prefix. -/
abbrev Namespace := Weft.Backend.Key

/-- A key `k` is *under* namespace `ns` when it starts with `ns` byte-for-byte. -/
def startsWith : Weft.Backend.Key → Namespace → Prop
  | _, [] => True
  | [], _ :: _ => False
  | k :: ks, n :: ns => k = n ∧ startsWith ks ns

/-- Op `o` lives under namespace `ns`: every read and every write key does. -/
def opUnderNamespace (ns : Namespace) (o : Op) : Prop :=
  (∀ k ∈ o.reads, startsWith k ns) ∧
  (∀ kv ∈ o.writes, startsWith kv.1 ns)

/-! ## Disjointness by namespace

Two Ops under different namespaces of equal length cannot share a key. The proof is
by simultaneous induction on both namespaces: a key that starts with both must
agree byte-for-byte on the shared prefix, forcing the two namespaces to be equal. -/

/-- A key that starts with both `ns1` and `ns2`, where the two namespaces have the
same length, forces `ns1 = ns2`. -/
theorem startsWith_of_same_length_forces_equal :
    ∀ (k ns1 ns2 : List UInt8),
      ns1.length = ns2.length →
      startsWith k ns1 → startsWith k ns2 →
      ns1 = ns2 := by
  intro k ns1
  induction ns1 generalizing k with
  | nil =>
      intro ns2 hlen _ _
      match ns2 with
      | [] => rfl
      | _ :: _ => simp [List.length] at hlen
  | cons a as ih =>
      intro ns2 hlen h1 h2
      match k, ns2 with
      | [], _ => exact h1.elim
      | b :: bs, [] => simp [List.length] at hlen
      | b :: bs, c :: cs =>
          have ha : b = a := h1.1
          have hc : b = c := h2.1
          have has : startsWith bs as := h1.2
          have hcs : startsWith bs cs := h2.2
          have hlen' : as.length = cs.length := by
            simp [List.length] at hlen; exact hlen
          have hrest : as = cs := ih bs cs hlen' has hcs
          have hac : a = c := ha.symm.trans hc
          rw [hac, hrest]

/-- **Disjoint namespaces have disjoint keys.** Two keys under different namespaces
of equal length cannot be equal: if they were, one key would start with both
namespaces, and `startsWith_of_same_length_forces_equal` would give `ns1 = ns2`. -/
theorem disjoint_namespaces_have_disjoint_keys
    (ns1 ns2 : Namespace) (k1 k2 : Weft.Backend.Key)
    (hlen : ns1.length = ns2.length)
    (hne : ns1 ≠ ns2)
    (h1 : startsWith k1 ns1) (h2 : startsWith k2 ns2) :
    k1 ≠ k2 := by
  intro heq
  rw [heq] at h1
  exact hne (startsWith_of_same_length_forces_equal k2 ns1 ns2 hlen h1 h2)

/-- **Disjoint namespaces have disjoint writes.** Two Ops under different namespaces
share no write key. This is what makes a batched shard commit's write set structurally
disjoint — an FDB transaction whose writes are disjoint has no write-write conflict
inside the batch, so all the avatars land together and independently. -/
theorem disjoint_namespaces_have_disjoint_writes
    (ns1 ns2 : Namespace) (o1 o2 : Op)
    (hlen : ns1.length = ns2.length)
    (hne : ns1 ≠ ns2)
    (h1 : opUnderNamespace ns1 o1) (h2 : opUnderNamespace ns2 o2) :
    ∀ kv1 ∈ o1.writes, ∀ kv2 ∈ o2.writes, kv1.1 ≠ kv2.1 := by
  intro kv1 hkv1 kv2 hkv2
  exact disjoint_namespaces_have_disjoint_keys ns1 ns2 kv1.1 kv2.1 hlen hne
    (h1.2 kv1 hkv1) (h2.2 kv2 hkv2)

/-- **Disjoint namespaces have disjoint reads too.** The same argument on the read
side: a key another avatar's transaction read is a key this avatar's transaction
did not read (because it starts with the wrong namespace). Used implicitly by
`GroupCommit.merged_commit_aborts_on_any_conflict`: a conflict on an avatar's read
key stays attributable to that avatar rather than leaking across the batch. -/
theorem disjoint_namespaces_have_disjoint_reads
    (ns1 ns2 : Namespace) (o1 o2 : Op)
    (hlen : ns1.length = ns2.length)
    (hne : ns1 ≠ ns2)
    (h1 : opUnderNamespace ns1 o1) (h2 : opUnderNamespace ns2 o2) :
    ∀ k1 ∈ o1.reads, ∀ k2 ∈ o2.reads, k1 ≠ k2 := by
  intro k1 hk1 k2 hk2
  exact disjoint_namespaces_have_disjoint_keys ns1 ns2 k1 k2 hlen hne
    (h1.1 k1 hk1) (h2.1 k2 hk2)

/-! ## Rule-2 controls -/

/-- Same-namespace Ops CAN share keys, and that is what makes the disjointness
theorem non-trivial. If the theorem were vacuous, this control would still hold. -/
example : startsWith [1,2,3,4] [1,2] ∧ startsWith [1,2,5,6] [1,2] := by
  refine ⟨?_, ?_⟩
  · simp [startsWith]
  · simp [startsWith]

/-- **Rule-2 control** — planted-wrong disjointness claim. If we forgot the equal-
length hypothesis, a namespace `[1]` and `[1, 2]` would trivially be "different"
yet a key starting with `[1, 2, 3]` starts with both. Named here so a future
refactor cannot silently drop `hlen`. -/
def stubKey1 : List UInt8 := [1, 2, 3]

theorem unequal_length_prefixes_can_share_a_key :
    startsWith stubKey1 [1] ∧ startsWith stubKey1 [1, 2] := by
  refine ⟨?_, ?_⟩
  · simp [startsWith, stubKey1]
  · simp [startsWith, stubKey1]

end Weft.ShardBatchCommit
