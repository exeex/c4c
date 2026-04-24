# Out-Of-SSA Critical-Edge And Parallel-Copy Deepening

Status: Open
Created: 2026-04-24
Last-Updated: 2026-04-24
Parent Idea: [87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md](/workspaces/c4c/ideas/closed/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md)

## Intent

Deepen the post-idea-87 `out_of_ssa` contract so critical-edge handling,
parallel-copy ordering, cycle breaking, and copy coalescing policy are
published as explicit target-independent authority rather than left as partial
implementation detail.

## Owned Failure Family

This idea owns prealloc/BIR contract gaps where:

- `out_of_ssa` is already the truthful owner of phi elimination
- the remaining ambiguity is in how edge copies are sequenced, normalized, or
  repaired at CFG boundaries
- downstream consumers can read `join_transfers` and
  `parallel_copy_bundles`, but the bundle semantics are still not rich enough
  to serve as the final contract for harder CFG cases
- the first missing fact is target-independent copy/edge ownership, not x86
  instruction selection

## Scope Notes

Expected repair themes include:

- explicit critical-edge policy for out-of-SSA publication
- stronger cycle-breaking and copy-order semantics for
  `PreparedParallelCopyBundle`
- clearer contract around temporary carriers and copy coalescing boundaries
- richer prepared dump summaries for per-edge copy obligations
- tests covering harder join/edge-copy cases than the current phase-boundary
  fixtures

## Boundaries

This idea does not own:

- target-specific register-copy spelling or final assembly rendering
- generic frame/stack/call authority publication
- grouped-register allocation semantics except where grouped occupancy changes
  target-independent copy publication rules
- reopening legalize as the owner of phi elimination

## Completion Signal

This idea is complete when the remaining hard parts of out-of-SSA edge-copy
publication are explicit in prepared contracts and dumps, and downstream phases
no longer need to infer critical-edge or cycle-breaking behavior from
implementation accidents.
