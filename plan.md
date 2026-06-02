# AArch64 Call-Boundary Aggregate-Lane Record Schema Cleanup Runbook

Status: Active
Source Idea: ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md

## Purpose

Make the aggregate-lane shape carried by
`CallBoundaryMoveInstructionRecord` explicit through a narrow schema,
tagged sub-shape, or validated accessor cleanup without changing existing
call-boundary move behavior.

## Goal

Clarify the aggregate register-lane publication contract while preserving
ordinary call-boundary moves, value publication, preservation moves, immediate
publication, f128 facts, prepared frame-slot memory, printer output, and
diagnostics.

## Core Rule

This is a no-semantics schema cleanup. Preserve accepted and rejected record
shapes, generated assembly, diagnostics, unsupported-path behavior, selection
status, and record-surface classification.

## Read First

- `ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Targets

- `CallBoundaryMoveInstructionRecord`
- `is_aggregate_register_lane_publication`
- `aggregate_register_lane_scratch`
- `aggregate_register_lane_memory`
- `aggregate_register_lane_memory_is_printable`
- `aggregate_register_lane_printable_chunk_descriptor`
- `aggregate_register_lane_destination`
- aggregate-lane record construction in `calls.cpp`
- aggregate-lane printer dispatch in `machine_printer.cpp`
- ordinary call-boundary move, immediate, f128, frame-slot, value publication,
  and preservation paths that share `CallBoundaryMoveInstructionRecord`

## Non-Goals

- Do not split every call-boundary record family.
- Do not redesign `CallBoundaryMoveInstructionRecord` beyond the narrow
  aggregate-lane schema/accessor boundary.
- Do not move AArch64 ABI construction, selected-node facts, source/destination
  selection, prepared-source authority, or scratch/preservation decisions out
  of `calls.cpp`.
- Do not delete or weaken printer-side validation because a helper validates
  the aggregate-lane shape earlier.
- Do not change assembly output, diagnostics, unsupported-path behavior,
  selection status, or record-surface classification.
- Do not rework `CallBoundaryAbiBindingInstructionRecord`, direct/indirect call
  records, or generic printer entry points without separate traced evidence.

## Working Model

`CallBoundaryMoveInstructionRecord` remains the broad call-boundary move record
unless a narrow aggregate-lane sub-shape can be made explicit without changing
behavior. `calls.cpp` owns construction and AArch64 ABI facts. `instruction.*`
may expose the narrow record schema or validated accessors. `machine_printer.cpp`
still owns printable assembly constraints, diagnostics, scratch selection, and
the final aggregate-lane print route.

## Execution Rules

- Start by mapping current record shapes and validations before editing code.
- Prefer an explicit aggregate-lane schema or validated accessor that removes
  ambiguity from the aggregate-lane path without weakening generic move checks.
- Keep ordinary call-boundary move validation and aggregate-lane validation
  separately reviewable.
- Treat expectation rewrites, helper-only renames, and named-testcase shortcuts
  as route drift unless they are paired with a real schema/accessor cleanup.
- For every code-changing step, run a fresh build or compile proof plus the
  delegated focused tests.
- Use matching `test_before.log` and `test_after.log` when public instruction
  declarations, record fields, helper predicates, or printer dispatch
  contracts change.

## Ordered Steps

### Step 1: Map Current Call-Boundary Record Shapes

Goal: identify the exact aggregate-lane shape and nearby non-aggregate shapes
before any schema or accessor changes.

Primary targets:

- `CallBoundaryMoveInstructionRecord`
- `is_aggregate_register_lane_publication`
- aggregate-lane construction and validation in `calls.cpp`
- aggregate-lane and ordinary move printing in `machine_printer.cpp`

Concrete actions:

- Inspect every field used by aggregate register-lane publication and record
  which invariant currently comes from phase, move reason, source memory,
  destination register, provenance, size, and register view checks.
- Classify ordinary call-boundary moves, value/preservation moves, immediate
  publication, f128 carrier moves, and prepared frame-slot memory moves that
  must keep the same generic record shape.
- Identify rejection paths in `calls.cpp`, `instruction.cpp`, and
  `machine_printer.cpp` that must remain observable after cleanup.
- Decide whether the narrow route should be a tagged sub-schema, a small
  aggregate-lane view object, or validated accessor helpers.
- Do not edit implementation code in this step unless the executor is
  explicitly delegated a code-changing packet.

Completion check:

- `todo.md` names the selected schema/accessor route, the preserved record
  shapes, the rejection paths to keep, and the narrow proof command.

### Step 2: Introduce the Narrow Aggregate-Lane Schema or Accessor

Goal: make aggregate-lane publication shape explicit at the instruction-record
boundary without changing behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`

Concrete actions:

- Add the minimal aggregate-lane sub-shape, view type, or validated accessor
  chosen in Step 1.
- Keep validation equivalent to the old
  `is_aggregate_register_lane_publication` contract.
- Preserve helper behavior for chunk selection, memory printability, scratch
  register choice, and X-view destination validation.
- Avoid broad field renames or wrapper-only churn that leaves the old ambiguity
  intact.

Completion check:

- Backend build proof passes.
- Focused record/helper tests prove the new schema/accessor accepts the same
  aggregate-lane shape and rejects the same malformed shapes.
- If public declarations changed, matching `test_before.log` and
  `test_after.log` exist.

### Step 3: Route Construction Through the Explicit Shape

Goal: have `calls.cpp` construct or validate aggregate-lane publication through
the explicit record shape while preserving AArch64 ABI ownership.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`.

Concrete actions:

- Update aggregate register-lane publication construction to use the new
  schema/accessor at the call-boundary record creation point.
- Keep `calls.cpp` responsible for ABI construction, selected-node facts,
  prepared source decisions, destination selection, and diagnostics.
- Preserve ordinary call-boundary move, immediate, f128, frame-slot, value
  publication, and preservation move construction.
- Keep existing rejection diagnostics and fail-closed paths for incomplete or
  inconsistent aggregate-lane authority.

Completion check:

- Backend build proof passes.
- Focused call-boundary owner and dispatch proof covers ordinary moves,
  immediate publication, f128 carrier cases, prepared frame-slot memory moves,
  aggregate register-lane publication, and aggregate-lane rejection paths.

### Step 4: Route Printing Through the Explicit Shape

Goal: have `machine_printer.cpp` consume the explicit aggregate-lane shape while
keeping final printer validation authoritative.

Primary target: `src/backend/mir/aarch64/codegen/machine_printer.cpp`.

Concrete actions:

- Update aggregate-lane print routing to consume the new schema/accessor.
- Keep printer-side presence checks, printable memory checks, scratch
  selection, materialized-address emission, and diagnostics.
- Keep ordinary call-boundary register, immediate, f128, and frame-slot printing
  behavior unchanged.
- Add or preserve rejection tests for malformed aggregate-lane records and
  malformed ordinary call-boundary records.

Completion check:

- Backend build proof passes.
- Focused machine-printer proof covers unchanged printed assembly and
  diagnostics for aggregate-lane publication and ordinary call-boundary moves.

### Step 5: Validate Surface and Acceptance

Goal: prove the cleanup clarified aggregate-lane record shape without changing
call-boundary behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- touched tests under `tests/backend/mir/`

Concrete actions:

- Audit the final public instruction-record surface for unnecessary wrappers,
  stale ambiguous helpers, and accidental broad call-boundary redesign.
- Verify `calls.cpp` still owns construction and prepared-source authority.
- Verify `machine_printer.cpp` still owns final printable assembly constraints
  and diagnostics.
- Check that ordinary call-boundary moves, value/preservation moves, immediate
  publication, f128 cases, prepared frame-slot memory, aggregate register-lane
  publication, and rejection paths all have focused proof.

Completion check:

- Backend build proof passes.
- Focused AArch64 call-boundary, record, printer, and dispatch proof passes.
- Matching `test_before.log` and `test_after.log` exist if record schema,
  helper predicates, public instruction declarations, or printer dispatch
  contracts changed.
- Reviewer can trace every implementation change back to the source idea
  without seeing output, diagnostic, or classification drift.
