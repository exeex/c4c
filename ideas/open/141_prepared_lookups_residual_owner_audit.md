# 141 Prepared lookups residual owner audit

## Goal

Audit the remaining public and implementation surface in
`src/backend/prealloc/prepared_lookups.hpp` and
`src/backend/prealloc/prepared_lookups.cpp` after ideas 136-140 moved the first
round of domain-owned query APIs out of the broad prepared lookup facade.

This idea is analysis-only. It should not directly rewrite prealloc or AArch64
codegen. It should produce focused follow-up ideas only where a remaining
prepared-lookup group has a clearer domain owner or exposes target-facing
routing shape that should be separated from reusable prepared facts.

## Why This Exists

Idea 136 concluded that `prepared_lookups.*` should shrink by moving or
re-exporting ownership to domain owners, not by deleting reusable prepared
facts. Ideas 137-140 then handled the first concrete groups:

- select-chain query public ownership
- call-plan lookup ownership
- addressing lookup ownership
- edge-publication facade splitting

That reduced the public facade substantially, but `prepared_lookups.*` still
owns several mixed groups:

- move-bundle and return-chain lookup indexes
- value-home lookup indexes and the `PreparedFunctionLookups` aggregate
- current-block entry publication lookup
- same-block scalar producer and integer-constant materialization queries
- call-argument source producer materialization queries
- fused-compare and materialized-condition producer facts
- same-block load-local stored-value source facts
- current-block join parallel-copy source facts and instruction-routing
  conveniences

Some of these may be coherent as a small central prepared-query aggregate.
Others may belong under control-flow, publication, comparison, value-home, or
same-block materialization ownership. The next step should classify this
residual surface before opening implementation work.

## Required Analysis

Inspect the remaining declarations and definitions in:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/select_chain_lookups.hpp`

Also inspect AArch64 consumers that still include or call residual
`prepared_lookups` APIs, especially:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`

For each residual group, classify it as:

- `keep-core-aggregate`: central lookup/index data needed by
  `PreparedFunctionLookups` or many domains;
- `move-to-domain-owner`: declarations or definitions should move to a clearer
  owner such as control-flow, publication, comparison, value-home, or
  same-block materialization;
- `split-fact-from-routing`: reusable prepared fact is valid, but target-facing
  routing convenience should be separated or named explicitly;
- `consumer-cleanup`: AArch64 consumers should include a narrower owner header
  or call a more precise domain API;
- `no-new-idea`: current owner is justified and should remain.

The audit must explicitly revisit idea 140's target:

- whether current-block join parallel-copy source facts are reusable shared
  facts or target-facing routing convenience;
- whether `PreparedCurrentBlockJoinParallelCopyInstructionRouting` belongs in
  `prepared_lookups.*`, control-flow ownership, publication ownership, or
  AArch64 codegen;
- whether same-width stack-source and aggregate stack-source publication
  helpers now have a clear publication-plan owner.

## Expected Output

The closure note must contain:

- a residual ownership table for every remaining public group in
  `prepared_lookups.hpp`;
- a definition-location map for the corresponding groups in
  `prepared_lookups.cpp`;
- a consumer map for AArch64 and prealloc users;
- explicit no-new-idea notes for groups that should remain under
  `prepared_lookups.*`;
- follow-up ideas for concrete owner moves, facade splits, or consumer include
  cleanups;
- a recommendation for the long-term role of `PreparedFunctionLookups`.

If the residual surface is already coherent after 137-140, close with no
follow-up ideas and explain why `prepared_lookups.*` is now the right central
aggregate instead of an overgrown facade.

## Out Of Scope

- Direct code changes in this analysis idea.
- Deleting shared queries and forcing AArch64 consumers back to local BIR
  scans, predecessor rescans, or name matching.
- Moving AArch64 register spelling, scratch policy, hazard policy, or final
  instruction emission into shared prealloc code.
- Reopening select-chain, call-plan, addressing, or edge-publication ownership
  work already completed by ideas 137-140 unless a concrete residual dependency
  proves the prior slice left an unresolved owner boundary.
- Broad redesign of control-flow, out-of-SSA, or call planning.

## Reviewer Reject Signals

- The route creates vague follow-ups such as "clean prepared lookups" without
  naming specific residual groups and owner files.
- It treats line count alone as evidence that a query should move.
- It proposes moving target-local AArch64 routing/emission policy into
  prealloc.
- It proposes deleting semantic prepared facts that x86/riscv would need to
  avoid reimplementing AArch64-style rediscovery.
- It duplicates work already completed by ideas 137-140.
- It ignores current AArch64 consumers and creates header/API churn without a
  clearer ownership boundary.

## Suggested Proof Route For Follow-Ups

This idea itself is analysis-only. Follow-up implementation ideas should choose
their proof by touched owner:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- full `ctest --test-dir build -j --output-on-failure` only if shared prepared
  semantics change rather than API ownership or include boundaries
