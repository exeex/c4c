# RV64 Object Route Prepared Move-Bundle Target Shapes Runbook

Status: Active
Source Idea: ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md

## Purpose

Repair the next RV64 object-route blocker after scalar compare/trunc lowering:
prepared move bundles whose source and destination homes are supported scalar
integer locations, but whose target shape is still rejected by object emission.

## Goal

Admit the first semantic prepared move-bundle target shape needed by
`src/20000217-1.c`, preserve fail-closed diagnostics for unsupported adjacent
shapes, and rerun the representative to either pass or expose the next owner.

## Core Rule

Use prepared move metadata as the contract. Do not infer behavior from testcase
names, hard-coded value ids, source syntax, instruction indexes, or the exact
`src/20000217-1.c` expression.

## Read First

- `ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md`
- `build/agent_state/375_step3_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Rejected representative: `tests/c/external/gcc_torture/src/20000217-1.c`
- Current diagnostic:
  `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves`
- Narrow backend coverage:
  `backend_prepare_frame_stack_call_contract`,
  `backend_prepared_printer`, and
  `backend_riscv_object_emission`

## Non-Goals

- Do not reopen scalar compare/trunc lowering from idea 375.
- Do not implement frame-slot address call-argument materialization, pointer
  local-memory loads/stores, frame-slot payload call arguments, non-register
  parameter homes, aggregate `va_arg`, byval aggregate homes, terminator
  lowering, CFG reconstruction, globals, strings, or data-section support.
- Do not weaken diagnostics, allowlists, or unsupported expectations and claim
  capability progress.
- Do not add testcase-shaped or named-case shortcuts.

## Working Model

The representative has already advanced past compare/trunc materialization. The
remaining owner is the prepared move-bundle target-shape gate in RV64 object
emission. The implementation should admit a small semantic move shape only when
the prepared bundle gives explicit supported scalar integer source and
destination homes. Unsupported widths, homes, phases, or ambiguous bundles must
continue to fail closed with precise diagnostics.

## Execution Rules

- Keep each code step focused on one admitted prepared move shape or one
  diagnostic refinement.
- Add or update focused backend tests before treating an admitted shape as done.
- Preserve existing prepared-contract tests unless the source idea explicitly
  requires a contract change.
- Write routine packet progress and proof details into `todo.md`, not this
  plan.
- Rerun the representative after the focused backend proof. If it advances to a
  distinct out-of-scope failure, record that owner instead of expanding this
  plan.

## Step 1: Audit Rejected Move Bundle

Goal: Identify the first supportable prepared move-bundle target shape from the
post-idea-375 representative failure.

Primary target:
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`

Actions:
- Inspect the rejected prepared move bundle and collect source home,
  destination home, width, phase, and bundle cardinality facts.
- Confirm the rejection is downstream of the idea 375 compare/trunc repair.
- Determine whether the first rejected shape is a semantic scalar integer move
  between supported homes or an out-of-scope shape.
- Record the selected shape and adjacent unsupported shapes in `todo.md`.

Completion check:
- `todo.md` names one concrete prepared move shape for implementation, or names
  the out-of-scope owner if no in-scope shape is present.

## Step 2: Add Focused Move-Shape Tests

Goal: Lock the admitted prepared move target shape and nearby fail-closed
contracts before or alongside implementation.

Primary target:
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:
- Add a positive object-emission test for the selected prepared move shape.
- Add nearby negative tests for unsupported homes, widths, phases, ambiguous
  bundles, or target shapes that must remain rejected.
- Keep tests based on prepared move metadata, not the gcc-torture testcase name.

Completion check:
- The focused tests fail for the missing capability or pass only after a
  semantic implementation, and rejected adjacent shapes retain precise
  diagnostics.

## Step 3: Implement Semantic RV64 Move Emission

Goal: Emit RV64 object code for the selected scalar integer prepared move shape.

Primary target:
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:
- Extend the prepared move-bundle emission path for the selected supported
  source and destination homes.
- Use existing RV64 emission helpers and prepared location data where possible.
- Keep unsupported sources, destinations, widths, phases, and ambiguous bundles
  on explicit fail-closed paths.
- Avoid broad rewrites outside the object-route move-bundle gate.

Completion check:
- Focused object-emission tests prove the admitted move shape and adjacent
  rejected shapes.

## Step 4: Validate Focused Backend Coverage

Goal: Prove the move-shape change does not break the nearby prepared/object
emission contracts.

Proof command:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Completion check:
- The command succeeds and `test_after.log` is the canonical proof log for the
  executor packet.

## Step 5: Rerun Representative

Goal: Check whether `src/20000217-1.c` now passes the RV64 object route or
advances to the next distinct owner.

Proof command:

```sh
ALLOWLIST=build/agent_state/376_step5_20000217-1.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/376_step5_20000217-1.runner.log 2>&1
```

Actions:
- Create the allowlist with only `src/20000217-1.c`.
- Inspect both the runner log and the case log after the run.
- If the representative advances to a distinct out-of-scope blocker, record the
  owner candidate in `todo.md` for supervisor/plan-owner handoff.

Completion check:
- `src/20000217-1.c` passes, or the next blocker is documented as distinct from
  prepared move-bundle target-shape support.

## Step 6: Closure Or Handoff Check

Goal: Decide whether idea 376 is complete under its source acceptance criteria
or whether another in-scope move-bundle packet is required.

Actions:
- Compare the implementation, focused tests, representative run, and diagnostics
  against the source idea acceptance criteria.
- If another in-scope prepared move-bundle target shape remains, keep execution
  under this plan and update `todo.md` with the next packet.
- If only an out-of-scope blocker remains, ask the supervisor to route closure
  and follow-up idea creation.

Completion check:
- `todo.md` clearly states either the next in-scope executor packet or that
  source-idea closure/handoff is ready.
