/-! # Frontier learning over the store's attacks (RFD 2323)

A level is an attack template with fixed attributes; a problem is one run of it. Each level
runs on two targets: the store as it is, and the store with the defect the level exists to
catch planted in it. On the planted target, success is a witness found, and regret
`1[s ≥ 1] - s/n` over the last eight runs marks the levels that find their defect only by
luck. A clean run on the real store counts only once its level is calibrated: it found its
planted defect in each of its last eight runs.

Families are triaged in a fixed order when the budget is short: performance first, then
round trip (data written reads back whole), then security.
-/

namespace Weft.Frontier

def window : Nat := 8
def stalenessWeight : Float := 0.05
/-- A timed run is slow when it reaches less than this share of the slowest recent real run.
The slowest, not the median: a 5% margin under the median flagged a clean run as slow. -/
def slowShare : Float := 0.97

inductive Family | perf | roundTrip | security
  deriving BEq, Repr, Inhabited

def Family.rank : Family → Nat
  | .perf => 0 | .roundTrip => 1 | .security => 2

def Family.name : Family → String
  | .perf => "performance" | .roundTrip => "round trip" | .security => "security"

/-- The attack a level runs, decoded from its id. -/
inductive Kind
  | crash (rows lo hi : Nat)
  | fence (mode : String)
  | perf (rows reps : Nat)
  /-- A page-delta plant, read back through two folds by `ycsb_direct --verify`. -/
  | delta (plant : String) (rows : Nat)
  /-- The YCSB run phase rate, the number the roundtable compares. -/
  | ycsb (rows workers batch : Nat)
  deriving BEq, Repr, Inhabited

def Kind.family : Kind → Family
  | .crash .. => .roundTrip | .fence _ => .security | .perf .. => .perf
  | .delta .. => .roundTrip | .ycsb .. => .perf

def Kind.plant : Kind → String
  | .crash .. => "WEFT_PLANT_TORN_COMMIT"
  | .fence _ => "WEFT_PLANT_STALE_VIEW"
  | .perf .. => "WEFT_PLANT_SLOW_COMMIT"
  | .delta p _ => "WEFT_PLANT_DELTA_" ++ p.toUpper
  | .ycsb .. => "WEFT_PLANT_SLOW_COMMIT"

def deltaPlants : List String := ["chain", "stale", "fold"]

def Kind.id : Kind → String
  | .crash r lo hi => s!"crash/rows={r}/at={lo}..{hi}"
  | .fence m => s!"fence/{m}"
  | .perf r k => s!"perf/rows={r}/reps={k}"
  | .delta p r => s!"delta/{p}/rows={r}"
  | .ycsb r w b => s!"ycsb/rows={r}/workers={w}/batch={b}"

def field (pre s : String) : Option String :=
  if s.startsWith pre then some (s.drop pre.length).toString else none

def parseKind (id : String) : Option Kind :=
  match id.splitOn "/" with
  | ["crash", r, at_] => do
      let rows ← (← field "rows=" r).toNat?
      match (← field "at=" at_).splitOn ".." with
      | [lo, hi] => do
          let lo ← lo.toNat?
          let hi ← hi.toNat?
          if lo ≤ hi && 0 < lo then some (.crash rows lo hi) else none
      | _ => none
  | ["fence", m] => if m == "writer" || m == "reader" then some (.fence m) else none
  | ["perf", r, k] => do
      let rows ← (← field "rows=" r).toNat?
      let reps ← (← field "reps=" k).toNat?
      if 0 < reps then some (.perf rows reps) else none
  | ["delta", p, r] => do
      let rows ← (← field "rows=" r).toNat?
      if deltaPlants.contains p && 0 < rows then some (.delta p rows) else none
  | ["ycsb", r, w, b] => do
      let rows ← (← field "rows=" r).toNat?
      let workers ← (← field "workers=" w).toNat?
      let batch ← (← field "batch=" b).toNat?
      if 0 < rows && 0 < workers && 0 < batch then some (.ycsb rows workers batch) else none
  | _ => none

inductive Target | real | planted
  deriving BEq, Repr, Inhabited

def Target.name : Target → String | .real => "real" | .planted => "planted"

/-- `baseline` is a timed run on the real store, kept to judge later runs, not judged itself. -/
inductive Outcome | witness | clean | unreached | baseline
  deriving BEq, Repr, Inhabited

def Outcome.name : Outcome → String
  | .witness => "witness" | .clean => "clean" | .unreached => "unreached"
  | .baseline => "baseline"

/-- One problem: one run of one level on one target. -/
structure Run where
  seq : Nat
  gen : Nat
  level : String
  target : Target
  param : Nat
  outcome : Outcome
  value : Nat
  ms : Nat
  deriving Inhabited, Repr

def Run.row (r : Run) : String :=
  s!"{r.seq}\t{r.gen}\t{r.level}\t{r.target.name}\t{r.param}\t{r.outcome.name}\t{r.value}\t{r.ms}"

def parseTarget : String → Option Target
  | "real" => some .real | "planted" => some .planted | _ => none

def parseOutcome : String → Option Outcome
  | "witness" => some .witness | "clean" => some .clean | "unreached" => some .unreached
  | "baseline" => some .baseline | _ => none

def parseRun (line : String) : Except String Run :=
  match line.splitOn "\t" with
  | [seq, gen, level, target, param, outcome, value, ms] =>
      match seq.toNat?, gen.toNat?, parseTarget target, param.toNat?, parseOutcome outcome,
          value.toNat?, ms.toNat? with
      | some seq, some gen, some t, some p, some o, some v, some ms =>
          if (parseKind level).isNone then .error s!"unknown level in record: {line}"
          else .ok { seq, gen, level, target := t, param := p, outcome := o, value := v, ms }
      | _, _, _, _, _, _, _ => .error s!"unreadable record: {line}"
  | _ => .error s!"a record needs eight fields: {line}"

/-- A level in the set, and the level it was mutated from. -/
structure Level where
  id : String
  parent : Option String := none
  deriving Inhabited, Repr, BEq

inductive Cls | unknown | frontier | blind | calibrated | noPlant
  deriving BEq, Repr, Inhabited

def Cls.name : Cls → String
  | .unknown => "unknown" | .frontier => "frontier" | .blind => "blind"
  | .calibrated => "calibrated" | .noPlant => "no plant"

def runsOf (runs : List Run) (id : String) (t : Target) : List Run :=
  (runs.filter fun r => r.level == id && r.target == t).mergeSort (fun a b => a.seq ≤ b.seq)

/-- The last `window` judged planted outcomes, oldest first. -/
def plantedWindow (runs : List Run) (id : String) : List Outcome :=
  let judged := (runsOf runs id .planted).filter fun r =>
    r.outcome == .witness || r.outcome == .clean
  (judged.drop (judged.length - window)).map (·.outcome)

def kills (w : List Outcome) : Nat := (w.filter (· == .witness)).length

def classify (runs : List Run) (id : String) : Cls :=
  let w := plantedWindow runs id
  let s := kills w
  if w.isEmpty then .unknown
  else if s == 0 && w.length ≥ window then .blind
  else if s == w.length && w.length ≥ window then .calibrated
  else .frontier

def regret (runs : List Run) (id : String) : Float :=
  let w := plantedWindow runs id
  if w.isEmpty then 0 else
    let s := kills w
    (if s ≥ 1 then 1.0 else 0.0) - s.toFloat / w.length.toFloat

def killRate (runs : List Run) (id : String) : Float :=
  let w := plantedWindow runs id
  if w.isEmpty then 0 else (kills w).toFloat / w.length.toFloat

def costMs (runs : List Run) (id : String) : Nat :=
  let rs := runs.filter (·.level == id)
  if rs.isEmpty then 2000 else max 1 ((rs.map (·.ms)).foldl (· + ·) 0 / rs.length)

def lastGen (runs : List Run) (id : String) (t : Target) : Option Nat :=
  ((runsOf runs id t).map (·.gen)).max?

def staleness (runs : List Run) (gen : Nat) (id : String) (t : Target) : Nat :=
  match lastGen runs id t with
  | some g => gen - g
  | none => gen + 1

def priority (runs : List Run) (gen : Nat) (id : String) (t : Target) : Float :=
  (regret runs id + stalenessWeight * (staleness runs gen id t).toFloat) /
    ((costMs runs id).toFloat / 1000.0)

def family (id : String) : Family := ((parseKind id).map Kind.family).getD .roundTrip

/-- The slowest of the real store's recent timed runs, the rate a slow run is measured against. -/
def baselineRate (runs : List Run) (id : String) : Option Nat :=
  if (match parseKind id with | some (.perf ..) | some (.ycsb ..) => false | _ => true) then none
  else
  let vs := ((runsOf runs id .real).filter fun r =>
    r.outcome == .baseline || r.outcome == .clean).map (·.value)
  let vs := vs.drop (vs.length - window)
  if vs.length < 3 then none else
    vs.min?

def slow (floor value : Nat) : Bool := value.toFloat < slowShare * floor.toFloat

/-- A real store witness is a defect: it is never averaged into a rate. -/
def escapes (runs : List Run) : List Run :=
  runs.filter fun r => r.target == .real && r.outcome == .witness

structure Plan where
  /-- Calibrated levels on the real store, inside the per change budget. -/
  change : List String
  /-- Levels on the planted target: unknown and frontier ones, to fill or slide the window. -/
  soak : List String
  /-- Timed levels whose real store baseline is not yet three runs long. -/
  baseline : List String
  blind : List String
  noPlant : List String
  mutations : List Level
  dropped : List String
  escapes : List Run
  deriving Inhabited

/-- One attribute moves to its neighbour. A crash band splits in two, so the half nearer the
defect can win; a timed level takes more repetitions, so noise matters less. -/
def mutate (id : String) : List String :=
  match parseKind id with
  | some (.crash r lo hi) =>
      let bigger := match [1, 8, 64, 400, 2000].find? (· > r) with
        | some r' => [(Kind.crash r' lo hi).id]
        | none => []
      let halves := if hi - lo < 2 then [] else
        let mid := (lo + hi) / 2
        [(Kind.crash r lo mid).id, (Kind.crash r (mid + 1) hi).id]
      halves ++ bigger
  | some (.perf r k) => if k < 9 then [(Kind.perf r (k + 2)).id] else []
  | some (.delta p r) => match [500, 2000, 8000].find? (· > r) with
      | some r' => [(Kind.delta p r').id]
      | none => []
  -- Smaller commits, so the planted round trip is a larger share of the run.
  | some (.ycsb r w b) => if b ≥ 100 then [(Kind.ycsb r w (b / 10)).id] else []
  | _ => []

/-- Greedy fill in triage order, then by priority. -/
def fill (runs : List Run) (gen : Nat) (t : Target) (budgetMs : Nat) (ids : List String) :
    List String :=
  let key id := (family id |>.rank, -(priority runs gen id t))
  let sorted := ids.mergeSort fun a b =>
    let (fa, pa) := key a
    let (fb, pb) := key b
    fa < fb || (fa == fb && (pa < pb || (pa == pb && a ≤ b)))
  let (picked, _) := sorted.foldl (init := ([], 0)) fun (acc, spent) id =>
    let c := costMs runs id
    if spent + c ≤ budgetMs then (acc ++ [id], spent + c) else (acc, spent)
  picked

def plan (levels : List Level) (runs : List Run) (gen changeMs soakMs : Nat) : Plan :=
  let ids := levels.map (·.id)
  let cls id := classify runs id
  let calibrated := ids.filter (cls · == .calibrated)
  let timed id := match parseKind id with | some (.perf ..) | some (.ycsb ..) => true | _ => false
  let needsBaseline := ids.filter fun id => timed id && (baselineRate runs id).isNone
  let soakable := ids.filter fun id =>
    (cls id == .unknown || cls id == .frontier) && !needsBaseline.contains id
  let dropped := levels.filterMap fun l =>
    match l.parent with
    | some p =>
        if (plantedWindow runs l.id).length ≥ window && killRate runs l.id ≤ killRate runs p
          && cls l.id != .calibrated then some l.id else none
    | none => none
  let frontierFull := ids.filter fun id =>
    cls id == .frontier && (plantedWindow runs id).length ≥ window && !dropped.contains id
  let mutations := (frontierFull.flatMap fun p =>
    (mutate p).map fun c => ({ id := c, parent := some p } : Level)).filter
      fun l => !ids.contains l.id
  { change := fill runs gen .real changeMs (calibrated.filter (!dropped.contains ·))
  , soak := fill runs gen .planted soakMs (soakable.filter (!dropped.contains ·))
  , baseline := needsBaseline
  , blind := ids.filter (cls · == .blind)
  , noPlant := []
  , mutations
  , dropped
  , escapes := escapes runs }

/-! ## Controls on the scheduler itself -/

def mk (seq : Nat) (level : String) (t : Target) (o : Outcome) (ms : Nat := 100)
    (value : Nat := 0) (gen : Nat := 0) : Run :=
  { seq, gen, level, target := t, param := 0, outcome := o, value, ms }

def series (level : String) (t : Target) (os : List Outcome) (ms : Nat := 100) : List Run :=
  (os.zip (List.range os.length)).map fun (o, i) => mk i level t o ms

/-- The run phase rate `ycsb_direct` prints, in operations a second. -/
def runRate (out : String) : Option Nat :=
  (out.splitOn "\n").findSome? fun line =>
    if line.startsWith "[RUN]" then
      match (line.splitOn "throughput=").getLast? with
      | some t => ((t.splitOn ".").head?.getD "").toNat?
      | none => none
    else none

def selfTest : IO UInt32 := do
  let a := "crash/rows=400/at=6..80"
  let b := "fence/writer"
  let all8 := List.replicate 8 Outcome.witness
  let none8 := List.replicate 8 Outcome.clean
  let checks : List (String × Bool) := [
    ("a delta level reads back as itself",
      ((parseKind "delta/stale/rows=2000").map Kind.id) == some "delta/stale/rows=2000"),
    ("a delta level with no such plant is refused",
      (parseKind "delta/torn/rows=2000").isNone),
    ("a ycsb level mutates to smaller commits",
      mutate "ycsb/rows=10000/workers=4/batch=1000" == ["ycsb/rows=10000/workers=4/batch=100"]),
    ("the run rate is read from the [RUN] line",
      runRate "[OVERALL] wall_s=1 throughput=5.0 ops/sec\n[RUN] wall_s=2.0 workers=4 throughput=45122.0 ops/sec" == some 45122),
    ("a level that never finds its planted defect is blind",
      classify (series a .planted none8) a == .blind),
    ("a level that finds it eight times in eight is calibrated",
      classify (series a .planted all8) a == .calibrated),
    ("one miss in eight leaves it on the frontier with regret",
      let rs := series a .planted (Outcome.clean :: List.replicate 7 .witness)
      classify rs a == .frontier && regret rs a > 0.1),
    ("a planted witness on the real store is an escape",
      (plan [{ id := a }] [mk 0 a .real .witness] 1 120000 600000).escapes.length == 1),
    ("a calibrated level whose cost is planted at 100 times leaves the change lane by name",
      let ok := plan [{ id := a }] (series a .planted all8 1500) 1 120000 0
      let slow := plan [{ id := a }] (series a .planted all8 150000) 1 120000 0
      ok.change.contains a && !slow.change.contains a),
    ("shuffled records give the same plan",
      let rs := series a .planted all8 ++ series b .planted (none8.take 3)
      let lv := [{ id := a }, { id := b }]
      (plan lv rs 1 120000 600000).change == (plan lv rs.reverse 1 120000 600000).change &&
        (plan lv rs 1 120000 600000).soak == (plan lv rs.reverse 1 120000 600000).soak),
    ("a record with a level the scheduler cannot decode is rejected",
      match parseRun "0\t0\tcrash/rows=x\treal\t0\tclean\t0\t5" with
      | .error _ => true | .ok _ => false),
    ("a zero budget gives an empty change lane",
      (plan [{ id := a }] (series a .planted all8) 1 0 0).change.isEmpty),
    ("performance is picked before round trip and round trip before security",
      let p := "perf/rows=2000/reps=1"
      let rs := series p .planted all8 ++ series a .planted all8 ++ series b .planted all8
        ++ (List.range 3).map fun i => mk (100 + i) p .real .baseline 100 200
      (plan [{ id := b }, { id := a }, { id := p }] rs 1 120000 0).change == [p, a, b]),
    ("a timed run inside the baseline's spread is not slow, and one well under it is",
      !slow 213 210 && slow 213 190),
    ("a frontier level with a full window proposes a split band and a bigger commit",
      let rs := series a .planted (Outcome.clean :: List.replicate 7 .witness)
      ((plan [{ id := a }] rs 1 120000 600000).mutations.map (·.id)) ==
        ["crash/rows=400/at=6..43", "crash/rows=400/at=44..80", "crash/rows=2000/at=6..80"]),
    ("a mutation whose kill rate does not rise is dropped",
      let c := "crash/rows=400/at=6..43"
      let rs := series a .planted (Outcome.clean :: List.replicate 7 .witness)
        ++ (series c .planted (List.replicate 2 Outcome.clean ++ List.replicate 6 .witness)).map
          fun r => { r with seq := r.seq + 50 }
      (plan [{ id := a }, { id := c, parent := some a }] rs 1 120000 600000).dropped == [c])
  ]
  let mut failed := 0
  for (name, ok) in checks do
    let tag := if ok then "PASS" else "FAIL"
    IO.println s!"  {tag}  {name}"
    if !ok then failed := failed + 1
  let verdict := if failed == 0 then "PASS" else "FAIL"
  IO.println s!"RESULT: {verdict} ({checks.length} controls, {failed} failed)"
  return if failed == 0 then 0 else 1

/-! ## Running problems -/

/-- A problem's parameter, drawn from the level's band by a seed that the record keeps. -/
def draw (seed : Nat) (lo hi : Nat) : Nat :=
  let x := (seed * 6364136223846793005 + 1442695040888963407) % 18446744073709551616
  lo + (x / 65536) % (hi - lo + 1)

def env (k : Kind) (t : Target) : Array (String × Option String) :=
  let off := #[("WEFT_PLANT_TORN_COMMIT", none), ("WEFT_PLANT_STALE_VIEW", none),
    ("WEFT_PLANT_SLOW_COMMIT", none), ("WEFT_PLANT_DELTA_CHAIN", none),
    ("WEFT_PLANT_DELTA_STALE", none), ("WEFT_PLANT_DELTA_FOLD", none)]
  match t with
  | .real => off
  | .planted => off.map fun (n, v) => if n == k.plant then (n, some "1") else (n, v)

/-- The rate `bench_vfs` prints for one commit each, through FoundationDB. -/
def commitRate (out : String) : Option Nat :=
  (out.splitOn "\n").findSome? fun line =>
    if line.startsWith "insert, one commit each" then
      match (line.splitOn " ").filter (· ≠ "") with
      | _ :: _ :: _ :: _ :: _ :: fdb :: _ => fdb.toNat?
      | _ => none
    else none

structure Ctx where
  bin : String
  work : String
  gen : Nat

def exec (ctx : Ctx) (cmd : String) (args : Array String) (k : Kind) (t : Target) :
    IO IO.Process.Output :=
  IO.Process.output
    { cmd := cmd, args := args, cwd := some ctx.work, env := env k t }

/-- Run one problem. A harness failure is an error, not a finding. -/
def runOne (ctx : Ctx) (runs : List Run) (seq : Nat) (id : String) (t : Target) :
    IO (Run × String) := do
  let some k := parseKind id | throw (IO.userError s!"unknown level {id}")
  let t0 ← IO.monoMsNow
  let db := s!"frontier-{ctx.gen}-{seq}.db"
  let (param, outcome, value, text) ← match k with
    | .crash rows lo hi =>
        let p := draw (ctx.gen * 100003 + seq) lo hi
        let o ← exec ctx s!"{ctx.bin}/prove_crash" #[db, toString rows, "at", toString p] k t
        match o.exitCode with
        | 0 => pure (p, Outcome.clean, 0, "")
        | 1 => pure (p, Outcome.witness, 0, o.stdout ++ o.stderr)
        | 2 => pure (p, Outcome.unreached, 0, "")
        | c => throw (IO.userError s!"prove_crash exited {c}: {o.stderr}")
    | .fence m =>
        let o ← exec ctx s!"{ctx.bin}/prove_fence_read" #[m, db] k t
        if o.exitCode == 0 then pure (0, Outcome.clean, 0, "")
        else pure (0, Outcome.witness, 0, o.stdout ++ o.stderr)
    | .perf rows reps =>
        let mut rates : Array Nat := #[]
        for _ in [0:reps] do
          let o ← exec ctx s!"{ctx.bin}/bench_vfs" #[toString rows] k t
          match commitRate o.stdout with
          | some r => rates := rates.push r
          | none => throw (IO.userError s!"bench_vfs printed no commit rate: {o.stderr}")
        let sorted := rates.toList.mergeSort (· ≤ ·)
        let med := sorted[sorted.length / 2]?.getD 0
        match baselineRate runs id with
        | none => pure (0, Outcome.baseline, med, "")
        | some base =>
            if slow base med then
              pure (0, Outcome.witness, med, s!"{med} commits/s against a floor of {base}")
            else pure (0, Outcome.clean, med, "")
    | .delta _ rows =>
        let o ← exec ctx s!"{ctx.bin}/ycsb_direct"
          #[toString rows, toString rows, "1", "100", "--sorted-load", "--nomutex", "--verify",
            "--prefix", s!"frontier-{ctx.gen}-{seq}"] k t
        let out := o.stdout ++ o.stderr
        if o.exitCode == 0 && (out.splitOn "MISMATCH").length == 1 then
          pure (0, Outcome.clean, 0, "")
        else pure (0, Outcome.witness, 0, out)
    | .ycsb rows workers batch =>
        let o ← exec ctx s!"{ctx.bin}/ycsb_direct"
          #[toString rows, toString rows, toString workers,
            toString batch, "--sorted-load", "--nomutex", "--prefix", s!"frontier-{ctx.gen}-{seq}"] k t
        let some rate := runRate o.stdout
          | throw (IO.userError s!"ycsb_direct printed no run rate: {o.stderr}")
        match baselineRate runs id with
        | none => pure (0, Outcome.baseline, rate, "")
        | some base =>
            if slow base rate then
              pure (0, Outcome.witness, rate, s!"{rate} ops/s against a floor of {base}")
            else pure (0, Outcome.clean, rate, "")
  -- A run leaves its databases behind, and a memory-engine cluster that fills up slows every
  -- later timed run: four generations of them read as a slow store on the real target.
  match k with
  | .delta .. | .ycsb .. =>
      let pre := s!"weft/db/frontier-{ctx.gen}-{seq}-"
      let stop := s!"weft/db/frontier-{ctx.gen}-{seq}."
      let cmd := s!"writemode on; clearrange {pre} {stop}"
      let spawn : IO.Process.SpawnArgs :=
        { cmd := "fdbcli", args := #["--exec", cmd, "--timeout", "30"] }
      let o ← IO.Process.output spawn
      if o.exitCode != 0 then throw (IO.userError s!"fdbcli could not clear {pre}: {o.stderr}")
  | _ => pure ()
  let t1 ← IO.monoMsNow
  return ({ seq, gen := ctx.gen, level := id, target := t, param, outcome, value,
            ms := t1 - t0 }, text)

def readLines (path : System.FilePath) : IO (List String) := do
  if ← path.pathExists then
    return ((← IO.FS.readFile path).splitOn "\n").filter (· ≠ "")
  else return []

def appendLine (path : System.FilePath) (line : String) : IO Unit := do
  let h ← IO.FS.Handle.mk path .append
  h.putStrLn line
  h.flush

def seedLevels : List Level := [
  { id := "perf/rows=2000/reps=1" },
  { id := "crash/rows=8/at=1..30" },
  { id := "crash/rows=400/at=6..21" },
  { id := "crash/rows=2000/at=6..80" },
  { id := "fence/writer" },
  { id := "fence/reader" },
  { id := "delta/chain/rows=2000" },
  { id := "delta/stale/rows=2000" },
  { id := "delta/fold/rows=2000" },
  { id := "ycsb/rows=10000/workers=4/batch=1000" }]

def loadLevels (dir : System.FilePath) : IO (List Level) := do
  let path := dir / "levels.tsv"
  let lines ← readLines path
  if lines.isEmpty then
    for l in seedLevels do appendLine path s!"{l.id}\t-"
    return seedLevels
  lines.mapM fun line => match line.splitOn "\t" with
    | [id, "-"] => if (parseKind id).isSome then pure { id } else
        throw (IO.userError s!"unknown level {id}")
    | [id, p] => if (parseKind id).isSome then pure { id, parent := some p } else
        throw (IO.userError s!"unknown level {id}")
    | _ => throw (IO.userError s!"unreadable level row: {line}")

def loadRuns (dir : System.FilePath) : IO (List Run) := do
  (← readLines (dir / "runs.tsv")).mapM fun line => match parseRun line with
    | .ok r => pure r
    | .error e => throw (IO.userError e)

def report (levels : List Level) (runs : List Run) (p : Plan) : IO Unit := do
  for l in levels do
    let w := plantedWindow runs l.id
    let base := match baselineRate runs l.id with | some b => s!" floor {b}/s" | none => ""
    IO.println s!"  {(family l.id).name}  {l.id}  {(classify runs l.id).name}  planted {kills w}/{w.length}  regret {regret runs l.id}{base}"
  IO.println s!"CHANGE LANE (real store): {p.change}"
  IO.println s!"SOAK LANE (planted): {p.soak}"
  IO.println s!"BASELINE (real store, timed): {p.baseline}"
  IO.println s!"BLIND (the attack cannot see its defect): {p.blind}"
  IO.println s!"MUTATIONS: {p.mutations.map (·.id)}"
  IO.println s!"DROPPED (kill rate did not rise): {p.dropped}"
  IO.println s!"ESCAPES (real store witnesses): {p.escapes.map (·.row)}"

def usage : String :=
  "usage: frontier --self-test\n" ++
  "       frontier plan <records-dir>\n" ++
  "       frontier run <build-dir> <records-dir> [change-seconds] [soak-seconds]\n\n" ++
  "Exit: 0 no escape, 1 a witness on the real store or a failed control, 2 usage."

def runCmd (bin dir : String) (changeS soakS : Nat) : IO UInt32 := do
  let dir : System.FilePath := dir
  IO.FS.createDirAll (dir / "work")
  let levels ← loadLevels dir
  let runs ← loadRuns dir
  let gen := ((runs.map (·.gen)).max?.map (· + 1)).getD 0
  let ctx : Ctx := { bin, work := (dir / "work").toString, gen }
  let p := plan levels runs gen (changeS * 1000) (soakS * 1000)
  for m in p.mutations do
    let parent := m.parent.getD "-"
    appendLine (dir / "levels.tsv") s!"{m.id}\t{parent}"
  let levels := levels ++ p.mutations
  let mut runs := runs
  let mut seq := ((runs.map (·.seq)).max?.map (· + 1)).getD 0
  let work := p.baseline.map (·, Target.real) ++ p.change.map (·, Target.real)
    ++ (p.soak ++ p.mutations.map (·.id)).map (·, Target.planted)
  for (id, t) in work do
    let reps := if t == .planted then max 1 (window - (plantedWindow runs id).length)
      else if p.baseline.contains id then 3 else 1
    for _ in [0:reps] do
      let (r, text) ← runOne ctx runs seq id t
      appendLine (dir / "runs.tsv") r.row
      if text ≠ "" then
        appendLine (dir / "witnesses.tsv") s!"{r.seq}\t{text.replace "\n" " | "}"
      IO.println s!"{r.row}"
      runs := runs ++ [r]
      seq := seq + 1
      if r.target == .real && r.outcome == .witness then
        IO.println s!"ESCAPE: {id} found a defect on the real store; the lane stops here"
        return 1
  report levels runs (plan levels runs (gen + 1) (changeS * 1000) (soakS * 1000))
  return 0

def main (argv : List String) : IO UInt32 := do
  match argv with
  | ["--self-test"] => selfTest
  | ["plan", dir] =>
      let levels ← loadLevels dir
      let runs ← loadRuns dir
      let gen := ((runs.map (·.gen)).max?.map (· + 1)).getD 0
      let p := plan levels runs gen 120000 600000
      report levels runs p
      return if p.escapes.isEmpty then 0 else 1
  | ["run", bin, dir] => runCmd bin dir 120 600
  | ["run", bin, dir, c, s] =>
      match c.toNat?, s.toNat? with
      | some c, some s => runCmd bin dir c s
      | _, _ => IO.eprintln usage; return 2
  | _ => IO.eprintln usage; return 2

end Weft.Frontier

def main (args : List String) : IO UInt32 := Weft.Frontier.main args
