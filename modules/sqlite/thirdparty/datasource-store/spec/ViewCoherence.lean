/-! # View coherence: every transaction on a file checks the view its handle holds

A handle caches the store. SQLite keeps a page cache, and under `locking_mode=EXCLUSIVE` it
never re-reads page 1 to learn that the file changed; the VFS keeps the dirty buffer, the
read-ahead window, the head and the shard version. All of it describes the store as this
handle last left it.

The fence is what makes that description true for a writer: another writer raises the fence
before it writes, so while the fence still equals the handle's, nobody else has written.
`check_fence` in `fdb_vfs.c` reads the fence inside every write transaction. It read it inside
no read transaction, so a handle whose fence had moved kept reading pages, and a page read
after the move sat beside a page cached before it. A B-tree assembled from two databases is
not a database. In Uro that was a SIGSEGV in `sqlite3BtreeInsert`, on an upload through a
handle that had already been refused a write.

The comment on `check_fence` says it: a fence that covers one write path and not the others
is not a fence. The same sentence holds across reads and writes, and this file is that
sentence made checkable. Two parts:

* **Coverage.** Every `*_body` in `fdb_vfs.c` is a row of `Body`, with its scope and whether it
  checks the view. `file_bodies_check_view` is decided over the whole list, so a file-scoped
  body that skips the check is a failed build. `before_fix` keeps the table as it stood at
  `72e3fdf` and shows the theorem false there, which is the negative control.
  `check_bodies.sh` holds `Body` against the C, so a body added to one and not the other
  fails the `spec` stage.
* **Meaning.** `coherent` says what a passed check buys. It is also what keeps a SQLite
  transaction inside FoundationDB's semantics when it outlives one FoundationDB transaction,
  as a long read does at the cached transaction's four-second mark and a large commit does by
  staging: every read version a SQLite transaction runs on is the same store state or the
  read is refused, so the SQLite transaction sees one state, as a FoundationDB one does. In the protocol as `fdb_vfs.c` runs it
  — a writer raises the fence at open, a commit or a fold is refused unless the fence is the
  writer's — a store whose fence equals a handle's has that handle's head and shard version.
  So a transaction whose check passed reads the store the cache was built on.

What this does not claim. SQLite answering a query from its own cache never calls the VFS,
and the VFS cannot reach it. That read is stale rather than mixed: it hands SQLite no page
from a store it does not own. A reader that wants to follow the head keeps NORMAL locking,
where SQLite reads page 1 at every transaction and the check runs; `prove_fence_read.c` is
both cases, measured.

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.ViewCoherence

/-- Every transaction body in `fdb_vfs.c`, by its C name. -/
inductive Body where
  | open_body
  | read_body
  | delta_body
  | head_body
  | commit_body
  | fold_read_body
  | fold_write_body
  | finish_body
  | txn_seq_body
  | record_body
  | set_status_body
  | drop_record_body
  | read_parts_body
  | query_intent_body
  | resolve_body
  | prevent_body
  | sweep_body
  | delete_body
  | access_body
  deriving DecidableEq, Repr

/-- The list `decide` walks. `all_complete` is what makes walking it the same as `∀`. -/
def Body.all : List Body :=
  [.open_body, .read_body, .delta_body, .head_body, .commit_body, .fold_read_body,
   .fold_write_body, .finish_body, .txn_seq_body, .record_body, .set_status_body,
   .drop_record_body, .read_parts_body, .query_intent_body, .resolve_body, .prevent_body,
   .sweep_body, .delete_body, .access_body]

theorem Body.all_complete (b : Body) : b ∈ Body.all := by
  cases b <;> simp [Body.all]

/-- The C name, for `check_bodies.sh`. -/
def Body.cName : Body → String
  | .open_body => "open_body"
  | .read_body => "read_body"
  | .delta_body => "delta_body"
  | .head_body => "head_body"
  | .commit_body => "commit_body"
  | .fold_read_body => "fold_read_body"
  | .fold_write_body => "fold_write_body"
  | .finish_body => "finish_body"
  | .txn_seq_body => "txn_seq_body"
  | .record_body => "record_body"
  | .set_status_body => "set_status_body"
  | .drop_record_body => "drop_record_body"
  | .read_parts_body => "read_parts_body"
  | .query_intent_body => "query_intent_body"
  | .resolve_body => "resolve_body"
  | .prevent_body => "prevent_body"
  | .sweep_body => "sweep_body"
  | .delete_body => "delete_body"
  | .access_body => "access_body"

/-- What keys a body touches, and so whether a handle's view applies to it. -/
inductive Scope where
  /-- Loads the view and, for a writer, raises the fence. There is no view to check yet. -/
  | opens
  /-- Reads or writes the page and meta keys of the file a handle holds. The view applies. -/
  | file
  /-- The `weft/txn/` keyspace: group records and recovery. No handle, no view. `prevent_body`
  raises the fence of another file on purpose, which is what makes the owner's next check
  fail. -/
  | group
  /-- A file's keyspace reached by name with no handle, so there is no view to compare.
  `delete_body` is `xDelete`, which SQLite calls for journals this VFS never has, and
  `access_body` reads `SIZE` to answer `xAccess`. -/
  | name
  deriving DecidableEq, Repr

def scope : Body → Scope
  | .open_body => .opens
  | .read_body => .file
  | .delta_body => .file
  | .head_body => .file
  | .commit_body => .file
  | .fold_read_body => .file
  | .fold_write_body => .file
  | .finish_body => .file
  | .txn_seq_body => .group
  | .record_body => .file
  | .set_status_body => .group
  | .drop_record_body => .group
  | .read_parts_body => .group
  | .query_intent_body => .group
  | .resolve_body => .group
  | .prevent_body => .group
  | .sweep_body => .group
  | .delete_body => .name
  | .access_body => .name

/-- Whether the body, or the runner that calls it, reads the view inside the transaction.

`read_body` runs under `run_read_txn`, which calls `check_view` on every fresh transaction;
every read in that transaction is at the read version the check saw. The write bodies and
`fold_read_body` call `check_fence` themselves. `record_body` checks the fence of every
participant. -/
def checksView : Body → Bool
  | .open_body => false
  | .read_body => true
  | .delta_body => true
  | .head_body => true
  | .commit_body => true
  | .fold_read_body => true
  | .fold_write_body => true
  | .finish_body => true
  | .txn_seq_body => false
  | .record_body => true
  | .set_status_body => false
  | .drop_record_body => false
  | .read_parts_body => false
  | .query_intent_body => false
  | .resolve_body => false
  | .prevent_body => false
  | .sweep_body => false
  | .delete_body => false
  | .access_body => false

/-- The same table at `72e3fdf`, before the fix: two file-scoped bodies read pages with no
check, and the partial-write pre-read in `buffer_page` ran `read_body` under `run_txn`,
which checked nothing either. -/
def checksViewBefore : Body → Bool
  | .read_body => false
  | .fold_read_body => false
  | b => checksView b

/-- Every body that touches a file's keys through a handle checks the handle's view. -/
theorem file_bodies_check_view :
    Body.all.all (fun b => scope b != .file || checksView b) = true := by decide

theorem file_body_checks (b : Body) (h : scope b = .file) : checksView b = true := by
  cases b <;> simp_all [scope, checksView]

/-- Negative control: the table as it stood before the fix fails the same theorem. -/
theorem before_fix :
    Body.all.all (fun b => scope b != .file || checksViewBefore b) = false := by decide

/-- The bodies the control names, so the number is not read off a build log. -/
theorem before_fix_misses :
    (Body.all.filter (fun b => scope b == .file && !checksViewBefore b)) =
      [.read_body, .fold_read_body] := by decide

/-! ## What a passed check means -/

/-- The store as a view sees it. `owner` is the handle whose open raised the fence last. -/
structure Store where
  fence : Nat
  head : Nat
  shard : Nat
  owner : Nat

/-- One writer handle: the id it opened under and the view it cached. `pending` is a group
commit it has staged and not yet settled: the pages are in the store under `head + 1`, no
read can reach them, and the handle still holds them in its dirty buffer. -/
structure Handle where
  id : Nat
  fence : Nat
  head : Nat
  shard : Nat
  pending : Bool

structure State where
  store : Store
  a : Handle

/-- What other processes, and this one, do to the store.

`openRW` carries recovery with it, because that is the only caller of `weft_txn_recover`:
an opener first finishes any group whose intents are all present, moving that file's head
to the staged txid without touching its fence (`resolve_body`), and then raises the fence
(`open_body`). The window in which the owner's head is behind the store's is therefore
always closed by the fence moving, and inside it the owner's dirty buffer still holds every
page the resolution re-pointed, so its reads are the staged state either way. -/
inductive Event where
  /-- `vfs_open` read-write: recovery, then `raise_fence`, and the opener loads the view. -/
  | openRW (h : Nat)
  /-- `flush`: refused by `check_fence` unless `h` owns the fence; else the head moves. -/
  | commit (h : Nat)
  /-- `compact`: refused the same way; else the shard version becomes the head. -/
  | fold (h : Nat)
  /-- `txn_stage_here`: the owner's pages go under `head + 1`, unreachable, and stay in its
  dirty buffer. The store a read sees does not change. -/
  | stage (h : Nat)
  /-- `txn_resolve_part` by the owner: the head moves to the staged txid, by `head_body`
  under `check_fence`, and the buffer is cleared. -/
  | settle (h : Nat)

/-- What `weft_txn_recover` does to the store before an open raises the fence: a group the
owner staged and did not settle is finished, by moving the head to the staged txid. -/
def recover (s : State) : Store :=
  if s.a.pending ∧ s.store.owner = s.a.id ∧ s.store.head = s.a.head then
    { s.store with head := s.a.head + 1 }
  else s.store

theorem recover_fence (s : State) : (recover s).fence = s.store.fence := by
  unfold recover; split <;> rfl

def step (s : State) : Event → State
  | .openRW h =>
    let st := { recover s with fence := (recover s).fence + 1, owner := h }
    if h = s.a.id then
      { store := st,
        a := { s.a with fence := st.fence, head := st.head, shard := st.shard, pending := false } }
    else
      { s with store := st }
  | .commit h =>
    if s.store.owner = h then
      let st := { s.store with head := s.store.head + 1 }
      if h = s.a.id then
        if s.a.pending then s else { store := st, a := { s.a with head := st.head } }
      else { s with store := st }
    else s
  | .fold h =>
    if s.store.owner = h then
      let st := { s.store with shard := s.store.head }
      if h = s.a.id then { store := st, a := { s.a with shard := st.shard } }
      else { s with store := st }
    else s
  | .stage h =>
    if s.store.owner = h ∧ h = s.a.id then { s with a := { s.a with pending := true } } else s
  | .settle h =>
    if s.store.owner = h ∧ h = s.a.id ∧ s.a.pending then
      { store := { s.store with head := s.a.head + 1 },
        a := { s.a with head := s.a.head + 1, pending := false } }
    else s

def run : List Event → State → State
  | [], s => s
  | e :: es, s => run es (step s e)

/-- The view is coherent: a fence that still equals the handle's means the store's head and
shard are the handle's, and nobody else owns it. The bound on the fence is what the
induction needs, because a fence only ever rises. -/
def Inv (s : State) : Prop :=
  s.a.fence ≤ s.store.fence ∧
    (s.store.fence = s.a.fence →
      s.store.owner = s.a.id ∧ s.store.head = s.a.head ∧ s.store.shard = s.a.shard)

theorem step_inv (s : State) (e : Event) (h : Inv s) : Inv (step s e) := by
  rcases h with ⟨hle, hcoh⟩
  cases e with
  | openRW k =>
    simp only [step]
    split
    · exact ⟨Nat.le_refl _, fun _ => ⟨by simp_all, rfl, rfl⟩⟩
    · refine ⟨?_, fun hf => ?_⟩
      · show s.a.fence ≤ (recover s).fence + 1
        rw [recover_fence]; omega
      · exfalso
        have hf' : (recover s).fence + 1 = s.a.fence := hf
        rw [recover_fence] at hf'; omega
  | commit k =>
    simp only [step]
    split
    · split
      · split
        · exact ⟨hle, hcoh⟩
        · refine ⟨hle, fun hf => ?_⟩
          have := hcoh hf
          simp_all
      · refine ⟨hle, fun hf => ?_⟩
        have := hcoh hf
        simp_all
    · exact ⟨hle, hcoh⟩
  | fold k =>
    simp only [step]
    split
    · split
      · refine ⟨hle, fun hf => ?_⟩
        have := hcoh hf
        simp_all
      · refine ⟨hle, fun hf => ?_⟩
        have := hcoh hf
        simp_all
    · exact ⟨hle, hcoh⟩
  | stage k =>
    simp only [step]
    split
    · exact ⟨hle, fun hf => hcoh hf⟩
    · exact ⟨hle, hcoh⟩
  | settle k =>
    simp only [step]
    split
    · refine ⟨hle, fun hf => ?_⟩
      have := hcoh hf
      exact ⟨this.1, rfl, this.2.2⟩
    · exact ⟨hle, hcoh⟩

theorem run_inv (evs : List Event) (s : State) (h : Inv s) : Inv (run evs s) := by
  induction evs generalizing s with
  | nil => exact h
  | cons e es ih => exact ih _ (step_inv s e h)

/-- A handle that just opened read-write holds the fence it raised, and its view is the
store's. -/
def opened (st : Store) (id : Nat) : State :=
  step { store := st, a := { id := id, fence := 0, head := 0, shard := 0, pending := false } }
    (.openRW id)

theorem opened_inv (st : Store) (id : Nat) : Inv (opened st id) := by
  unfold opened
  simp only [step, if_true]
  exact ⟨Nat.le_refl _, fun _ => ⟨rfl, rfl, rfl⟩⟩

/-- **Coherence.** After any sequence of opens, commits and folds by anyone, a writer whose
check passes — the store's fence is still its own — reads a store with its head and its
shard version. That is the store its cache describes. -/
theorem coherent (st : Store) (id : Nat) (evs : List Event)
    (hpass : (run evs (opened st id)).store.fence = (run evs (opened st id)).a.fence) :
    (run evs (opened st id)).store.head = (run evs (opened st id)).a.head ∧
      (run evs (opened st id)).store.shard = (run evs (opened st id)).a.shard :=
  ((run_inv evs _ (opened_inv st id)).2 hpass).2

/-- A reader's check compares the head and the shard version themselves, so a passed check is
coherence by definition. -/
def readerCheck (store : Store) (head shard : Nat) : Bool :=
  store.head == head && store.shard == shard

theorem reader_coherent (store : Store) (head shard : Nat)
    (h : readerCheck store head shard = true) : store.head = head ∧ store.shard = shard := by
  simp [readerCheck] at h
  exact h

/-! ## The rule, both ways

A SQLite transaction may run on several FoundationDB read versions: the cached transaction
is replaced at the four-second mark, and a large commit stages. The rule is that every read
version it is served from is the same store state, or the read is refused. -/

/-- Two reads a reader's check let through, against one view, saw the same head and shard. -/
theorem served_reads_agree (s₁ s₂ : Store) (head shard : Nat)
    (h₁ : readerCheck s₁ head shard = true) (h₂ : readerCheck s₂ head shard = true) :
    s₁.head = s₂.head ∧ s₁.shard = s₂.shard := by
  have a := reader_coherent s₁ head shard h₁
  have b := reader_coherent s₂ head shard h₂
  exact ⟨a.1.trans b.1.symm, a.2.trans b.2.symm⟩

/-- The contrapositive: a store that is not the view's is refused. -/
theorem refused_unless_same (s : Store) (head shard : Nat)
    (h : ¬ (s.head = head ∧ s.shard = shard)) : readerCheck s head shard = false := by
  cases hc : readerCheck s head shard
  · rfl
  · exact absurd (reader_coherent s head shard hc) h

/-- The writer's side of the same rule, through `coherent`: two reads whose fence check passed
on the same handle saw the same head and shard, because each saw the handle's. -/
theorem writer_reads_agree (st : Store) (id : Nat) (evs₁ evs₂ : List Event)
    (h₁ : (run evs₁ (opened st id)).store.fence = (run evs₁ (opened st id)).a.fence)
    (h₂ : (run evs₂ (opened st id)).store.fence = (run evs₂ (opened st id)).a.fence)
    (same : (run evs₁ (opened st id)).a.head = (run evs₂ (opened st id)).a.head ∧
      (run evs₁ (opened st id)).a.shard = (run evs₂ (opened st id)).a.shard) :
    (run evs₁ (opened st id)).store.head = (run evs₂ (opened st id)).store.head ∧
      (run evs₁ (opened st id)).store.shard = (run evs₂ (opened st id)).store.shard := by
  have a := coherent st id evs₁ h₁
  have b := coherent st id evs₂ h₂
  exact ⟨a.1.trans (same.1.trans b.1.symm), a.2.trans (same.2.trans b.2.symm)⟩

end Weft.ViewCoherence
