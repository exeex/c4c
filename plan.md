# Phase E3 Route 7 Materialized-Condition Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/233_phase_e3_route7_materialized_condition_helper_oracle_follow_up.md

## Purpose

Activate the Route 7 follow-up for one materialized-condition helper-oracle row
while preserving prepared oracle strength, branch-policy output, wrappers,
final assembler behavior, helper-oracle strings, and expected strings.

## Goal

Make the selected materialized-condition row in
`verify_prepared_bir_comparison_condition_producer_equivalence` use Route 7
comparison evidence only when that evidence agrees with prepared comparison
facts, and fail closed to existing prepared oracle behavior everywhere else.

## Core Rule

Route 7 may augment the selected materialized-condition helper-oracle row only
after prepared agreement. Prepared comparison helpers and target branch policy
remain authoritative for absent-route, invalid-reference, duplicate/conflict,
mismatch, unfused, prepared fallback, and non-agreement paths.

## Read First

- The source idea named in the metadata above.
- `ideas/closed/216_route7_comparison_oracle_row.md`
- `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/comparison.cpp`
- `src/backend/prealloc/comparison.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- nearby fused-compare, materialized-condition, branch-control, and
  machine-printer tests selected by the supervisor

## Current Targets

- One materialized-condition helper-oracle row in
  `verify_prepared_bir_comparison_condition_producer_equivalence`.
- Route 7 materialized-condition evidence from the existing Route 7 comparison
  condition index/query helpers.
- Prepared agreement against
  `find_prepared_materialized_condition_producer(...)` and the current BIR
  materialized-condition identity.
- Positive agreement plus absent-route, invalid-reference, duplicate/conflict,
  mismatch, unfused, prepared fallback, and non-agreement behavior.
- Byte-stable helper-oracle strings, branch-control output, wrappers, final
  assembler behavior, expected strings, baselines, and supported-path
  contracts.

## Non-Goals

- Do not migrate all Route 7 comparison consumers or helper-oracle rows.
- Do not replace branch-control, machine-printer, route-index, suffix mapping,
  fused legality, hazard checks, wrapper policy, final assembler policy, or
  emitted-output authority.
- Do not change helper names, helper-oracle strings, expected strings,
  branch-control output, baselines, supported/unsupported contracts, wrappers,
  or final assembler behavior.
- Do not start draft 155, E5, aggregate prepared retirement, or broad prepared
  diagnostic/oracle replacement.
- Do not add testcase-shaped matching, fixture-name shortcuts, or
  row-name-only logic to claim Route 7 progress.

## Working Model

- Route 7 owns selected comparison and materialized-condition provenance, not
  target branch spelling, fused legality, suffix policy, hazards, emitted
  register state, final instruction order, or final assembler rows.
- `route7_build_comparison_condition_index(...)`,
  `route7_find_materialized_condition(...)`, and the Route 7 reference facade
  expose route-native materialized-condition evidence.
- `find_prepared_materialized_condition_producer(...)` remains the prepared
  oracle authority for compatibility and fallback behavior.
- The helper-oracle row can report Route 7-backed positive evidence only when
  Route 7, BIR identity, and prepared facts agree on the same materialized
  condition producer.

## Execution Rules

- Start by locating the exact row and current assertion shape before editing
  implementation.
- Preserve prepared oracle authority for absent route evidence, invalid
  references, duplicate/conflicting facts, mismatches, unfused cases, prepared
  fallback, non-agreement, branch-control output, wrappers, final assembler
  behavior, and public fallback.
- Prefer existing Route 7 materialized-condition index/query and prepared
  comparison helper surfaces over adding a new diagnostic channel.
- Keep helper-oracle strings, branch-control output, wrapper output, expected
  strings, baselines, and supported/unsupported contracts byte-stable unless
  the supervisor explicitly approves a different contract.
- Every code-changing step needs fresh build or compile proof plus the
  supervisor-delegated test subset. Broader validation is required before
  acceptance if the implementation touches shared Route 7 indexing, AArch64
  branch-control, machine-printer, wrapper, final assembler, or expected-output
  surfaces beyond the selected row.

## Steps

### Step 1: Locate The Row And Agreement Boundary

Goal: identify the selected materialized-condition helper-oracle row, the Route
7 evidence it can use, and the existing tests that prove positive and fallback
behavior.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/comparison.cpp`
- `src/backend/prealloc/comparison.hpp`

Actions:

- Inspect `verify_prepared_bir_comparison_condition_producer_equivalence` and
  identify the exact materialized-condition row and assertion shape.
- Trace the agreement boundary through
  `route7_build_comparison_condition_index(...)`,
  `route7_find_materialized_condition(...)`,
  `route_index_validate_materialized_condition_reference(...)`,
  `find_materialized_condition_producer_identity(...)`, and
  `find_prepared_materialized_condition_producer(...)`.
- Inventory existing positive, absent-route, invalid-reference,
  duplicate/conflict, mismatch, unfused, prepared fallback, non-agreement,
  branch-control, machine-printer, wrapper, final assembler, and
  expected-string coverage.
- Record the exact row, minimal Step 2 target files, and fallback/proof gaps in
  `todo.md` before implementation edits.

Completion check:

- `todo.md` names the selected helper-oracle row, the Route 7/prepared
  agreement boundary, minimal implementation targets, and fallback/proof gaps
  for Step 2.
- No code, helper-oracle string, expected-string, baseline, branch-control,
  wrapper, machine-printer, or final assembler behavior change is made in this
  step unless the executor is explicitly delegated to start implementation.

### Step 2: Use Route 7 Evidence Only Under Prepared Agreement

Goal: augment the selected materialized-condition helper-oracle row with Route
7 comparison evidence after prepared agreement.

Primary targets:

- The helper-oracle row path found in Step 1
- Existing Route 7 query/reference helpers in `src/backend/bir/bir.*`
- Existing prepared comparison helpers in `src/backend/prealloc/comparison.*`

Actions:

- Reuse existing Route 7 materialized-condition status, record lookup, and
  reference validation where possible.
- Require agreement between Route 7 materialized-condition evidence, BIR
  identity, and prepared materialized-condition producer facts before the row
  can count as Route 7-backed evidence.
- Fail closed to existing prepared oracle behavior for absent route evidence,
  invalid references, duplicate/conflicting route facts, route/prepared
  mismatch, unfused cases, prepared fallback, non-agreement, branch-control,
  wrappers, final assembler behavior, and public fallback.
- Keep helper-oracle strings, expected strings, branch-control output, wrapper
  output, final assembler behavior, baselines, and supported/unsupported
  contracts byte-stable.

Completion check:

- The selected positive row is explained by Route 7 materialized-condition
  evidence after prepared agreement.
- Every non-agreement path retains existing prepared oracle authority.
- The slice builds and the delegated narrow proof passes without expectation
  or baseline rewrites.

### Step 3: Prove Fallback And Nearby Same-Feature Stability

Goal: prove the implementation is semantic and not shaped around one known
fixture.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- nearby fused-compare and materialized-condition helper-oracle cases
- machine-printer, wrapper, and final assembler stability checks selected by
  the supervisor

Actions:

- Add or tighten focused tests only where coverage is missing for the selected
  row and fallback matrix.
- Cover positive Route 7/prepared agreement plus absent route evidence,
  invalid reference, duplicate/conflict, mismatch, unfused, prepared fallback,
  and non-agreement behavior.
- Check nearby same-feature materialized-condition and fused-compare cases so
  the change does not depend on one fixture shape.
- Preserve branch-control output, wrappers, final assembler behavior,
  helper-oracle strings, expected strings, baselines, and
  supported/unsupported contracts.

Completion check:

- Focused tests or explicit proof cover the positive row and every required
  fallback path.
- Nearby same-feature coverage demonstrates this is not a named-case shortcut.
- No branch-control, machine-printer, wrapper, final assembler,
  expected-string, baseline, or supported/unsupported contract change is part
  of the diff.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed helper-oracle row slice and leave clear handoff state
for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow lookup-helper and Route 7 branch-control tests
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact supervisor-delegated proof command and record the command and
  result in `todo.md`.
- If the change touched shared Route 7 indexing, AArch64 branch-control,
  machine-printer, wrapper, final assembler, expected-output, or prepared
  comparison helper behavior beyond the selected row, ask the supervisor to
  choose broader validation before acceptance.
- Summarize the row changed, retained prepared oracle authority,
  same-feature proof, unchanged strings/output, and any residual risk.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  authority, unchanged output/string surfaces, and residual risks.
