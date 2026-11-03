/-! # Committing across several databases

`ParallelCommits.tla` is the authority for this protocol, and it lives in a CockroachDB fork
as 919 lines of TLA+ that nothing here can run. This is the same protocol stated against
*this* layout, in the language the rest of this store is specified in, so that CI checks it.

It is not a translation. CockroachDB has intents and a timestamp cache; this has pages
staged under a txid whose head has not moved, and a fence. The mapping was a comment in
`fdb_vfs.c` and is an argument rather than an observation:

| ParallelCommits | here |
| --- | --- |
| intent write | a participant's pages, staged under a txid above its head |
| transaction record | `weft/txn/<txnid>` |
| implicitly committed | the record is staging and every intent is present |
| timestamp cache | the fence |
| preventer | recovery, run by whoever finds a staging record |

The properties below are in two groups. The first are the TLA spec's own. The second are
ones it cannot state, because it has no pages — and the first of those is the one that was
actually broken in the C, at the cost of an item on a live cluster.
-/

namespace Weft.ParallelCommit

abbrev Db := Nat
abbrev Txid := Nat

/-- One database, as much of it as this protocol can see.

`head` is the newest commit, and a page staged under a txid above it is unreachable: no
PIDX row points at it and no read can find it. `fence` is what a stale writer is refused
by. -/
structure DbState where
  head : Txid
  fence : Nat
  /-- Whether pages exist under this txid. This is `QueryIntent`. -/
  staged : Txid → Bool

/-- A write is visible exactly when the head has reached it. Staging alone never is. -/
def DbState.visible (d : DbState) (tx : Txid) : Bool := tx ≤ d.head

inductive Status where
  | pending | staging | committed | aborted
  deriving Repr, DecidableEq

/-- The transaction record: what state the group is in, and who is in it. -/
structure Record where
  status : Status
  parts : List (Db × Txid)

structure World where
  db : Db → DbState
  txn : Record

/-- `QueryIntent` for one participant. -/
def intentPresent (w : World) (p : Db × Txid) : Bool := (w.db p.1).staged p.2

def allIntentsPresent (w : World) : Bool := w.txn.parts.all (intentPresent w)

/-- The commit point. It costs one round: the record landing is what makes it true, and
every intent is already durable by then. -/
def implicitlyCommitted (w : World) : Bool :=
  w.txn.status == Status.staging && allIntentsPresent w

def explicitlyCommitted (w : World) : Bool := w.txn.status == Status.committed

def committed (w : World) : Bool := implicitlyCommitted w || explicitlyCommitted w

def finalized (w : World) : Bool :=
  w.txn.status == Status.committed || w.txn.status == Status.aborted

/-! ## The operations -/

def setDb (w : World) (d : Db) (s : DbState) : World :=
  { w with db := fun x => if x = d then s else w.db x }

/-- Write one participant's intent: pages under `head + 1`, and the head does not move. -/
def stage (w : World) (d : Db) : World :=
  let s := w.db d
  let tx := s.head + 1
  let w' := setDb w d { s with staged := fun t => t == tx || s.staged t }
  { w' with txn := { w'.txn with parts := (d, tx) :: w'.txn.parts } }

/-- The commit: one transaction moves the record to staging. -/
def writeRecord (w : World) : World :=
  { w with txn := { w.txn with status := Status.staging } }

/-- Resolve one participant, which is what makes its staged pages reachable. -/
def resolve (w : World) (p : Db × Txid) : World :=
  let s := w.db p.1
  setDb w p.1 { s with head := max s.head p.2 }

/-- Prevent: raise the fence, so a writer holding the old one is refused. This is the
timestamp cache, and it is the only mechanism here that can stop a write that has not
happened yet. -/
def prevent (w : World) (d : Db) : World :=
  let s := w.db d
  setDb w d { s with fence := s.fence + 1 }

def finish (w : World) : World := { w with txn := { w.txn with status := Status.committed } }
def abort (w : World) : World := { w with txn := { w.txn with status := Status.aborted } }

/-! ## What the TLA spec says

A finalized record stays finalized, and recovery only ever moves a staging record to one of
the two ends. -/

theorem finish_is_final (w : World) : finalized (finish w) := by
  simp [finalized, finish]

theorem abort_is_final (w : World) : finalized (abort w) := by
  simp [finalized, abort]

/-- An implicitly committed group is committed, which is what lets a caller be told so
before any head has moved. -/
theorem implicit_is_committed (w : World) (h : implicitlyCommitted w) : committed w := by
  simp [committed, h]

/-- Finishing an implicitly committed group keeps it committed. This is
`ImplicitCommitLeadsToExplicitCommit`, the step recovery performs. -/
theorem finish_keeps_committed (w : World) : committed (finish w) := by
  simp [committed, explicitlyCommitted, finish]

/-! ## What the TLA spec cannot say, because it has no pages -/

/-- **Staging is invisible.**

The property that was broken. `weft_txn_stage` asked a caller to stage *after* writing, but
SQLite ends a statement at `xSync` and that is where `flush` commits, so the first write
became visible on its own and the group never held it. It cost an item on a live cluster.

Staging puts pages under `head + 1` and leaves the head alone, so nothing a participant
staged can be read before the record lands. -/
theorem stage_is_invisible (w : World) (d : Db) :
    ¬ ((stage w d).db d).visible ((w.db d).head + 1) := by
  simp [stage, setDb, DbState.visible]

/-- And the head is exactly what it was, which is the same fact said the other way. -/
theorem stage_moves_no_head (w : World) (d : Db) :
    ((stage w d).db d).head = (w.db d).head := by
  simp [stage, setDb]

/-- **Resolving is what makes it visible**, and only after the record. -/
theorem resolve_is_visible (w : World) (d : Db) (tx : Txid) :
    ((resolve w (d, tx)).db d).visible tx := by
  simp [resolve, setDb, DbState.visible, Nat.le_max_right]

/-- **The bug, as a counterexample.** An implementation that commits the write first and
stages afterwards makes it visible with no record at all.

`w₀` is a database at head 3. Committing the ordinary way moves the head to 4, and the
write is readable while the record is still pending — so a crash here leaves the item gone
from one database and never arrived in the other. -/
def w₀ : World :=
  { db := fun _ => { head := 3, fence := 1, staged := fun _ => false }
    txn := { status := Status.pending, parts := [] } }

/-- What a plain `flush` does: the head moves. -/
def commitAlone (w : World) (d : Db) : World :=
  let s := w.db d
  setDb w d { s with head := s.head + 1, staged := fun t => t == s.head + 1 || s.staged t }

theorem committing_before_staging_is_visible_with_no_record :
    ((commitAlone w₀ 0).db 0).visible 4 = true ∧ (commitAlone w₀ 0).txn.status = Status.pending := by
  constructor
  · rfl
  · rfl

/-- Whereas staging leaves it unreadable, which is what the group needs. -/
theorem staging_leaves_it_unreadable : ((stage w₀ 0).db 0).visible 4 = false := by rfl

/-! ## Recovery -/

/-- A world where the record staged and one participant's intent never landed. Recovery
must not commit this: it can never become implicitly committed. -/
def missingIntent : World :=
  { db := fun d => { head := 3, fence := 1, staged := fun t => d == 0 && t == 4 }
    txn := { status := Status.staging, parts := [(0, 4), (1, 4)] } }

theorem missing_intent_is_not_committed : implicitlyCommitted missingIntent = false := by
  decide

/-- A world where every intent landed. Recovery must finish this, whatever the committer
wanted, because it is already committed. -/
def allPresent : World :=
  { db := fun _ => { head := 3, fence := 1, staged := fun t => t == 4 }
    txn := { status := Status.staging, parts := [(0, 4), (1, 4)] } }

theorem all_present_is_committed : implicitlyCommitted allPresent = true := by decide

/-- **Preventing does not change the decision.** Raising a fence stops a future write; it
must not make a group that was already implicitly committed look uncommitted, or recovery
would abort a commit that happened. -/
theorem prevent_keeps_the_decision (w : World) (d : Db) :
    implicitlyCommitted (prevent w d) = implicitlyCommitted w := by
  have hp : intentPresent (prevent w d) = intentPresent w := by
    funext p
    by_cases h : p.1 = d <;> simp [intentPresent, prevent, setDb, h]
  simp only [implicitlyCommitted, allIntentsPresent, hp]
  rfl

/-- **Recovery is idempotent.** Two processes deciding one record reach the same end, which
is what lets any finder of a staging record decide it. -/
theorem finish_twice (w : World) : finish (finish w) = finish w := by
  simp [finish]

theorem abort_twice (w : World) : abort (abort w) = abort w := by
  simp [abort]

/-- Resolving a participant twice moves the head no further, so a resolve that runs again
after a crash is harmless. -/
theorem resolve_twice (w : World) (p : Db × Txid) :
    ((resolve (resolve w p) p).db p.1).head = ((resolve w p).db p.1).head := by
  simp [resolve, setDb]

/-! ## The legal steps

The properties above are about single operations. The TLA spec's are about every run: a
record that is committed stays committed, whatever happens next. That needs a relation
saying which steps are allowed at all, and writing it down turns out to be where the
protocol's guards live.

Two of them are not decoration:

* `stage` and `record` are only legal while the record is pending. Staging into a group
  that has already committed would add a participant whose intent nobody checked, and the
  group would stop being committed after it already was.
* `abort` is only legal when the group is **not** implicitly committed. Recovery obeys this
  — it aborts only on a missing intent — and without it recovery could abort a commit that
  had already happened, which is the whole failure the protocol exists to prevent. -/
inductive Step : World → World → Prop where
  | stage (w : World) (d : Db) : w.txn.status = Status.pending → Step w (stage w d)
  | record (w : World) : w.txn.status = Status.pending → Step w (writeRecord w)
  | resolve (w : World) (p : Db × Txid) : Step w (resolve w p)
  | prevent (w : World) (d : Db) : Step w (prevent w d)
  | finish (w : World) : implicitlyCommitted w = true → Step w (finish w)
  | abort (w : World) :
      implicitlyCommitted w = false → w.txn.status ≠ Status.committed → Step w (abort w)

/-- **Once committed, always committed.** `[](Committed => []Committed)` for one step, and
so for a run by induction.

The interesting cases are the two guards. A step that would un-commit a group is one the
relation does not admit. -/
theorem committed_stable {w w' : World} (h : Step w w') (hc : committed w = true) :
    committed w' = true := by
  cases h with
  | stage d hp =>
    exfalso
    simp [committed, implicitlyCommitted, explicitlyCommitted, hp] at hc
  | record hp =>
    exfalso
    simp [committed, implicitlyCommitted, explicitlyCommitted, hp] at hc
  | resolve p =>
    have hp : intentPresent (resolve w p) = intentPresent w := by
      funext q
      by_cases hq : q.1 = p.1 <;> simp [intentPresent, resolve, setDb, hq]
    have he : committed (resolve w p) = committed w := by
      simp only [committed, implicitlyCommitted, explicitlyCommitted, allIntentsPresent, hp]
      rfl
    rw [he]; exact hc
  | prevent d =>
    have hp : intentPresent (prevent w d) = intentPresent w := by
      funext q
      by_cases hq : q.1 = d <;> simp [intentPresent, prevent, setDb, hq]
    have he : committed (prevent w d) = committed w := by
      simp only [committed, implicitlyCommitted, explicitlyCommitted, allIntentsPresent, hp]
      rfl
    rw [he]; exact hc
  | finish _ => exact finish_keeps_committed _
  | abort hi hne =>
    exfalso
    simp [committed, implicitlyCommitted, explicitlyCommitted] at hc
    rcases hc with hc | hc
    · have hy : implicitlyCommitted w = true := by
        simp [implicitlyCommitted, hc.1, hc.2]
      rw [hi] at hy
      exact Bool.noConfusion hy
    · exact hne hc

/-- Once a record is committed it stays committed, and once aborted it stays aborted. -/
theorem committed_status_stable {w w' : World} (h : Step w w')
    (hc : w.txn.status = Status.committed) : w'.txn.status = Status.committed := by
  cases h with
  | stage d hp => exfalso; rw [hp] at hc; exact Status.noConfusion hc
  | record hp => exfalso; rw [hp] at hc; exact Status.noConfusion hc
  | resolve p => simpa [resolve, setDb] using hc
  | prevent d => simpa [prevent, setDb] using hc
  | finish _ => simp [finish]
  | abort _ hne => exact absurd hc hne

/-! ## What only ever grows

`TemporalTSCacheProperties` says the timestamp cache always advances. Here that is the
fence, and it is the same property: a fence that could fall would un-prevent a writer that
had been refused. The head is the same shape of fact for reads. -/

theorem fence_never_falls {w w' : World} (h : Step w w') (d : Db) :
    (w.db d).fence ≤ (w'.db d).fence := by
  cases h with
  | stage e _ => by_cases hd : d = e <;> simp [stage, setDb, hd]
  | record _ => simp [writeRecord]
  | resolve p => by_cases hd : d = p.1 <;> simp [resolve, setDb, hd]
  | prevent e => by_cases hd : d = e <;> simp [prevent, setDb, hd]
  | finish _ => simp [finish]
  | abort _ _ => simp [abort]

theorem head_never_falls {w w' : World} (h : Step w w') (d : Db) :
    (w.db d).head ≤ (w'.db d).head := by
  cases h with
  | stage e _ => by_cases hd : d = e <;> simp [stage, setDb, hd]
  | record _ => simp [writeRecord]
  | resolve p =>
    by_cases hd : d = p.1 <;> simp [resolve, setDb, hd, Nat.le_max_left]
  | prevent e => by_cases hd : d = e <;> simp [prevent, setDb, hd]
  | finish _ => simp [finish]
  | abort _ _ => simp [abort]

/-! ## Runs

A run is any sequence of legal steps. The stability above lifts to it by induction, which
is what `[](Committed => []Committed)` means. -/

inductive Reachable : World → World → Prop where
  | refl (w : World) : Reachable w w
  | tail {w v u : World} : Reachable w v → Step v u → Reachable w u

theorem committed_stays_committed {w w' : World} (r : Reachable w w')
    (hc : committed w = true) : committed w' = true := by
  induction r with
  | refl => exact hc
  | tail _ hs ih => exact committed_stable hs ih

theorem fence_never_falls_in_a_run {w w' : World} (r : Reachable w w') (d : Db) :
    (w.db d).fence ≤ (w'.db d).fence := by
  induction r with
  | refl => exact Nat.le_refl _
  | tail _ hs ih => exact Nat.le_trans ih (fence_never_falls hs d)

/-! ## Acknowledging, and finishing

`AckImpliesCommit` and `AckLeadsToExplicitCommit`. A caller may be told the commit happened
exactly when the record has landed with every intent present, which is one round. -/

def mayAck (w : World) : Bool := implicitlyCommitted w

theorem ack_implies_commit (w : World) (h : mayAck w = true) : committed w = true := by
  simp [committed, mayAck] at *
  simp [h]

/-- Recovery, as a function rather than a process: whoever finds a staging record decides
it, and the decision is forced. -/
def recover (w : World) : World :=
  if w.txn.status = Status.staging then
    if implicitlyCommitted w then finish w else abort w
  else w

/-- **A staging record always ends finalized.** This is what `<>[]RecordFinalized` asks for,
without the temporal machinery: recovery is total, so a record cannot sit staging once
anybody looks. -/
theorem recover_finalizes (w : World) (h : w.txn.status = Status.staging) :
    finalized (recover w) = true := by
  by_cases hi : implicitlyCommitted w <;> simp [recover, finalized, finish, abort, h, hi]

/-- Recovery only ever takes a legal step, which is what makes the guards above true of it
rather than merely stated near it. -/
theorem recover_is_a_step (w : World) (h : w.txn.status = Status.staging) :
    Step w (recover w) := by
  by_cases hi : implicitlyCommitted w
  · simpa [recover, h, hi] using Step.finish w hi
  · have hne : w.txn.status ≠ Status.committed := by rw [h]; exact Status.noConfusion
    simpa [recover, h, hi] using Step.abort w (by simpa using hi) hne

/-- **An acknowledged commit becomes explicit.** `AckLeadsToExplicitCommit`: if a caller was
told the group committed, recovery finishes it rather than aborting it. -/
theorem ack_leads_to_explicit (w : World) (h : mayAck w = true) :
    explicitlyCommitted (recover w) = true := by
  have hs : w.txn.status = Status.staging := by
    simp [mayAck, implicitlyCommitted] at h
    exact h.1
  simp [recover, hs, explicitlyCommitted, finish, mayAck] at *
  simp [h]

/-- And recovery run twice is recovery run once. -/
theorem recover_idempotent (w : World) : recover (recover w) = recover w := by
  by_cases hs : w.txn.status = Status.staging
  · by_cases hi : implicitlyCommitted w <;> simp [recover, hs, hi, finish, abort]
  · simp [recover, hs]

/-! ## Two things this model left out, and the defects they hide

The model above says a txid is the only ordering this layout carries, and that a write is
visible once the head reaches it. Both are simplifications, and each one is standing in
front of a real fault in `fdb_vfs.c`.

### A txid is reused, and nothing says whose pages are under it

CockroachDB retries at a higher epoch, so a record from a dead attempt can never be mistaken
for the live one. This layout has no epoch: a participant stages under `head + 1`, and a
group that gives up leaves those pages exactly where the next group will stage.

`weft_txn_abort` releases the lock and clears no pages, and `intentPresent` asks only whether
*something* is staged under the txid. So a group can be judged implicitly committed on a dead
group's pages. -/

abbrev Group := Nat

/-- A database that remembers which group staged under each txid. -/
structure Staging where
  head : Txid
  stagedBy : Txid → Option Group

/-- `QueryIntent` as `fdb_vfs.c` asks it: is anything there? -/
def intentLoose (s : Staging) (tx : Txid) : Bool := (s.stagedBy tx).isSome

/-- `QueryIntent` as it has to be asked: is *this group's* intent there? -/
def intentExact (s : Staging) (tx : Txid) (g : Group) : Bool := (s.stagedBy tx) == some g

/-- Group 1 staged under txid 4 and gave up without clearing. The head never moved, so group
2 will stage under 4 as well — and here it has not yet done so. -/
def deadGroupLeftovers : Staging :=
  { head := 3, stagedBy := fun t => if t == 4 then some 1 else none }

/-- **The defect.** Group 2's intent is not there, and the loose check says it is.

A record naming `(db, 4)` for group 2 is then implicitly committed on group 1's pages, and
what lands is a mixture of two groups' writes. An epoch, or clearing on abort, or a txid that
is never reused, each close it; having none of the three does not. -/
theorem loose_intent_accepts_a_dead_group :
    intentLoose deadGroupLeftovers 4 = true ∧ intentExact deadGroupLeftovers 4 2 = false := by
  constructor <;> rfl

/-- Asked exactly, the same state is decided correctly and the group aborts. -/
theorem exact_intent_rejects_a_dead_group : intentExact deadGroupLeftovers 4 2 = false := by
  rfl

/-- And a group that really did stage is still accepted, so the fix refuses nothing real. -/
def ownStaging : Staging := { head := 3, stagedBy := fun t => if t == 4 then some 2 else none }

theorem exact_intent_accepts_its_own : intentExact ownStaging 4 2 = true := by rfl

/-! ### A head may move over a page that was never indexed

`visible tx = tx ≤ head` assumes resolving a participant indexes every page it staged. It
does not. `query_intent_body` enumerates the staged pages with a range read, stops at 4096,
and ignores `more`; `resolve_body` writes a PIDX row for exactly the pages it was handed and
then advances the head regardless.

A page left out has no index row, so a read falls through to the shard and finds the version
from before the commit. The head says the commit landed. -/

abbrev Pgno := Nat

/-- A database as a read actually sees it: an index, a log, and a base. -/
structure Paged where
  head : Txid
  /-- PIDX: which txid owns a page. -/
  index : Pgno → Option Txid
  /-- The pages one commit staged. -/
  staged : Pgno → Option Nat
  /-- The compacted base, which is what a page without an index row falls back to. -/
  base : Pgno → Option Nat

/-- The read path of `page_from_store`, with no read-ahead in the way. -/
def Paged.read (d : Paged) (p : Pgno) : Option Nat :=
  match d.index p with
  | some _ => d.staged p
  | none => d.base p

/-- Resolving, as `resolve_body` does it: index the pages that were enumerated, then move the
head whatever happened. `covered` is what the range read returned. -/
def resolvePaged (d : Paged) (tx : Txid) (covered : Pgno → Bool) : Paged :=
  { d with
    head := tx
    index := fun p => if covered p ∧ (d.staged p).isSome then some tx else d.index p }

/-- A commit that staged two pages, over a base that still holds the old values. -/
def midCommit : Paged :=
  { head := 3
    index := fun _ => none
    staged := fun p => if p == 0 then some 10 else if p == 1 then some 11 else none
    base := fun p => if p == 0 then some 1 else if p == 1 then some 2 else none }

/-- **The defect.** The enumeration stopped after page 0, so page 1 was never indexed. The
head moved anyway, and page 1 reads as the value from before the commit.

Nothing reports it. The database is structurally perfect and one page is a version behind,
which is the fault `prove_crash` looks for arriving by a different route. -/
theorem truncated_resolve_reads_the_old_page :
    (resolvePaged midCommit 4 (fun p => p == 0)).read 1 = some 2 := by rfl

/-- What it should have read, and does when the enumeration covered the commit. -/
theorem full_resolve_reads_the_new_page :
    (resolvePaged midCommit 4 (fun _ => true)).read 1 = some 11 := by rfl

/-- The obligation, stated once: a resolve may only move the head when every staged page was
indexed. Below that condition the read agrees with the commit for every page. -/
def indexesEverything (d : Paged) (covered : Pgno → Bool) : Prop :=
  ∀ p, (d.staged p).isSome → covered p

theorem resolve_is_honest_when_it_covers_everything
    (d : Paged) (tx : Txid) (covered : Pgno → Bool) (h : indexesEverything d covered)
    (p : Pgno) (hp : (d.staged p).isSome) :
    (resolvePaged d tx covered).read p = d.staged p := by
  have hc : covered p = true := h p hp
  simp [resolvePaged, Paged.read, hc, hp]

end Weft.ParallelCommit
