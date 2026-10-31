import Lake
open Lake DSL

package «weft-store-spec» where
  -- Pure Lean, and no dependencies. A spec that needs a network to check is a spec
  -- nobody checks.

@[default_target] lean_lib «Spec» where
  roots := #[`ReadAhead, `ParallelCommit, `Backend, `PipelinedRead, `GroupCommit, `PrewriteCoalesce, `ShardBatchCommit, `ViewCoherence, `PageDelta]
