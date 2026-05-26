# AArch64 Absent-Selection Fallback Retirement Runbook

Status: Active
Source Idea: ideas/open/17_aarch64_absent_selection_fallback_retirement.md

## Purpose

Retire the remaining temporary AArch64 absent-selection fallback paths for local
aggregate address publication and indirect byval lane payload publication.

Goal: make the affected AArch64 lowering paths consume complete prepared facts
or fail closed with explicit diagnostics when those facts are absent.

Core Rule: do not replace one target-local rederivation fallback with another;
prepared call/source facts must be the source of truth for these paths.

## Read First

- `ideas/open/17_aarch64_absent_selection_fallback_retirement.md`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`

## Current Targets

- Local aggregate address call-argument publication when prepared source
  selection is absent or incomplete.
- Indirect byval lane payload publication for
  `call_arg_byval_aggregate_register_lanes`.
- Prepared source-selection records and printers that should expose enough facts
  for those paths.
- Focused backend tests that currently prove either semantic behavior or the
  legacy fallback contract.

## Non-Goals

- Do not perform broad BIR edge value-flow migration.
- Do not clean up unrelated dispatch edge-copy or publication authority.
- Do not consolidate calls files.
- Do not change ABI classification rules.
- Do not add a new temporary fallback under a different helper name.
- Do not weaken supported-path expectations to make fallback retirement easier.

## Working Model

- Prepared call plans should publish source selections through
  `PreparedCallArgumentSourceSelection` where AArch64 lowering needs local
  aggregate address or byval lane payload authority.
- AArch64 lowering should use `source_selection`, prepared value homes, prepared
  frame slots, prepared materializations, and prepared access facts directly.
- Missing or internally inconsistent prepared facts should produce precise
  diagnostics rather than recovering by reconstructing a source from raw local
  stack or value-home state.
- Tests may stop protecting the old fallback shape only when equivalent semantic
  behavior and missing-fact fail-closed coverage remain.

## Execution Rules

- Keep each implementation packet narrow: one fallback family or one prepared
  fact gap at a time.
- Preserve source idea stability; routine packet results belong in `todo.md`.
- Treat testcase-shaped matching, expectation downgrades, and helper renames as
  route failures.
- If a required prepared fact is missing, add the minimal shared prepared field,
  query, or printer coverage needed by the existing authority model.
- For code-changing steps, run a fresh build or compile proof plus the focused
  backend tests touched by the step. Escalate to broader backend validation
  before closure.

## Steps

### Step 1: Audit the Two Fallback Families

Goal: produce a concrete map of the remaining absent-selection fallback sites.

Primary targets:
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/prealloc/call_plans.cpp`
- focused tests under `tests/backend/bir/` and `tests/backend/mir/`

Actions:
- Inspect all `source_selection` absence branches in the named AArch64 call
  publication paths.
- Classify each branch as local aggregate address, indirect byval lane payload,
  unrelated required diagnostic, or already prepared-authoritative.
- Identify tests that explicitly expect missing local aggregate address or
  reconstructed byval lane behavior.
- Record the audit map in `todo.md` before implementation starts.

Completion check:
- `todo.md` names each owned fallback branch, the prepared fact it should use,
  and the focused tests that prove current behavior.

### Step 2: Retire Local Aggregate Address Fallbacks

Goal: make local aggregate address publication rely on prepared source facts.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:
- Confirm whether current prepared call plans already carry the required local
  frame address selection facts.
- If a fact is missing, add the narrowest prepared record/query needed by both
  printer and AArch64 consumer paths.
- Remove local aggregate address reconstruction from AArch64 lowering for the
  owned path.
- Add or update missing-fact tests so absence fails closed with a precise
  diagnostic.
- Preserve positive local aggregate address publication coverage.

Completion check:
- The owned local aggregate address route lowers from prepared facts only, and
  deleting the relevant prepared fact causes a diagnostic rather than fallback
  reconstruction.

### Step 3: Retire Indirect Byval Lane Payload Fallbacks

Goal: make byval register-lane payload publication rely on complete prepared
source-selection bytes and lane extent facts.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:
- Verify `PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane` carries
  the payload slot, offset, size, alignment, lane extent, and source instruction
  facts needed by all byval lane publication paths.
- Fill only real prepared fact gaps; do not derive the payload from unrelated
  target-local homes at emission time.
- Remove fallback collection or reconstruction from the owned byval lane paths.
- Strengthen diagnostics for incomplete prepared selected source bytes.
- Keep focused positive tests for register-lane and stack-lane byval behavior.

Completion check:
- Byval lane publication succeeds from prepared selection facts and fails closed
  when those facts are removed or incomplete.

### Step 4: Normalize Legacy Fallback Tests

Goal: preserve semantic coverage while retiring tests that only protect the
legacy fallback contract.

Primary targets:
- tests identified during Step 1
- any backend prepared-printer tests needed for new or clarified facts

Actions:
- Convert fallback-shape expectations into prepared-fact authority expectations.
- Keep positive behavior coverage for local aggregate address and indirect byval
  publication.
- Keep negative coverage for missing prepared facts and incomplete source
  selection.
- Do not downgrade supported routes to unsupported or weaker contracts.

Completion check:
- Tests no longer require broad source rederivation, but still prove behavior
  and fail-closed diagnostics.

### Step 5: Validation and Closure Notes

Goal: prove the cleanup did not regress backend behavior and record the final
mapping from legacy path to prepared authority.

Actions:
- Run a fresh build or compile proof.
- Run the focused backend tests touched by Steps 2 through 4.
- Run supervisor-selected broader backend validation before closure.
- Record in `todo.md` the final mapping of each retired path to its replacement
  prepared fact or precise unreachable condition.

Completion check:
- Validation is green, no owned fallback route remains broad or target-local,
  and the source idea can be closed with the required mapping note.
