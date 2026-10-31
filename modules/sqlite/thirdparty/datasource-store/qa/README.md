# Breaking the store on purpose

`prove_crash.c` kills a writer to look for a torn commit. These do the same thing one level
up, to the cluster the pages live in: stop a zone, stop a quorum, lose the disks entirely.

Vendored here rather than referenced, because a procedure that lives on one laptop is a
procedure that is gone when the laptop is. `qa/` needs nothing outside this checkout except
FoundationDB itself.

## What each one asserts

`consensus.sh` — three processes on distinct zones, three coordinators, `double` redundancy.

1. The cluster's own claim: `Coordinators - 3`, `Fault Tolerance - 1 zones`.
2. 200 keys written and read back.
3. **One zone stopped with `kill -9`.** The cluster must still answer, and all 200 keys must
   still read. This is the stage that means something: stage 1 is a number the cluster says
   about itself, and a cluster that reports tolerance and then loses data passes it.
4. The zone restarts, and tolerance returns to `1 zones`.
5. Every process stopped and restarted. The keys must survive.
6. **Two of three stopped.** Now the cluster must *refuse* to work — no quorum means it may
   not order writes, and a minority that answers can split-brain. Here a working cluster is
   the failure.
7. Quorum returns and so does the data.

`dr.sh` — backup, total loss, restore. It wipes the data directories and **proves the new
cluster holds zero keys before restoring**. Without that check the restore proves nothing.

## Running them

    qa/install.sh
    systemctl --user restart --no-block fdb-consensus-qa
    journalctl --user -u fdb-consensus-qa -f

`restart`, not `start`: `RemainAfterExit=yes` makes `start` a silent no-op after a green run.

Oneshot units rather than `nohup`, so a run survives the shell that began it, the exit
status stays readable afterwards, and the output lands in the journal with timestamps.
Both are time-bounded — a run that takes longer than its budget is stuck, not slow.

## The negative control

    systemctl --user restart --no-block fdb-consensus-qa-negative   # must FAIL

`single` redundancy keeps one copy of every key, so stopping a zone must lose data. If this
unit succeeds, the QA above proves nothing.

It has already earned its place. The first version of stage 3 wrote **one** key and passed
under `single` redundancy, because one key sits on one storage team and the stopped zone did
not hold it. The test was measuring placement luck. Hence 200 keys.

## Two traps these found

**A backup URL that is not restorable, and does not say so.** `fdbbackup start -d
file:///backup/` writes to a container underneath that path, not to it. `describe` on the
path you gave answers `Restorable: false`, `SnapshotBytes: 0`, and exits zero. A runbook
recording the `-d` path looks like it holds backups. A named subfolder does not fix it — the
container nests one level deeper. So: find the container with `fdbbackup list -b <base>`,
which also works for `blobstore://`, and check `Restorable: true` before trusting it.

**`fdbcli` quotes keys with a backtick**, not an apostrophe: `` `weft/qa/k0001' is `v1' ``.
A counter matching a leading apostrophe reports zero keys, which is indistinguishable from
total data loss.

## Configuration

Binaries come from `PATH` and are overridable: `SERVER`, `CLI`, `BACKUP`, `RESTORE`,
`AGENT`. `KEYS` sets the key count, `ROOT` the run directory, `REDUNDANCY` the mode.
`DEST` sets the backup URL — swap `file://` for
`blobstore://<key>:<secret>@<host>/<name>?bucket=<bucket>` and the procedure is unchanged.

`dr.sh` needs `fdbbackup`; the release ships it, and `fdbrestore` and `backup_agent` are the
same binary under different names.

## What one machine cannot test

These stop processes. They do not stop machines and they do not cut networks, so they
exercise consensus and recovery but never a partition. That needs two hosts.

## Two faults in these tests, found by running them

Recorded here rather than fixed, because both need a design decision.

**The negative control does not demonstrate what it claims.** Under
`single` redundancy the zone kill left **all 200 keys readable**. The
run still "failed", but on a tolerance status line at stage 4, not on
data loss -- so the suite reports the control failing as required while
never exercising its mechanism.

Key count is not the variable. **Shard count is.** 200 tiny keys is one
shard on one storage team, so killing a named zone is a one-in-three
chance of hitting it. Going from 1 key to 200 changed nothing about
that; it takes enough data to span several shards, or a kill aimed at
the team that actually holds them.

Until then, stage 3 of `consensus.sh` is uncontrolled: it is not known
whether it would catch a store that really did lose data.

**No restore has been proven.** `dr.sh` reaches `Restorable: true` and
then fails at stage 4, unable to reconfigure after wiping the data
directories, so the restore never runs. The backup half is exercised
and the recovery half is not, which is the wrong half to have
confidence in.

## What these still cannot test

The machine-scale rungs -- stop a machine, lose quorum, recover -- were
run against three Fly hosts and passed. Those hosts are gone, and the
procedure was not vendored here.

A **partition** remains untested anywhere. Every rung above stops
something cleanly. A partition leaves both sides alive and unable to
see each other, which is the only case where split-brain is possible,
and stopping a machine never produces it.
