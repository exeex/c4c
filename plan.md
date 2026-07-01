# RV64 Stack-To-Stack Prepared Move Materialization Runbook

Status: Active
Source Idea: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md

## Purpose

Materialize RV64 stack-to-stack prepared move bundles only when producer facts
explicitly publish coherent source and destination stack homes.

## Goal

Teach RV64 object emission to consume prepared move-bundle authority for the
owned stack-to-stack scalar move family without reconstructing storage from
testcase shape, ABI indexes, or instruction order.

## Core Rule

Consume explicit prepared move facts. Do not infer stack source or destination
locations from filenames, local names, source order, argument indexes, ABI
folklore, or textual instruction matching.

## Read First

- ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
- ideas/closed/512_stack_passed_parameter_home_publication.md
- build/agent_state/512_step7_remaining_rows/
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/mir/riscv/
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/

## Current Targets

- Primary rows: `src/20010518-1.c`, `src/pr27073.c`, `src/pr69447.c`
- Current rejection: `unsupported_move_bundle_target_shape`
- Owning layer: RV64 object emission consuming prepared move-bundle authority
- Required authority:
  - prepared move bundle identifies the value movement to emit
  - source home is an explicit prepared stack location
  - destination home is an explicit prepared stack location
  - width, alignment, and storage class are coherent and supported by the RV64
    lowering sequence

## Non-Goals

- Do not reopen stack-passed parameter-home publication from idea 512.
- Do not infer move sources or destinations from testcase names, source
  shapes, raw instruction order, argument indexes, ABI formulas, local names,
  or BIR text.
- Do not admit broad move-bundle support unrelated to the owned stack-to-stack
  scalar value move family.
- Do not combine this work with varargs, F128, aggregate ABI, dynamic stack,
  callee-saved FPR frame emission, or unrelated RV64 lowering repairs.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, or pass/fail accounting as proof of progress.

## Working Model

- Idea 512 made the relevant stack-parameter homes explicit and consumable.
- The remaining rows now reach a separate RV64 move-materialization gate:
  stack source and stack destination are both prepared facts, but the current
  object route cannot lower that move shape.
- A valid implementation should recognize a small semantic move shape from
  prepared bundle/home records and emit an RV64 load/store sequence through an
  appropriate scratch register or existing helper path.
- Missing, contradictory, oversized, memory-class, F128, aggregate, or
  unsupported facts should remain fail-closed with precise diagnostics.

## Execution Rules

- Start by reproducing the three listed rows and recording the exact prepared
  move bundle, source home, destination home, width, alignment, and rejection
  site in `todo.md`.
- Keep implementation in RV64 object emission consumption. If producer facts
  are missing or incoherent, stop and record that as a separate producer-owned
  initiative instead of inferring in RV64.
- Add focused backend coverage for one accepted stack-to-stack materialization
  and adjacent malformed/missing-authority reject cases.
- For code-changing steps, run a fresh build, focused representative proof,
  focused backend coverage, and the relevant backend subset chosen by the
  supervisor.

## Step 1: Rebuild Stack-To-Stack Move Evidence

Goal: Reconfirm the current representative failures and capture the prepared
facts that RV64 should consume.

Actions:

- Inspect `build/agent_state/512_step7_remaining_rows/` and rebuild focused
  dumps for `src/20010518-1.c`, `src/pr27073.c`, and `src/pr69447.c` if the
  artifacts are stale or incomplete.
- Record the prepared move bundle, source home, destination home, width,
  alignment, storage class, and current RV64 rejection point for each row.
- Identify which row is the smallest semantic representative for initial
  implementation and which rows are follow-up confirmation cases.
- Confirm that none of the rows still fails at `unsupported_param_home`.

Completion check:

- `todo.md` records the stack-to-stack prepared move facts, the exact
  `unsupported_move_bundle_target_shape` site, and any missing producer
  authority that would block RV64-only work.

## Step 2: Trace RV64 Move-Bundle Consumption

Goal: Locate the narrow RV64 object-emission path that should materialize
coherent prepared stack-to-stack scalar moves.

Actions:

- Trace the current move-bundle lowering path in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and neighboring RV64
  helpers.
- Identify the existing accepted source and target shapes, scratch register
  policy, width-specific load/store helpers, and diagnostics for unsupported
  move bundles.
- Define the minimal accepted stack-to-stack shape in terms of prepared
  bundle/home records, not representative names.
- Define malformed adjacent cases that must remain rejected, including missing
  source home, missing destination home, contradictory widths, unsupported
  memory class, F128, aggregates, dynamic stack, and unsupported alignment.

Completion check:

- `todo.md` names the implementation helper boundary, the accepted semantic
  predicate, the planned load/store sequence, and the fail-closed variants.

## Step 3: Materialize Coherent Stack-To-Stack Moves

Goal: Emit RV64 code for the owned stack-to-stack scalar move family using only
prepared move-bundle and stack-home authority.

Actions:

- Add a narrow RV64 materialization path for coherent stack-source to
  stack-destination prepared moves.
- Use an appropriate scratch register or existing helper sequence for the
  supported width and class.
- Preserve all existing accepted move shapes and diagnostics.
- Keep malformed, missing, ambiguous, memory-class, F128, aggregate, dynamic,
  and unsupported-width cases fail-closed.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused representative proof advances past
  `unsupported_move_bundle_target_shape` for the owned stack-to-stack move
  family.
- `todo.md` records the materialization boundary and remaining unsupported
  shapes.

## Step 4: Add Focused Materialization Coverage

Goal: Cover accepted and rejected stack-to-stack prepared move shapes without
depending on gcc_torture row names.

Actions:

- Add or extend focused backend object-emission tests for a coherent scalar
  stack-to-stack move.
- Add adjacent reject coverage for missing or malformed prepared move/source/
  destination facts.
- Ensure tests assert semantic facts and diagnostics rather than expectation
  rewrites or scan accounting.
- Keep the coverage narrow to the owned move family.

Completion check:

- Focused backend tests pass.
- Reject tests prove malformed adjacent cases still fail closed.
- `git diff --check` passes for touched files.

## Step 5: Reclassify Representative Rows

Goal: Verify the three source rows now clear the owned move-bundle gate or
identify any separate follow-up blockers.

Actions:

- Rerun focused dumps and object probes for `src/20010518-1.c`,
  `src/pr27073.c`, and `src/pr69447.c`.
- Confirm whether each row advances past the old
  `unsupported_move_bundle_target_shape` failure for the owned stack-to-stack
  move family.
- Record any new first-failure owner as a separate potential follow-up instead
  of expanding this runbook.
- Preserve evidence that the route still consumes prepared authority and does
  not infer stack locations.

Completion check:

- `todo.md` records the post-fix first failure or success state for all three
  rows and names any separate follow-up idea needed outside 513 scope.

## Step 6: Validate And Handoff

Goal: Prove the slice is stable enough for closure or for a clearly bounded
follow-up.

Actions:

- Run the build and focused tests required by the supervisor.
- Run the relevant backend subset for the touched RV64 object-emission scope.
- Run regression guard when the supervisor asks for close readiness.
- Update `todo.md` with validation results, residual risks, and whether the
  source idea acceptance criteria are satisfied.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The runbook can be closed, or any remaining work is explicitly separated
  from the stack-to-stack prepared move materialization scope.
