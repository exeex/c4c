# Out-Of-SSA Contract And Parallel-Copy Authority For Prealloc

Status: Active
Source Idea: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Activated from: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Supersedes: ideas/open/86_full_x86_backend_contract_first_replan.md

## Purpose

Make `out_of_ssa` a truthful prealloc phase with clear contract ownership over
phi elimination, join-transfer publication, and parallel-copy obligations, so
later backends consume authority instead of reconstructing CFG meaning from raw
shape or historical legalize leftovers.

## Goal

Reach a state where:

- `legalize` preserves semantic CFG and phi form
- `out_of_ssa` is the only owner of phi elimination semantics
- `join_transfers` and `parallel_copy_bundles` have stable contract meaning
- dumps and tests expose out-of-SSA authority directly

## Core Rules

- do not push phi or parallel-copy meaning back into `legalize`
- do not let target backends infer out-of-SSA ownership from raw block shape
- prefer target-independent contract fields and helpers over x86-specific
  recovery logic
- keep behavior-preserving semantics ahead of cleanup-only refactors

## Read First

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/backend_prepare_phi_materialize_test.cpp`
- `tests/backend/backend_prepare_authoritative_join_ownership_test.cpp`

## Current Targets / Scope

- `PreparedJoinTransfer`
- `PreparedParallelCopyBundle`
- prepared CFG / continuation ownership around phi elimination
- out-of-SSA dump coverage and internal proof coverage

## Non-Goals

- target-specific register copy emission
- x86-side CFG consumption changes unless a truthful contract bug requires it
- grouped-register resource modeling
- generic frame/stack/call authority work unrelated to out-of-SSA

## Working Model

- `legalize` owns type/ABI/control-flow legalization and phi preservation
- `out_of_ssa` owns phi elimination and edge-copy publication
- `regalloc` and later backends consume published out-of-SSA facts, not raw
  CFG reinterpretation

## Execution Rules

- if a case still depends on slot-backed phi carriers, describe that as an
  explicit out-of-SSA carrier contract, not an accidental fallback
- if a missing fact is target-independent continuation or edge-copy meaning,
  publish it in prealloc instead of inventing an emitter-local rule
- keep dump and test coverage aligned with every contract change

## Step 1: Contract Surface Audit

Goal: define what `out_of_ssa` already publishes, what remains implicit, and
which records are authoritative versus accidental leftovers.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- inventory current `PreparedJoinTransfer` and `PreparedParallelCopyBundle`
  fields
- document missing or ambiguous semantics around carriers, edge ownership, and
  cycle-breaking
- make the prepared dump expose any missing summary needed for human review

Completion check:

- the active contract gaps are listed concretely
- dump output makes current out-of-SSA ownership readable without source dives

## Step 2: Join And Parallel-Copy Authority Completion

Goal: publish target-independent out-of-SSA meaning for join transfers and
parallel copies so downstream phases consume plans, not topology guesses.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- strengthen join-transfer and parallel-copy semantics where current fields are
  underspecified
- make continuation ownership and cycle-breaking policy explicit when needed
- keep publication generic and target-independent

Completion check:

- join-transfer and parallel-copy records fully explain edge obligations for
  owned cases
- no downstream consumer needs legalize-era leftovers or raw CFG guesses

## Step 3: Proof And Observation Tightening

Goal: lock the phase boundary and new authority into tests and dumps.

Primary target:

- `tests/backend/backend_prepare_phi_materialize_test.cpp`
- `tests/backend/backend_prepare_authoritative_join_ownership_test.cpp`
- `tests/backend/backend_prepared_printer_test.cpp`
- relevant CLI dump checks under `tests/backend/`

Actions:

- add or tighten tests for phi preservation in `legalize`
- add or tighten tests for phi elimination and edge-copy publication in
  `out_of_ssa`
- update dump assertions so new contract fields are visible and stable

Completion check:

- phase-boundary tests prove `legalize` preserves phi and `out_of_ssa` removes
  it
- dump tests cover the new published authority

## Step 4: Cleanup And Consumer Confirmation

Goal: remove obsolete leftovers and confirm downstream prealloc phases still
consume the new authority cleanly.

Primary target:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/regalloc.cpp`

Actions:

- delete dead pre-out-of-SSA leftovers once coverage is in place
- confirm `regalloc` and later prepared consumers read the published facts
  without hidden dependencies on removed helpers

Completion check:

- no duplicate phi-lowering ownership remains
- build and owned backend/prealloc proofs stay green after cleanup
