# Out-Of-SSA Contract And Parallel-Copy Authority For Prealloc

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Parent Idea: [86_full_x86_backend_contract_first_replan.md](/workspaces/c4c/ideas/open/86_full_x86_backend_contract_first_replan.md)

## Intent

Finish turning `out_of_ssa` into a first-class prealloc phase with explicit
authority over phi elimination, join transfers, and parallel-copy obligations,
instead of leaving those semantics split between historical legalize-era
fallbacks and downstream backend interpretation.

## Owned Failure Family

This idea owns prealloc/BIR contract gaps where:

- semantic BIR CFG and phi form are already preserved through `legalize`
- `out_of_ssa` is the intended phase owner for phi elimination
- the remaining ambiguity is in how join ownership, edge copies, cycle
  breaking, or continuation/critical-edge handling are published
- backend code would otherwise need to infer missing out-of-SSA meaning from
  raw CFG shape or slot-backed leftovers

## Latest Durable Note

As of 2026-04-23, `out_of_ssa` is the sole published owner of phi-elimination
semantics for the cases covered by this idea, `join_transfers` and
`parallel_copy_bundles` are exercised as authoritative downstream contracts in
prepared dumps, x86 prepared-module consumption checks, and regalloc-side
`phi_join_*` move resolution, and the remaining slot-backed `EdgeStoreSlot`
path was retained as an explicit carrier contract rather than a legalize-era
fallback.

## Scope Notes

Expected repair themes include:

- formalizing `out_of_ssa` contract surfaces and invariants
- authoritative publication of `join_transfers` and
  `parallel_copy_bundles`
- cycle-breaking and copy-order semantics for parallel copies
- explicit policy for critical-edge handling and continuation ownership
- tests and dump views that prove `legalize` preserves phi while
  `out_of_ssa` owns elimination

## Boundaries

This idea does not own:

- target-specific register allocation or x86 copy emission details
- stack-layout publication except where phi lowering needs a durable
  target-independent slot-backed carrier contract
- x86-side CFG consumption once truthful out-of-SSA facts are already
  published
- grouped-register resource modeling; that belongs to a separate prealloc
  authority leaf

## Completion Signal

This idea is complete when `out_of_ssa` is the sole authoritative owner of phi
elimination semantics, `join_transfers` and `parallel_copy_bundles` have clear
contract-level meaning, and downstream backends can consume those records
without reopening raw CFG or historical legalize-local phi logic.
