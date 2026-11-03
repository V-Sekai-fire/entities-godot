/-! # A commit sends the bytes a page changed, not the page

A commit today writes each dirty page whole. Measured on the YCSB F table (25 000 rows of
ten 100-byte fields), one commit of 1000 updates dirties 927 pages, 3.8 MB as whole pages
and 146 KB as changed bytes with their offsets: 26 times less. The commit is where the run
spends its time, so this is the change that moves the benchmark.

mvSQLite (losfair/mvsqlite ce008f1, `mvstore/src/delta/writer.rs`) does the same on the
same database: a page is stored as its difference from a full page, kept only when under a
fifth of the bytes differ, and the base of a difference is always a full page, never
another difference. This file models that rule over the store's log and proves what the C
must keep true.

* `patch_diff`: a difference applied to the page it was taken from gives the new page.
* `read_after_commit`: the page a commit wrote reads back, whichever row form it chose.
* `staged_rows_are_invisible`: a row under a txid above the head changes no read, so pages
  may be sent ahead during the transaction and made visible by moving the head.
* `fold_preserves_read`: compaction may replace the log with one full page.
* `read_touches_two_rows`: a read costs at most two rows, because a base is always full.

What the model does not cover is FoundationDB itself. Every row here is written by a
transaction, the head moves in one transaction, and the base a writer diffs against comes
from the writer's own memory rather than a snapshot read, so the C uses no FoundationDB
operation it did not use before. Serializability and durability are FoundationDB's, and
this change neither weakens nor relies on anything beyond them.

SPDX-License-Identifier: Apache-2.0
-/

namespace Weft.PageDelta

abbrev Page := List Nat

/-- One slot per byte: `none` where the byte is unchanged, `some b` where it is now `b`.
The C encodes the `some` runs as offset and bytes; that encoding is not modelled. -/
abbrev Delta := List (Option Nat)

def diff : Page → Page → Delta
  | a :: as, b :: bs => (if a = b then none else some b) :: diff as bs
  | _, _ => []

def patch : Page → Delta → Page
  | a :: as, o :: os => o.getD a :: patch as os
  | as, [] => as
  | [], _ :: _ => []

/-- The bytes a delta carries, which is what the commit pays for. -/
def changed (d : Delta) : Nat := d.countP Option.isSome

theorem patch_diff : ∀ (old new : Page), old.length = new.length →
    patch old (diff old new) = new
  | [], [], _ => rfl
  | [], _ :: _, h => by simp at h
  | _ :: _, [], h => by simp at h
  | a :: as, b :: bs, h => by
      have ih := patch_diff as bs (by simpa using h)
      simp only [diff, patch, ih]
      by_cases hab : a = b <;> simp [hab]

/-! ## The log of one page -/

inductive Row where
  | full (p : Page)
  /-- The difference from the full row at txid `base`. -/
  | delta (base : Nat) (d : Delta)
  deriving DecidableEq, Repr

/-- Newest first. A row is `(txid, row)`; the head is the newest txid a read may see. -/
abbrev Log := List (Nat × Row)

def rowAt (l : Log) (t : Nat) : Option Row := (l.find? (fun e => e.1 = t)).map (·.2)

def visible (l : Log) (h : Nat) : Option (Nat × Row) := l.find? (fun e => decide (e.1 ≤ h))

def read (l : Log) (h : Nat) : Option Page :=
  match visible l h with
  | some (_, .full p) => some p
  | some (_, .delta b d) =>
    match rowAt l b with
    | some (.full p) => some (patch p d)
    | _ => none
  | none => none

/-- The txid of the full row a writer diffs against: the visible row if it is full, else
its base. -/
def baseTx (l : Log) (h : Nat) : Option Nat :=
  match visible l h with
  | some (t, .full _) => some t
  | some (_, .delta b _) => some b
  | none => none

/-- That full row's page. The writer holds it in memory, because reading the page to
commit it already read it. -/
def baseOf (l : Log) (h : Nat) : Option (Nat × Page) :=
  match baseTx l h with
  | some bt =>
    match rowAt l bt with
    | some (.full p) => if bt ≤ h then some (bt, p) else none
    | _ => none
  | none => none

/-- Store the difference when it is small, else the page. `cut` is 5 in mvSQLite: a delta
is kept when fewer than a fifth of the bytes changed. -/
def encode (cut bt : Nat) (base new : Page) : Row :=
  if base.length = new.length ∧ changed (diff base new) * cut < new.length then
    .delta bt (diff base new)
  else
    .full new

def commit (l : Log) (t : Nat) (r : Row) : Log := (t, r) :: l

/-- A page's first commit has no base, so it is full. Every later commit encodes against
`baseOf` at the head it read. -/
def write (cut : Nat) (l : Log) (h t : Nat) (new : Page) : Log :=
  match baseOf l h with
  | some (bt, bp) => commit l t (encode cut bt bp new)
  | none => commit l t (.full new)

/-! ## What the C keeps true -/

theorem baseOf_spec {l : Log} {h bt : Nat} {bp : Page} (hb : baseOf l h = some (bt, bp)) :
    rowAt l bt = some (.full bp) ∧ bt ≤ h := by
  unfold baseOf at hb
  cases hbt : baseTx l h with
  | none => simp [hbt] at hb
  | some bt' =>
    rw [hbt] at hb
    cases hr : rowAt l bt' with
    | none => simp [hr] at hb
    | some r =>
      cases r with
      | delta b d => simp [hr] at hb
      | full p =>
        simp only [hr] at hb
        by_cases hle : bt' ≤ h
        · simp [hle] at hb
          obtain ⟨rfl, rfl⟩ := hb
          exact ⟨hr, hle⟩
        · simp [hle] at hb

theorem rowAt_cons_ne {l : Log} {t b : Nat} {r : Row} (hne : b ≠ t) :
    rowAt ((t, r) :: l) b = rowAt l b := by
  simp [rowAt, Ne.symm hne]

/-- **The round trip.** Whatever row the commit chose, reading at the new head gives the
page that was written. -/
theorem read_after_commit (cut : Nat) (l : Log) (h t : Nat) (new : Page) (ht : h < t) :
    read (write cut l h t new) t = some new := by
  unfold write
  split
  · rename_i bt bp hb
    obtain ⟨hrow, hle⟩ := baseOf_spec hb
    have hne : bt ≠ t := by omega
    unfold encode
    split
    · rename_i hsmall
      simp only [read, visible, commit, List.find?_cons, Nat.le_refl, decide_true]
      rw [rowAt_cons_ne hne, hrow]
      simp [patch_diff bp new hsmall.1]
    · simp [read, visible, commit]
  · simp [read, visible, commit]

/-- A delta's base sits below its own row. The writer gets this from `baseOf`, which only
returns a txid at or below the head it read, and the commit's txid is above that head. -/
def BasesBelow (l : Log) : Prop := ∀ e ∈ l, ∀ b d, e.2 = .delta b d → b < e.1

/-- **Sending pages ahead.** Rows staged under a txid above the head change no read at the
head. So the pages of a large commit can go out while the transaction runs, and the commit
itself only moves the head. -/
theorem staged_rows_are_invisible (l : Log) (h t : Nat) (r : Row) (ht : h < t)
    (hwf : BasesBelow l) : read ((t, r) :: l) h = read l h := by
  have hvis : visible ((t, r) :: l) h = visible l h := by
    have : ¬ t ≤ h := by omega
    simp [visible, this]
  unfold read
  rw [hvis]
  split
  · rfl
  · rename_i e b d hv
    have he : (e, Row.delta b d) ∈ l := List.mem_of_find?_eq_some hv
    have hle : e ≤ h := by
      have := List.find?_some hv
      simpa using this
    have hb : b < e := hwf _ he b d rfl
    rw [rowAt_cons_ne (by omega)]
  · rfl

/-- **Compaction.** Folding the log into one full row at the head reads the same. -/
def fold (l : Log) (h : Nat) : Log :=
  match read l h with
  | some p => [(h, .full p)]
  | none => []

theorem fold_preserves_read (l : Log) (h : Nat) : read (fold l h) h = read l h := by
  unfold fold
  split
  · rename_i p hp
    rw [hp]
    simp [read, visible]
  · rename_i hp
    rw [hp]
    simp [read, visible]

/-- The rows a read looks at: the visible one, and its base when it is a delta. -/
def rowsRead (l : Log) (h : Nat) : Nat :=
  match visible l h with
  | some (_, .full _) => 1
  | some (_, .delta _ _) => 2
  | none => 0

/-- **Read cost.** A base is always full, so no read follows a chain. -/
theorem read_touches_two_rows (l : Log) (h : Nat) : rowsRead l h ≤ 2 := by
  unfold rowsRead
  split <;> omega

/-- The delta a commit sends is under the cut, so it is smaller than the page. -/
theorem delta_is_small (cut bt : Nat) (base new : Page) (b : Nat) (d : Delta)
    (he : encode cut bt base new = .delta b d) : changed d * cut < new.length := by
  unfold encode at he
  split at he
  · rename_i hs
    injection he with _ hd
    rw [← hd]
    exact hs.2
  · cases he

/-! ## Rule-2 controls: each wrong version fails a property above -/

def page0 : Page := [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
def page1 : Page := [7, 0, 0, 0, 0, 0, 0, 0, 0, 0]
def page2 : Page := [7, 9, 0, 0, 0, 0, 0, 0, 0, 0]
def log0 : Log := [(1, .full page0)]

/-- The mvSQLite rule, as `write` applies it: two commits, each a delta on txid 1. -/
example : read (write 5 (write 5 log0 1 2 page1) 2 3 page2) 3 = some page2 := by decide

/-- A writer that takes the previous version as its base, which may be a delta. The read
follows one link, so a chain loses the page. -/
def writeChained (cut : Nat) (l : Log) (h t : Nat) (new : Page) (prev : Page) : Log :=
  commit l t (encode cut h prev new)

theorem chained_base_loses_the_page :
    read (writeChained 5 (write 5 log0 1 2 page1) 2 3 page2 page1) 3 ≠ some page2 := by
  decide

/-- A writer diffing against a page it did not read from the store: a base held across a
change it missed. The delta applies to the stored base and gives the wrong page. -/
def writeStaleBase (cut : Nat) (l : Log) (t bt : Nat) (held new : Page) : Log :=
  commit l t (encode cut bt held new)

theorem stale_base_reads_wrong :
    read (writeStaleBase 5 log0 2 1 page1 page2) 2 ≠ some page2 := by
  decide

/-- A row staged at or below the head is visible before its commit. -/
theorem staged_at_head_leaks : read ((1, .full page2) :: log0) 1 ≠ read log0 1 := by
  decide

/-- Without the length check a delta drops the bytes past the old page's end. -/
theorem patch_needs_equal_lengths : patch [1, 2] (diff [1, 2] [1, 2, 3]) ≠ [1, 2, 3] := by
  decide

end Weft.PageDelta
