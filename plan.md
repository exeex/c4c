# RV64 Prepared Callee-Saved GPR Save-Slot Emission Runbook

Status: Active
Source Idea: ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md

## Purpose

Repair the RV64 object-emission consumer path for coherent prepared
callee-saved GPR save-slot facts.

## Goal

Teach RV64 object emission to lower explicit prepared callee-saved GPR save and
restore slots without reconstructing frame facts from source shape, testcase
identity, or raw stack layout guesses.

## Core Rule

Consume prepared saved-register slot authority directly. If the prepared facts
are missing, contradictory, non-GPR, or otherwise unsupported, keep the route
fail-closed with a diagnostic that identifies the unsupported fact family.

## Read First

- ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md
- docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
- build/agent_state/424_step2_infrastructure_classification/20000603-1/

## Current Targets

- Representative row: `src/20000603-1.c`
- Failure bucket: `unsupported_stack_frame`
- Owning layer: RV64 object emission
- Known prepared facts:
  - `f` publishes saved `gpr:s1` at `slot#10+stack16`, size/alignment `8`
  - `main` publishes saved `gpr:s1` at `slot#10+stack8`, size/alignment `8`

## Non-Goals

- Do not repair or invent producer saved-register facts.
- Do not implement FPR, F128, vector, dynamic-stack, varargs, or broad ABI
  save-slot support.
- Do not turn this into general fixed prepared stack-frame support beyond the
  addressing needed for coherent GPR save slots.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, scan accounting, or default test contracts.

## Working Model

- The prepared contract already owns frame size, alignment, saved register,
  slot, offset, width, and alignment for the target GPR save slots.
- RV64 object emission should use those facts to emit prologue stores and
  epilogue reloads/restores for callee-saved GPRs.
- Unsupported variants should remain rejected before emission rather than being
  guessed in the target backend.

## Execution Rules

- Keep implementation changes in RV64 object emission and nearby prepared fact
  helpers only when they are required to consume existing facts.
- Prefer narrow helper extraction only when it makes save-slot validation or
  emission reusable and testable.
- Add ordinary-C coverage that proves behavior through a nontrivial
  callee-saved GPR path, not through a named gcc_torture shortcut.
- For code-changing steps, use a build proof, focused representative or unit
  proof, focused coverage proof, and an appropriate `^backend_` subset.

## Step 1: Rebuild Save-Slot Evidence

Goal: Reconfirm the representative failure and the prepared fact contract before
editing code.

Actions:

- Inspect the idea 424 handoff docs and the
  `build/agent_state/424_step2_infrastructure_classification/20000603-1/`
  artifacts.
- Run or reuse focused dumps for `src/20000603-1.c` as needed to show prepared
  saved-register slot facts and the current RV64 object-route rejection.
- Record the fact shape, rejection point, and proof artifacts in `todo.md`.

Completion check:

- `todo.md` identifies the prepared saved-register facts, the current
  unsupported diagnostic, and the exact code path that rejects them.

## Step 2: Trace RV64 Save-Slot Consumption

Goal: Locate the minimal RV64 object-emission path that should validate and
emit coherent GPR save slots.

Actions:

- Trace prepared frame and saved-register data from prepared output into MIR or
  object-emission lowering.
- Identify existing stack-pointer adjustment and frame-slot addressing helpers
  that should be reused.
- Identify unsupported fact variants that must continue to fail closed.
- Record the intended helper boundaries and blast radius in `todo.md`.

Completion check:

- `todo.md` names the producer fact path, the RV64 consumer entry point, the
  helpers to reuse or add, and the unsupported variants that remain rejected.

## Step 3: Implement GPR Save-Slot Emission

Goal: Emit prologue save and epilogue restore code for coherent prepared
callee-saved GPR slots.

Actions:

- Add RV64 object-emission validation for prepared callee-saved GPR save-slot
  facts with explicit register, slot, offset, size, and alignment authority.
- Emit prologue stores and epilogue reloads/restores using prepared stack
  offsets and existing RV64 stack/addressing helpers.
- Keep non-GPR, incomplete, contradictory, dynamic, or otherwise unsupported
  facts fail-closed.
- Add or update focused object-emission tests for the prepared contract and
  emitted instruction shape where appropriate.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused representative proof advances past the old
  `requires supported prepared callee-saved GPR save slots` rejection.
- Focused object-emission tests pass if they were added or changed.
- `todo.md` records proof commands and artifacts.

## Step 4: Add Ordinary-C Coverage

Goal: Prove the repaired route with nearby ordinary-C backend coverage.

Actions:

- Add a focused ordinary-C case that requires a nontrivial callee-saved GPR
  save and restore path.
- Register the object-route backend test needed to exercise the repaired
  consumer path.
- Do not rely on `src/20000603-1.c`, function names, raw slot numbers, or one
  exact frame shape as the only proof.

Completion check:

- The new focused backend test passes.
- The focused representative route remains advanced past the repaired
  save-slot failure.
- `git diff --check` passes for touched files.

## Step 5: Validate And Handoff

Goal: Prove the slice and prepare the source idea for closure or any necessary
follow-up split.

Actions:

- Run the build, focused representative proof, focused coverage proof, and
  `ctest --test-dir build -j --output-on-failure -R "^backend_"`.
- Confirm unsupported variants still fail closed and no expectation or
  allowlist downgrade was used as progress.
- Record any remaining non-goal failures, such as broad fixed-frame, FPR, F128,
  or dynamic-stack support, in `todo.md`.

Completion check:

- Backend validation passes.
- `todo.md` contains the final proof summary and closure recommendation.
