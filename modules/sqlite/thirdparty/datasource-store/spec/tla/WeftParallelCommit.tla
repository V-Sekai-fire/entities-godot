-------------------------- MODULE WeftParallelCommit --------------------------
EXTENDS TLC, Integers, FiniteSets, Sequences
CONSTANTS DBS, RECOVERERS, MAX_ATTEMPTS
ASSUME Cardinality(DBS) > 0
ASSUME Cardinality(RECOVERERS) > 0
ASSUME MAX_ATTEMPTS > 0

(*************************************************************************)
(* This module is `thirdparty/tla/ParallelCommits.tla` with this layout's *)
(* vocabulary substituted for CockroachDB's. It is produced by            *)
(* `spec/tla/derive.sh`, and `./ci.sh tla` fails if the committed file is *)
(* not what that script emits, so the two cannot drift apart.             *)
(*                                                                       *)
(* The substitution, which is the mapping `spec/ParallelCommit.lean`      *)
(* tabulates:                                                            *)
(*                                                                       *)
(*   KEYS          -> DBS         a participant is a database, not a key *)
(*   PREVENTERS    -> RECOVERERS  recovery is what the process runs      *)
(*   intent_writes -> staged      pages under a txid above the head      *)
(*   tscache       -> fence       what refuses a stale writer            *)
(*   record        -> txn         weft/txn/<txnid>                       *)
(*   ts            -> txid        a commit's identity, not a clock       *)
(*   resolved      -> reclaimed   the staged pages are gone              *)
(*                                                                       *)
(* Nothing else is changed. Every action, label, invariant and temporal  *)
(* property below is the authority's, so a disagreement between the two  *)
(* protocols would appear as a diff rather than as a reading exercise.   *)
(*                                                                       *)
(* What this does not establish: that `fdb_vfs.c` implements the module. *)
(* A renaming is exact about what it renames and silent about everything *)
(* below it. `prove_parallel_commit` tests the C at eight crash points   *)
(* and `spec/ParallelCommit.lean` argues the layout properties this      *)
(* module cannot state, having no pages.                                 *)
(*                                                                       *)
(* The authority's own header, 39 lines on CockroachDB's implementation  *)
(* files and its safety and liveness claims, is left where it is.        *)
(*************************************************************************)
(*--algorithm parallelcommits
variables
  txn = [status |-> "pending", epoch |-> 0, txid |-> 0];
  staged = [k \in DBS |-> [epoch |-> 0, txid |-> 0, reclaimed |-> FALSE]];
  fence = [k \in DBS |-> 0];
  commit_ack = FALSE;

define
  \* Simulates a QueryIntent request, taking care to model the exact
  \* condition in which the request considers an intent to be found.
  QueryIntent(key, query_epoch, query_ts) ==
    LET
      intent == staged[key]
    IN
      /\ intent.epoch = query_epoch
      /\ intent.txid <= query_ts
      \* The loss of information from intent resolution that is reflected
      \* here has a few unfortunate effects:
      \* 1. it is ambiguous whether a QueryIntent issued in parallel with
      \*    a parallel commit is due to a missing intent or intent resolution
      \*    after transaction finalization. In order to reduce this ambiguity,
      \*    we're forced to query the transaction txn after we detect this
      \*    condition (#37866).
      \* 2. a transaction recovery process that detects a missing intent cannot
      \*    definitively conclude that the transaction being recovered was not
      \*    committed without checking the transaction txn first (#37784).
      \*
      \* We could address part of this by storing transaction IDs in reclaimed
      \* values and allowing QueryIntent to correctly identify reclaimed values
      \* that correspond to the desired intent (i.e. removing this condition).
      \* However, there will still be complications with value GC.
      /\ intent.reclaimed = FALSE

  RecordStatuses  == {"pending", "staging", "committed", "aborted"}
  RecordStaging   == txn.status = "staging"
  RecordCommitted == txn.status = "committed"
  RecordAborted   == txn.status = "aborted"
  RecordFinalized == RecordCommitted \/ RecordAborted

  ImplicitlyCommitted ==
    /\ RecordStaging
    /\ \A k \in DBS:
      /\ staged[k].epoch = txn.epoch
      /\ staged[k].txid   <= txn.txid
  ExplicitlyCommitted == RecordCommitted
  Committed           == ImplicitlyCommitted \/ ExplicitlyCommitted

  TypeInvariants ==
    /\ txn \in [status: RecordStatuses, epoch: 0..MAX_ATTEMPTS, txid: 0..MAX_ATTEMPTS]
    /\ DOMAIN staged = DBS
      /\ \A k \in DBS:
        staged[k] \in [
          epoch:    0..MAX_ATTEMPTS, 
          txid:       0..MAX_ATTEMPTS, 
          reclaimed: BOOLEAN
        ]
    /\ DOMAIN fence = DBS
      /\ \A k \in DBS: fence[k] \in 0..MAX_ATTEMPTS

  TemporalTxnRecordProperties ==
    \* The txn txn always ends with either a COMMITTED or ABORTED status.
    /\ <>[]RecordFinalized
    \* Once the txn txn moves to a finalized status, it stays there.
    /\ [](RecordCommitted => []RecordCommitted)
    /\ [](RecordAborted   => []RecordAborted)
    \* Once the txn is committed, it remains committed.
    /\ [](Committed => []Committed)
    \* The txn txn's epoch must always grow.
    /\ [][txn'.epoch >= txn.epoch]_txn
    \* The txn txn's timestamp must always grow.
    /\ [][txn'.txid >= txn.txid]_txn

  TemporalIntentProperties ==
    \* Intent writes' epochs must always grow.
    /\ [][\A k \in DBS: staged'[k].epoch >= staged[k].epoch]_staged
    \* Intent writes' timestamps must always grow.
    /\ [][\A k \in DBS: staged'[k].txid >= staged[k].txid]_staged
    \* All intents are eventually reclaimed and stay reclaimed.
    /\ <>[](\A k \in DBS: staged[k].reclaimed)

  TemporalTSCacheProperties ==
    \* The timestamp cache always advances.
    /\ [][\A k \in DBS: fence'[k] >= fence[k]]_fence

  \* If the transaction ever becomes implicitly committed, it should
  \* eventually become explicitly committed.
  ImplicitCommitLeadsToExplicitCommit == ImplicitlyCommitted ~> ExplicitlyCommitted

  \* If the client is acked, the transaction must be committed.
  AckImpliesCommit == commit_ack => Committed

  \* If the client is acked, the txn should eventually be explicitly committed.
  AckLeadsToExplicitCommit == commit_ack ~> ExplicitlyCommitted
end define;

\* Give up after MAX_ATTEMPTS attempts. This bounds the state space for the
\* spec and ensures that it terminates. A real transaction coordinator will not
\* give up after a certain number of attempts. However, real transactions will
\* probabilistically terminate because concurrent transactions will not attempt
\* to recover a parallel commit (i.e. serve as a "recoverer" process) until the
\* parallel committing transaction's heartbeat expires.
macro maybe_abandon_retry()
begin
  if attempt > MAX_ATTEMPTS then
    goto EndCommitter;
  end if;
end macro;

process committer = "committer"
variables
  \* -- constants --
  \* Represents keys that are written before the final Batch.
  pipelined_keys \in SUBSET DBS;
  \* Represents keys that are written in the final Batch.
  parallel_keys = DBS \ pipelined_keys;

  \* -- variables --
  attempt   = 1;
  txn_epoch = 0;
  txn_ts    = 0;
  to_write  = {};
  to_check  = {};
  have_staging_record = FALSE;
begin
  \* Begin a new transaction epoch.
  BeginTxnEpoch:
    txn_epoch := txn_epoch + 1;
    txn_ts := txn_ts + 1;
    to_write := pipelined_keys;
    maybe_abandon_retry();

  \* Attempt to perform all pipelined intent writes. These are writes that
  \* occur before the final Batch containing the EndTransaction request.
  \* These writes are ordered, but it's more hassle than it's worth to model
  \* them that way.
  PipelineWrites:
    while to_write /= {} do
      with key \in to_write do
        to_write := to_write \ {key};
        if staged[key].reclaimed then
          \* Can't write over reclaimed write. In reality, this would result
          \* in laying down an (uncommitable) intent at a higher timestamp
          \* and returning a WriteTooOld error. For the sake of this model,
          \* we don't write anything. The pre-commit QueryIntent sent to
          \* this key during the parallel commit will fail.
        elsif fence[key] >= txn_ts then
          \* Write prevented. This shouldn't happen.
          assert FALSE;
        else
          either
            \* Async consensus successful.
            staged[key] := [
              epoch    |-> txn_epoch,
              txid       |-> txn_ts,
              reclaimed |-> FALSE
            ];
          or
            \* Async consensus unsuccessful. Should be
            \* discovered by a pre-commit QueryIntent.
            skip;
          end either;
        end if;
      end with;
    end while;

  \* Attempt to perform all final-batch intent writes, query all pipelined
  \* writes, and stage the transaction txn in parallel.
  StageWritesAndRecord:
    to_write := parallel_keys;
    to_check := pipelined_keys;
    have_staging_record := FALSE;
    maybe_abandon_retry();

    StageWritesAndRecordLoop:
      while to_check /= {} \/ to_write /= {} \/ ~have_staging_record do
        either
          await to_check /= {};
          QueryPipelinedWrite:
            with key \in to_check do
              if QueryIntent(key, txn_epoch, txn_ts) then
                \* Intent found. Pipelined write succeeded.
                to_check := to_check \ {key}
              else
                \* Intent missing. Pipelined write failed.
                \* Check the transaction txn to see whether it has already
                \* been finalized using a QueryTxn request. This would indicate
                \* that the missing intent is due to intent resolution.
                if txn.status \in {"pending", "staging"} then
                  \* Unambiguously not finalized. Perform a transaction restart
                  \* at new epoch.
                  attempt := attempt + 1;
                  goto BeginTxnEpoch;
                elsif txn.status = "aborted" then
                  \* Unambiguously aborted here, but in the implementation this is
                  \* ambiguous because "aborted" may indicate an aborted txn or
                  \* a committed txn that was GCed.
                  goto EndCommitter;
                elsif txn.status = "committed" then
                  \* Unambiguously committed.
                  goto AckClient;
                end if;
              end if;
            end with;
        or
          await to_write /= {};
          ParallelWrite:
            with key \in to_write,
                 cur_intent = staged[key] do
              to_write := to_write \ {key};
              if cur_intent.epoch = txn_epoch then
                \* Write already succeeded before refresh. Writes should be idempotent,
                \* so there's nothing to do. In practice, this is not strictly true (e.g.
                \* after intents are reclaimed), which is why we currently reject retry
                \* attempts that would rely on idempotence with MixedSuccessErrors.
              elsif fence[key] >= txn_ts \/ cur_intent.reclaimed then
                \* Write prevented.
                either
                  \* Successful refresh. Try again at same epoch.
                  \* No need to re-write existing intents at new timestamp.
                  txn_ts := txn_ts + 1;
                  attempt := attempt + 1;
                  goto StageWritesAndRecord;
                or
                  \* Failed refresh. Try again at new epoch.
                  \* Must re-write all intents at new epoch.
                  attempt := attempt + 1;
                  goto BeginTxnEpoch;
                end either;
              else
                \* Write successful.
                staged[key] := [
                  epoch    |-> txn_epoch,
                  txid       |-> txn_ts,
                  reclaimed |-> FALSE
                ];
              end if;
            end with;
        or
          await ~have_staging_record;
          StageRecord:
            have_staging_record := TRUE;
            if txn.status = "pending" then
              \* Move to staging status.
              txn := [status |-> "staging", epoch |-> txn_epoch, txid |-> txn_ts];
            elsif txn.status = "staging" then
              \* Bump txn timestamp and maybe epoch.
              assert txn.epoch <= txn_epoch /\ txn.txid < txn_ts;
              txn := [status |-> "staging", epoch |-> txn_epoch, txid |-> txn_ts];
            elsif txn.status = "aborted" then
              \* Aborted before STAGING transaction txn.
              goto EndCommitter;
            elsif txn.status = "committed" then
              \* Should not already be committed.
              assert FALSE;
            end if;
        end either
      end while;

  \* Ack the client now that all writes have succeeded
  \* and the transaction is implicitly committed.
  AckClient:
    assert Committed;
    commit_ack := TRUE;

  \* Now that the transaction is implicitly committed,
  \* asynchronously make the commit explicit.
  AsyncExplicitlyCommitted:
    if txn.status = "staging" then
      assert ImplicitlyCommitted;
      \* Make implicit commit explicit.
      txn.status := "committed";
    elsif txn.status = "committed" then
      \* Already committed by a recovery process.
      skip;
    else
      \* Should not be pending or aborted at this point.
      assert FALSE;
    end if;

  \* Now that the commit is explicit, asynchronously resolve
  \* all intents. Re-use the to_write variable for convenience.
  to_write := DBS;
  AsyncResolveIntents:
    while to_write /= {} do
      with key \in to_write do
        if ~staged[key].reclaimed then
          staged[key].reclaimed := TRUE;
        end if;
        to_write := to_write \ {key};
      end with;
    end while;

  EndCommitter:
    skip;

end process;

fair process recoverer \in RECOVERERS
variable
  prevent_epoch = 0;
  prevent_ts    = 0;
  found_writes  = {};
  to_resolve    = DBS;
begin
  PreventLoop:
    found_writes := {};

    \* Push the transaction txn to determine its
    \* status, epoch, and timestamp.
    PushRecord:
      if txn.status = "pending" then
        \* Transaction not yet staged, abort.
        txn.status := "aborted";
        goto ResolveIntents;
      elsif txn.status = "staging" then
        \* Transaction staging, kick off recovery process.
        prevent_epoch := txn.epoch;
        prevent_ts := txn.txid;
      elsif txn.status \in {"committed", "aborted"} then
        \* Already finalized, nothing to do.
        goto ResolveIntents;
      end if;

    \* Attempt to prevent any of its in-flight intent writes.
    PreventWrites:
      while found_writes /= DBS do
        with key \in DBS \ found_writes do
          if QueryIntent(key, prevent_epoch, prevent_ts) then
            \* Intent found. Could not prevent.
            found_writes := found_writes \union {key}
          else
            \* Intent missing. Prevent.
            if fence[key] < prevent_ts then
              fence[key] := prevent_ts;
            end if;
            goto RecoverRecord;
          end if;
        end with;
      end while;

    \* Recover based on whether any of its in-flight writes
    \* were prevented. If not, the transaction is already
    \* implicitly committed.
    RecoverRecord:
      with prevented = found_writes /= DBS do
        if prevented then
          with legal_change = txn.epoch >= prevent_epoch
                           /\ txn.txid    >  prevent_ts do
            if txn.status = "aborted" then
              \* Already aborted, nothing to do.
              skip;
            elsif txn.status = "committed" then
              \* Already committed, nothing to do.
              skip;
            elsif txn.status = "pending" then
              \* Should not be pending at this point.
              assert FALSE;
            elsif txn.status = "staging" then
              if legal_change then
                \* Try to prevent at higher epoch.
                goto PreventLoop;
              else
                \* Can abort as result of recovery.
                txn.status := "aborted";
              end if;
            end if;
          end with;
        else
          \* The transaction was implicitly committed.
          if txn.status \in {"pending", "aborted"} then
            \* Should not be pending or aborted at this point.
            assert FALSE;
          elsif txn.status \in {"staging", "committed"} then
            \* The epoch and timestamp should be what we expect.
            assert txn.epoch = prevent_epoch;
            assert txn.txid    = prevent_ts;

            \* Can commit as result of recovery.
            if txn.status = "staging" then
              assert ImplicitlyCommitted;
              txn.status := "committed";
            end if;
          end if;
        end if;
      end with;

  \* Now that the transaction is finalized, synchronously resolve
  \* all of its intents. After this point, the conflicting transaction
  \* can return to doing whatever it was doing.
  ResolveIntents:
    while to_resolve /= {} do
      with key \in to_resolve do
        if ~staged[key].reclaimed then
          staged[key].reclaimed := TRUE;
        end if;
        to_resolve := to_resolve \ {key};
      end with;
    end while;

end process;
end algorithm;*)
\* BEGIN TRANSLATION - the hash of the PCal code: PCal-7847e3ffca2156d2f95a169911409dbb
VARIABLES txn, staged, fence, commit_ack, pc

(* define statement *)
QueryIntent(key, query_epoch, query_ts) ==
  LET
    intent == staged[key]
  IN
    /\ intent.epoch = query_epoch
    /\ intent.txid <= query_ts















    /\ intent.reclaimed = FALSE

RecordStatuses  == {"pending", "staging", "committed", "aborted"}
RecordStaging   == txn.status = "staging"
RecordCommitted == txn.status = "committed"
RecordAborted   == txn.status = "aborted"
RecordFinalized == RecordCommitted \/ RecordAborted

ImplicitlyCommitted ==
  /\ RecordStaging
  /\ \A k \in DBS:
    /\ staged[k].epoch = txn.epoch
    /\ staged[k].txid   <= txn.txid
ExplicitlyCommitted == RecordCommitted
Committed           == ImplicitlyCommitted \/ ExplicitlyCommitted

TypeInvariants ==
  /\ txn \in [status: RecordStatuses, epoch: 0..MAX_ATTEMPTS, txid: 0..MAX_ATTEMPTS]
  /\ DOMAIN staged = DBS
    /\ \A k \in DBS:
      staged[k] \in [
        epoch:    0..MAX_ATTEMPTS,
        txid:       0..MAX_ATTEMPTS,
        reclaimed: BOOLEAN
      ]
  /\ DOMAIN fence = DBS
    /\ \A k \in DBS: fence[k] \in 0..MAX_ATTEMPTS

TemporalTxnRecordProperties ==

  /\ <>[]RecordFinalized

  /\ [](RecordCommitted => []RecordCommitted)
  /\ [](RecordAborted   => []RecordAborted)

  /\ [](Committed => []Committed)

  /\ [][txn'.epoch >= txn.epoch]_txn

  /\ [][txn'.txid >= txn.txid]_txn

TemporalIntentProperties ==

  /\ [][\A k \in DBS: staged'[k].epoch >= staged[k].epoch]_staged

  /\ [][\A k \in DBS: staged'[k].txid >= staged[k].txid]_staged

  /\ <>[](\A k \in DBS: staged[k].reclaimed)

TemporalTSCacheProperties ==

  /\ [][\A k \in DBS: fence'[k] >= fence[k]]_fence



ImplicitCommitLeadsToExplicitCommit == ImplicitlyCommitted ~> ExplicitlyCommitted


AckImpliesCommit == commit_ack => Committed


AckLeadsToExplicitCommit == commit_ack ~> ExplicitlyCommitted

VARIABLES pipelined_keys, parallel_keys, attempt, txn_epoch, txn_ts, to_write, 
          to_check, have_staging_record, prevent_epoch, prevent_ts, 
          found_writes, to_resolve

vars == << txn, staged, fence, commit_ack, pc, pipelined_keys, 
           parallel_keys, attempt, txn_epoch, txn_ts, to_write, to_check, 
           have_staging_record, prevent_epoch, prevent_ts, found_writes, 
           to_resolve >>

ProcSet == {"committer"} \cup (RECOVERERS)

Init == (* Global variables *)
        /\ txn = [status |-> "pending", epoch |-> 0, txid |-> 0]
        /\ staged = [k \in DBS |-> [epoch |-> 0, txid |-> 0, reclaimed |-> FALSE]]
        /\ fence = [k \in DBS |-> 0]
        /\ commit_ack = FALSE
        (* Process committer *)
        /\ pipelined_keys \in SUBSET DBS
        /\ parallel_keys = DBS \ pipelined_keys
        /\ attempt = 1
        /\ txn_epoch = 0
        /\ txn_ts = 0
        /\ to_write = {}
        /\ to_check = {}
        /\ have_staging_record = FALSE
        (* Process recoverer *)
        /\ prevent_epoch = [self \in RECOVERERS |-> 0]
        /\ prevent_ts = [self \in RECOVERERS |-> 0]
        /\ found_writes = [self \in RECOVERERS |-> {}]
        /\ to_resolve = [self \in RECOVERERS |-> DBS]
        /\ pc = [self \in ProcSet |-> CASE self = "committer" -> "BeginTxnEpoch"
                                        [] self \in RECOVERERS -> "PreventLoop"]

BeginTxnEpoch == /\ pc["committer"] = "BeginTxnEpoch"
                 /\ txn_epoch' = txn_epoch + 1
                 /\ txn_ts' = txn_ts + 1
                 /\ to_write' = pipelined_keys
                 /\ IF attempt > MAX_ATTEMPTS
                       THEN /\ pc' = [pc EXCEPT !["committer"] = "EndCommitter"]
                       ELSE /\ pc' = [pc EXCEPT !["committer"] = "PipelineWrites"]
                 /\ UNCHANGED << txn, staged, fence, commit_ack, 
                                 pipelined_keys, parallel_keys, attempt, 
                                 to_check, have_staging_record, prevent_epoch, 
                                 prevent_ts, found_writes, to_resolve >>

PipelineWrites == /\ pc["committer"] = "PipelineWrites"
                  /\ IF to_write /= {}
                        THEN /\ \E key \in to_write:
                                  /\ to_write' = to_write \ {key}
                                  /\ IF staged[key].reclaimed
                                        THEN /\ UNCHANGED staged
                                        ELSE /\ IF fence[key] >= txn_ts
                                                   THEN /\ Assert(FALSE, 
                                                                  "Failure of assertion at line 197, column 11.")
                                                        /\ UNCHANGED staged
                                                   ELSE /\ \/ /\ staged' = [staged EXCEPT ![key] =                       [
                                                                                                                   epoch    |-> txn_epoch,
                                                                                                                   txid       |-> txn_ts,
                                                                                                                   reclaimed |-> FALSE
                                                                                                                 ]]
                                                           \/ /\ TRUE
                                                              /\ UNCHANGED staged
                             /\ pc' = [pc EXCEPT !["committer"] = "PipelineWrites"]
                        ELSE /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecord"]
                             /\ UNCHANGED << staged, to_write >>
                  /\ UNCHANGED << txn, fence, commit_ack, pipelined_keys, 
                                  parallel_keys, attempt, txn_epoch, txn_ts, 
                                  to_check, have_staging_record, prevent_epoch, 
                                  prevent_ts, found_writes, to_resolve >>

StageWritesAndRecord == /\ pc["committer"] = "StageWritesAndRecord"
                        /\ to_write' = parallel_keys
                        /\ to_check' = pipelined_keys
                        /\ have_staging_record' = FALSE
                        /\ IF attempt > MAX_ATTEMPTS
                              THEN /\ pc' = [pc EXCEPT !["committer"] = "EndCommitter"]
                              ELSE /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                        /\ UNCHANGED << txn, staged, fence, 
                                        commit_ack, pipelined_keys, 
                                        parallel_keys, attempt, txn_epoch, 
                                        txn_ts, prevent_epoch, prevent_ts, 
                                        found_writes, to_resolve >>

StageWritesAndRecordLoop == /\ pc["committer"] = "StageWritesAndRecordLoop"
                            /\ IF to_check /= {} \/ to_write /= {} \/ ~have_staging_record
                                  THEN /\ \/ /\ to_check /= {}
                                             /\ pc' = [pc EXCEPT !["committer"] = "QueryPipelinedWrite"]
                                          \/ /\ to_write /= {}
                                             /\ pc' = [pc EXCEPT !["committer"] = "ParallelWrite"]
                                          \/ /\ ~have_staging_record
                                             /\ pc' = [pc EXCEPT !["committer"] = "StageRecord"]
                                  ELSE /\ pc' = [pc EXCEPT !["committer"] = "AckClient"]
                            /\ UNCHANGED << txn, staged, fence, 
                                            commit_ack, pipelined_keys, 
                                            parallel_keys, attempt, txn_epoch, 
                                            txn_ts, to_write, to_check, 
                                            have_staging_record, prevent_epoch, 
                                            prevent_ts, found_writes, 
                                            to_resolve >>

QueryPipelinedWrite == /\ pc["committer"] = "QueryPipelinedWrite"
                       /\ \E key \in to_check:
                            IF QueryIntent(key, txn_epoch, txn_ts)
                               THEN /\ to_check' = to_check \ {key}
                                    /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                                    /\ UNCHANGED attempt
                               ELSE /\ IF txn.status \in {"pending", "staging"}
                                          THEN /\ attempt' = attempt + 1
                                               /\ pc' = [pc EXCEPT !["committer"] = "BeginTxnEpoch"]
                                          ELSE /\ IF txn.status = "aborted"
                                                     THEN /\ pc' = [pc EXCEPT !["committer"] = "EndCommitter"]
                                                     ELSE /\ IF txn.status = "committed"
                                                                THEN /\ pc' = [pc EXCEPT !["committer"] = "AckClient"]
                                                                ELSE /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                                               /\ UNCHANGED attempt
                                    /\ UNCHANGED to_check
                       /\ UNCHANGED << txn, staged, fence, 
                                       commit_ack, pipelined_keys, 
                                       parallel_keys, txn_epoch, txn_ts, 
                                       to_write, have_staging_record, 
                                       prevent_epoch, prevent_ts, found_writes, 
                                       to_resolve >>

ParallelWrite == /\ pc["committer"] = "ParallelWrite"
                 /\ \E key \in to_write:
                      LET cur_intent == staged[key] IN
                        /\ to_write' = to_write \ {key}
                        /\ IF cur_intent.epoch = txn_epoch
                              THEN /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                                   /\ UNCHANGED << staged, attempt, 
                                                   txn_ts >>
                              ELSE /\ IF fence[key] >= txn_ts \/ cur_intent.reclaimed
                                         THEN /\ \/ /\ txn_ts' = txn_ts + 1
                                                    /\ attempt' = attempt + 1
                                                    /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecord"]
                                                 \/ /\ attempt' = attempt + 1
                                                    /\ pc' = [pc EXCEPT !["committer"] = "BeginTxnEpoch"]
                                                    /\ UNCHANGED txn_ts
                                              /\ UNCHANGED staged
                                         ELSE /\ staged' = [staged EXCEPT ![key] =                       [
                                                                                                   epoch    |-> txn_epoch,
                                                                                                   txid       |-> txn_ts,
                                                                                                   reclaimed |-> FALSE
                                                                                                 ]]
                                              /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                                              /\ UNCHANGED << attempt, txn_ts >>
                 /\ UNCHANGED << txn, fence, commit_ack, pipelined_keys, 
                                 parallel_keys, txn_epoch, to_check, 
                                 have_staging_record, prevent_epoch, 
                                 prevent_ts, found_writes, to_resolve >>

StageRecord == /\ pc["committer"] = "StageRecord"
               /\ have_staging_record' = TRUE
               /\ IF txn.status = "pending"
                     THEN /\ txn' = [status |-> "staging", epoch |-> txn_epoch, txid |-> txn_ts]
                          /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                     ELSE /\ IF txn.status = "staging"
                                THEN /\ Assert(txn.epoch <= txn_epoch /\ txn.txid < txn_ts, 
                                               "Failure of assertion at line 296, column 15.")
                                     /\ txn' = [status |-> "staging", epoch |-> txn_epoch, txid |-> txn_ts]
                                     /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                                ELSE /\ IF txn.status = "aborted"
                                           THEN /\ pc' = [pc EXCEPT !["committer"] = "EndCommitter"]
                                           ELSE /\ IF txn.status = "committed"
                                                      THEN /\ Assert(FALSE, 
                                                                     "Failure of assertion at line 303, column 15.")
                                                      ELSE /\ TRUE
                                                /\ pc' = [pc EXCEPT !["committer"] = "StageWritesAndRecordLoop"]
                                     /\ UNCHANGED txn
               /\ UNCHANGED << staged, fence, commit_ack, 
                               pipelined_keys, parallel_keys, attempt, 
                               txn_epoch, txn_ts, to_write, to_check, 
                               prevent_epoch, prevent_ts, found_writes, 
                               to_resolve >>

AckClient == /\ pc["committer"] = "AckClient"
             /\ Assert(Committed, 
                       "Failure of assertion at line 311, column 5.")
             /\ commit_ack' = TRUE
             /\ pc' = [pc EXCEPT !["committer"] = "AsyncExplicitlyCommitted"]
             /\ UNCHANGED << txn, staged, fence, pipelined_keys, 
                             parallel_keys, attempt, txn_epoch, txn_ts, 
                             to_write, to_check, have_staging_record, 
                             prevent_epoch, prevent_ts, found_writes, 
                             to_resolve >>

AsyncExplicitlyCommitted == /\ pc["committer"] = "AsyncExplicitlyCommitted"
                            /\ IF txn.status = "staging"
                                  THEN /\ Assert(ImplicitlyCommitted, 
                                                 "Failure of assertion at line 318, column 7.")
                                       /\ txn' = [txn EXCEPT !.status = "committed"]
                                  ELSE /\ IF txn.status = "committed"
                                             THEN /\ TRUE
                                             ELSE /\ Assert(FALSE, 
                                                            "Failure of assertion at line 326, column 7.")
                                       /\ UNCHANGED txn
                            /\ to_write' = DBS
                            /\ pc' = [pc EXCEPT !["committer"] = "AsyncResolveIntents"]
                            /\ UNCHANGED << staged, fence, commit_ack, 
                                            pipelined_keys, parallel_keys, 
                                            attempt, txn_epoch, txn_ts, 
                                            to_check, have_staging_record, 
                                            prevent_epoch, prevent_ts, 
                                            found_writes, to_resolve >>

AsyncResolveIntents == /\ pc["committer"] = "AsyncResolveIntents"
                       /\ IF to_write /= {}
                             THEN /\ \E key \in to_write:
                                       /\ IF ~staged[key].reclaimed
                                             THEN /\ staged' = [staged EXCEPT ![key].reclaimed = TRUE]
                                             ELSE /\ TRUE
                                                  /\ UNCHANGED staged
                                       /\ to_write' = to_write \ {key}
                                  /\ pc' = [pc EXCEPT !["committer"] = "AsyncResolveIntents"]
                             ELSE /\ pc' = [pc EXCEPT !["committer"] = "EndCommitter"]
                                  /\ UNCHANGED << staged, to_write >>
                       /\ UNCHANGED << txn, fence, commit_ack, 
                                       pipelined_keys, parallel_keys, attempt, 
                                       txn_epoch, txn_ts, to_check, 
                                       have_staging_record, prevent_epoch, 
                                       prevent_ts, found_writes, to_resolve >>

EndCommitter == /\ pc["committer"] = "EndCommitter"
                /\ TRUE
                /\ pc' = [pc EXCEPT !["committer"] = "Done"]
                /\ UNCHANGED << txn, staged, fence, commit_ack, 
                                pipelined_keys, parallel_keys, attempt, 
                                txn_epoch, txn_ts, to_write, to_check, 
                                have_staging_record, prevent_epoch, prevent_ts, 
                                found_writes, to_resolve >>

committer == BeginTxnEpoch \/ PipelineWrites \/ StageWritesAndRecord
                \/ StageWritesAndRecordLoop \/ QueryPipelinedWrite
                \/ ParallelWrite \/ StageRecord \/ AckClient
                \/ AsyncExplicitlyCommitted \/ AsyncResolveIntents
                \/ EndCommitter

PreventLoop(self) == /\ pc[self] = "PreventLoop"
                     /\ found_writes' = [found_writes EXCEPT ![self] = {}]
                     /\ pc' = [pc EXCEPT ![self] = "PushRecord"]
                     /\ UNCHANGED << txn, staged, fence, 
                                     commit_ack, pipelined_keys, parallel_keys, 
                                     attempt, txn_epoch, txn_ts, to_write, 
                                     to_check, have_staging_record, 
                                     prevent_epoch, prevent_ts, to_resolve >>

PushRecord(self) == /\ pc[self] = "PushRecord"
                    /\ IF txn.status = "pending"
                          THEN /\ txn' = [txn EXCEPT !.status = "aborted"]
                               /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                               /\ UNCHANGED << prevent_epoch, prevent_ts >>
                          ELSE /\ IF txn.status = "staging"
                                     THEN /\ prevent_epoch' = [prevent_epoch EXCEPT ![self] = txn.epoch]
                                          /\ prevent_ts' = [prevent_ts EXCEPT ![self] = txn.txid]
                                          /\ pc' = [pc EXCEPT ![self] = "PreventWrites"]
                                     ELSE /\ IF txn.status \in {"committed", "aborted"}
                                                THEN /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                                                ELSE /\ pc' = [pc EXCEPT ![self] = "PreventWrites"]
                                          /\ UNCHANGED << prevent_epoch, 
                                                          prevent_ts >>
                               /\ UNCHANGED txn
                    /\ UNCHANGED << staged, fence, commit_ack, 
                                    pipelined_keys, parallel_keys, attempt, 
                                    txn_epoch, txn_ts, to_write, to_check, 
                                    have_staging_record, found_writes, 
                                    to_resolve >>

PreventWrites(self) == /\ pc[self] = "PreventWrites"
                       /\ IF found_writes[self] /= DBS
                             THEN /\ \E key \in DBS \ found_writes[self]:
                                       IF QueryIntent(key, prevent_epoch[self], prevent_ts[self])
                                          THEN /\ found_writes' = [found_writes EXCEPT ![self] = found_writes[self] \union {key}]
                                               /\ pc' = [pc EXCEPT ![self] = "PreventWrites"]
                                               /\ UNCHANGED fence
                                          ELSE /\ IF fence[key] < prevent_ts[self]
                                                     THEN /\ fence' = [fence EXCEPT ![key] = prevent_ts[self]]
                                                     ELSE /\ TRUE
                                                          /\ UNCHANGED fence
                                               /\ pc' = [pc EXCEPT ![self] = "RecoverRecord"]
                                               /\ UNCHANGED found_writes
                             ELSE /\ pc' = [pc EXCEPT ![self] = "RecoverRecord"]
                                  /\ UNCHANGED << fence, found_writes >>
                       /\ UNCHANGED << txn, staged, commit_ack, 
                                       pipelined_keys, parallel_keys, attempt, 
                                       txn_epoch, txn_ts, to_write, to_check, 
                                       have_staging_record, prevent_epoch, 
                                       prevent_ts, to_resolve >>

RecoverRecord(self) == /\ pc[self] = "RecoverRecord"
                       /\ LET prevented == found_writes[self] /= DBS IN
                            IF prevented
                               THEN /\ LET legal_change ==    txn.epoch >= prevent_epoch[self]
                                                           /\ txn.txid    >  prevent_ts[self] IN
                                         IF txn.status = "aborted"
                                            THEN /\ TRUE
                                                 /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                                                 /\ UNCHANGED txn
                                            ELSE /\ IF txn.status = "committed"
                                                       THEN /\ TRUE
                                                            /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                                                            /\ UNCHANGED txn
                                                       ELSE /\ IF txn.status = "pending"
                                                                  THEN /\ Assert(FALSE, 
                                                                                 "Failure of assertion at line 406, column 15.")
                                                                       /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                                                                       /\ UNCHANGED txn
                                                                  ELSE /\ IF txn.status = "staging"
                                                                             THEN /\ IF legal_change
                                                                                        THEN /\ pc' = [pc EXCEPT ![self] = "PreventLoop"]
                                                                                             /\ UNCHANGED txn
                                                                                        ELSE /\ txn' = [txn EXCEPT !.status = "aborted"]
                                                                                             /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                                                                             ELSE /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                                                                                  /\ UNCHANGED txn
                               ELSE /\ IF txn.status \in {"pending", "aborted"}
                                          THEN /\ Assert(FALSE, 
                                                         "Failure of assertion at line 421, column 13.")
                                               /\ UNCHANGED txn
                                          ELSE /\ IF txn.status \in {"staging", "committed"}
                                                     THEN /\ Assert(txn.epoch = prevent_epoch[self], 
                                                                    "Failure of assertion at line 424, column 13.")
                                                          /\ Assert(txn.txid    = prevent_ts[self], 
                                                                    "Failure of assertion at line 425, column 13.")
                                                          /\ IF txn.status = "staging"
                                                                THEN /\ Assert(ImplicitlyCommitted, 
                                                                               "Failure of assertion at line 429, column 15.")
                                                                     /\ txn' = [txn EXCEPT !.status = "committed"]
                                                                ELSE /\ TRUE
                                                                     /\ UNCHANGED txn
                                                     ELSE /\ TRUE
                                                          /\ UNCHANGED txn
                                    /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                       /\ UNCHANGED << staged, fence, commit_ack, 
                                       pipelined_keys, parallel_keys, attempt, 
                                       txn_epoch, txn_ts, to_write, to_check, 
                                       have_staging_record, prevent_epoch, 
                                       prevent_ts, found_writes, to_resolve >>

ResolveIntents(self) == /\ pc[self] = "ResolveIntents"
                        /\ IF to_resolve[self] /= {}
                              THEN /\ \E key \in to_resolve[self]:
                                        /\ IF ~staged[key].reclaimed
                                              THEN /\ staged' = [staged EXCEPT ![key].reclaimed = TRUE]
                                              ELSE /\ TRUE
                                                   /\ UNCHANGED staged
                                        /\ to_resolve' = [to_resolve EXCEPT ![self] = to_resolve[self] \ {key}]
                                   /\ pc' = [pc EXCEPT ![self] = "ResolveIntents"]
                              ELSE /\ pc' = [pc EXCEPT ![self] = "Done"]
                                   /\ UNCHANGED << staged, to_resolve >>
                        /\ UNCHANGED << txn, fence, commit_ack, 
                                        pipelined_keys, parallel_keys, attempt, 
                                        txn_epoch, txn_ts, to_write, to_check, 
                                        have_staging_record, prevent_epoch, 
                                        prevent_ts, found_writes >>

recoverer(self) == PreventLoop(self) \/ PushRecord(self)
                      \/ PreventWrites(self) \/ RecoverRecord(self)
                      \/ ResolveIntents(self)

(* Allow infinite stuttering to prevent deadlock on termination. *)
Terminating == /\ \A self \in ProcSet: pc[self] = "Done"
               /\ UNCHANGED vars

Next == committer
           \/ (\E self \in RECOVERERS: recoverer(self))
           \/ Terminating

Spec == /\ Init /\ [][Next]_vars
        /\ \A self \in RECOVERERS : WF_vars(recoverer(self))

Termination == <>(\A self \in ProcSet: pc[self] = "Done")

\* END TRANSLATION - the hash of the generated TLA code (remove to silence divergence warnings): TLA-4b99a68dacd4c127554eb3f72922c6c6



=============================================================================
\* Modification History
\* Last modified Sat Sep 12 18:07:57 JST 2020 by ytaka23
\* Last modified Mon Sep 23 17:38:55 EDT 2019 by nathan
\* Created Mon May 13 10:03:40 EDT 2019 by nathan
