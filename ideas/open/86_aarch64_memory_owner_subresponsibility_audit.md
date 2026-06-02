# AArch64 Memory Owner Subresponsibility Audit

## Goal

Audit `src/backend/mir/aarch64/codegen/memory.cpp` after dispatch publication,
edge-copy, local-helper, and prepared-wrapper contraction, then produce scoped
implementation ideas only for memory subresponsibilities whose owner boundary is
clear.

## Why This Exists

Memory became the clearest post-contraction residue. Ideas 80 and 81 moved
owner-local memory helpers back from dispatch publication and edge-copy files,
and idea 84 removed only the redundant prepared wrapper surfaces. The result is
a more correct but larger memory owner.

This route should decide whether the growth is justified target-local memory
lowering, or whether memory now contains separable subresponsibilities such as
frame-slot address materialization, stack-source publication, store
retargeting, identity validation, or prepared memory-record construction.

## Owned Files

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- Read-only comparison against prior closed ideas:
  `ideas/closed/70_aarch64_memory_prepared_address_authority_cleanup.md`,
  `ideas/closed/80_aarch64_dispatch_publication_owner_relocation.md`,
  `ideas/closed/81_aarch64_dispatch_edge_copy_owner_contraction.md`,
  `ideas/closed/83_aarch64_local_helper_duplication_tail_cleanup.md`, and
  `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`.

## In Scope

- Build a function-level inventory of memory owner responsibilities.
- Classify helper clusters as:
  - `target-local-memory-emission`
  - `frame-slot-addressing`
  - `stack-source-publication`
  - `store-retargeting`
  - `identity-validation`
  - `prepared-record-construction`
  - `diagnostics-and-error-spelling`
  - `candidate-local-subowner`
  - `needs-shared-authority-evidence`
- Produce follow-up ideas only when a cluster has a stable owner boundary and
  proof route.
- Explain which large clusters should stay in `memory.cpp`.

## Out Of Scope

- Direct implementation edits in `memory.cpp`.
- Moving AArch64 addressing legality, scratch choice, register spelling, or
  machine-record construction into shared BIR/prealloc.
- Reopening dispatch publication or edge-copy relocation.
- Treating line-count reduction as success without preserving memory lowering
  ownership.

## Proof Expectations

- This is audit-only; no backend tests are required unless implementation files
  are accidentally touched.
- The audit close note must name any follow-up ideas created and explicitly say
  which memory clusters remain intentionally target-local.

## Reviewer Reject Signals

- The audit proposes one monolithic "shrink memory.cpp" implementation route.
- It says "move to BIR" without naming a missing target-neutral fact.
- It reopens dispatch relocation despite 80/81 closure notes.
- It treats owner relocation growth as a regression without evidence.

