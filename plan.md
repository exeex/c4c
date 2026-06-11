# Route 7 Comparison/Condition Oracle Thinning Runbook

Status: Active
Source Idea: ideas/open/173_route7_comparison_condition_oracle_thinning.md

## Purpose

Move one comparison or branch semantic consumer from prepared comparison
lookup answers to Route 7 comparison/condition records or selected route-index
validation, then contract only the prepared comparison oracle surface made
provably redundant by that migration.

## Goal

Make one real comparison/branch provenance consumer read Route 7 semantic
records while keeping target branch policy and compare emission decisions out
of BIR.

## Core Rule

Route 7 may own comparison operand, producer, materialized-condition, and
branch-condition provenance. It must not own target branch spelling,
fused-compare legality, condition-code selection, hazards, emitted-register
state, final instruction records, or AArch64 compare/branch instruction
selection.

## Read First

- `ideas/open/173_route7_comparison_condition_oracle_thinning.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/comparison.hpp`
- `src/backend/prealloc/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- comparison/branch target tests under `tests/backend/mir/`

## Current Targets

- `bir::Route7ComparisonConditionIndex`
- `bir::route7_find_comparison_operand(...)`
- `bir::route7_find_materialized_condition(...)`
- `bir::route7_find_branch_condition(...)`
- `bir::route7_validate_*_reference(...)`
- `bir::route_index_validate_materialized_condition_reference(...)`
- `bir::find_fused_compare_operand_producer_facts(...)`
- `bir::find_materialized_condition_producer_identity(...)`
- prepared compatibility helpers in `src/backend/prealloc/comparison.*`
- one AArch64 comparison or branch provenance consumer in
  `src/backend/mir/aarch64/codegen/comparison.cpp`

## Non-Goals

- Do not migrate fused-compare legality or branch-selection policy into BIR.
- Do not change target condition-code selection or final MIR instruction order.
- Do not downgrade supported-path tests to unsupported.
- Do not remove prepared comparison helpers while unexpanded consumers still
  need them as migration oracles.
- Do not claim progress from helper renames, expectation rewrites, or facade
  reshaping alone.

## Working Model

Prepared comparison helpers remain the oracle until the selected consumer has a
Route 7-backed path with matching lhs/rhs producer, integer constant,
materialized-condition, and negative-case behavior. Contraction is allowed only
after the executor proves the selected prepared answer is no longer required by
that migrated consumer.

## Execution Rules

- Move one consumer boundary at a time.
- Prefer Route 7 records or fail-closed route-index validation over generic BIR
  scans.
- Preserve prepared answers long enough to compare old and new behavior.
- Keep code changes behavior-preserving unless the source idea explicitly
  requires removal of a redundant prepared-only oracle path.
- Every code-changing step needs fresh build proof plus the delegated narrow
  subset from the supervisor.
- Escalate to broader comparison/branch validation before treating contraction
  as acceptance-ready.

## Step 1: Inventory The Current Consumer Boundary

Goal: identify the smallest comparison or branch semantic consumer that can
read Route 7 provenance without moving target policy.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/comparison.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect current AArch64 calls to
  `bir::find_fused_compare_operand_producer_facts(...)` and
  `bir::find_materialized_condition_producer_identity(...)`.
- Identify whether the first migration should target fused-operand producer
  provenance, materialized-condition producer identity, or branch-condition
  provenance.
- Record the selected consumer, nearby prepared oracle, and required test
  subset in `todo.md`.
- Do not edit implementation files in this step unless the supervisor delegates
  a combined discovery-and-code packet.

Completion check:

- `todo.md` names one selected consumer boundary and a concrete proof subset
  for Step 2.

## Step 2: Add Route 7-Backed Consumer Read

Goal: make the selected consumer obtain semantic provenance from
`Route7ComparisonConditionIndex` records or selected route-index validation.

Actions:

- Thread or build the narrow Route 7 index needed by the selected consumer.
- Replace only the selected semantic lookup with a Route 7-backed read.
- Keep prepared lookup results available as oracle or fallback while proving
  equivalence.
- Preserve all target-local decisions in the AArch64 lowering layer.
- Add or extend tests for lhs/rhs producer facts, integer constants,
  materialized conditions, and fail-closed negative cases relevant to the
  selected consumer.

Completion check:

- The selected consumer reads Route 7 provenance for its semantic answer.
- Existing comparison/branch behavior remains unchanged.
- Narrow Route 7 oracle tests and the selected comparison/branch target subset
  pass.

## Step 3: Prove Equivalence Against Prepared Oracles

Goal: prove the Route 7-backed path matches the prepared helper answers before
any contraction.

Actions:

- Compare Route 7 answers with prepared fused-operand or
  materialized-condition helper answers for the selected consumer.
- Cover lhs producer, rhs producer, integer constant, materialized-condition,
  unavailable, and wrong-key or wrong-relationship cases as applicable.
- Check that failure statuses remain fail-closed rather than silently scanning
  broad BIR structures.

Completion check:

- Tests demonstrate equivalence for the selected consumer and reject nearby
  invalid references.
- No expectation downgrade is part of the proof.

## Step 4: Contract Only Redundant Prepared Surface

Goal: remove or narrow only the prepared comparison/cache surface made unused
by the migrated consumer.

Actions:

- Inspect remaining references to the prepared helper or cache surface.
- Contract only the piece no longer needed by expanded consumers.
- Leave public oracle helpers in place for any remaining branch/comparison
  consumers.
- Update tests only to reflect the real ownership boundary; do not weaken
  supported behavior.

Completion check:

- The contracted prepared surface is no longer used by the migrated consumer.
- Remaining consumers still have their required oracle or compatibility path.
- Narrow tests plus comparison/branch target subset pass.

## Step 5: Acceptance Validation

Goal: prove the slice is a semantic migration and not testcase-shaped
contraction.

Actions:

- Run the delegated narrow Route 7 oracle tests.
- Run the delegated comparison/branch target subset.
- Escalate to broader backend validation if the diff touches shared Route 7
  validation, route-index facade behavior, or multiple AArch64 comparison
  consumers.

Completion check:

- Fresh proof logs show the selected Route 7 consumer path and relevant target
  subset are green.
- No branch spelling, fused legality, condition-code, hazard, emitted-register,
  or final-record behavior moved into BIR.
