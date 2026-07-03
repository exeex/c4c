# RV64 Large-Offset GPR Callee-Saved Frame Slots Runbook

Status: Active
Source Idea: ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md

## Purpose

Consume prepared GPR callee-saved frame-slot facts whose stack offsets are
outside the direct RV64 immediate-offset contract.

## Goal

Teach RV64 frame lowering to materialize validated large-offset GPR
callee-saved save/restore slots without fabricating prepared frame facts or
weakening malformed-frame fail-closed behavior.

## Core Rule

Use explicit prepared callee-saved register and frame-slot facts as the only
authority. Do not infer slots from testcase names, register names, raw stack
offsets, or target-shaped memory copies.

## Read First

- `ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md`
- `ideas/closed/564_rv64_fpr_callee_saved_frame_slots.md`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Representative row: `src/20030209-1.c`
- Known residual: prepared `gpr:s1 stack80000`
- Prior evidence: `build/agent_state/564_step3_fpr_callee_saved_after.log`
- Expected old diagnostic to remove from this lane:
  `unsupported_stack_frame: RV64 object route requires supported prepared
  callee-saved save slots`

## Non-Goals

- Do not revisit FPR callee-saved save/restore materialization.
- Do not repair prepared frame-layout production or stack-slot publication.
- Do not handle unrelated instruction-fragment residuals such as
  `src/20000603-1.c`.
- Do not authorize arbitrary memory-to-memory stack copies.
- Do not change expectations, unsupported markers, allowlists, or pass/fail
  accounting.

## Working Model

Prepared frame facts already identify the callee-saved GPR slot. The current
RV64 consumer accepts direct-offset GPR saved-register slots but rejects a
valid large-offset shape. The repair should add an explicit large-offset
addressing path for validated GPR saved-register save/restore materialization,
preserving the existing direct-offset path and malformed fact rejection.

## Execution Rules

- Keep Step 1 inspection-only unless the supervisor delegates a combined slice.
- Add backend coverage before or with any consumer repair.
- Preserve GPR direct-offset behavior in tests.
- Prove malformed or missing large-offset prepared facts still fail closed.
- Use fresh backend proof for code slices:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Run the representative allowlist probe after repair to classify any
  downstream residual separately from the large-offset GPR frame-slot owner.

## Step 1: Inspect Large-Offset GPR Boundary

Goal: Reconfirm the current representative failure and exact RV64 consumer
boundary before code changes.

Primary target:

- `src/20030209-1.c`

Actions:

- Reproduce the representative failure with an allowlist probe.
- Confirm the old owner is the GPR callee-saved large-offset save-slot path,
  not FPR materialization or prepared frame publication.
- Record the first bad diagnostic, prepared facts, and concrete functions that
  reject the shape.
- Update `todo.md` with the evidence and next recommended packet.

Completion check:

- `todo.md` names the first bad fact, owner boundary, proof command, proof
  result, and whether Step 2 should add coverage only or coverage plus minimal
  repair.

## Step 2: Add Focused Large-Offset GPR Coverage

Goal: Prove the desired prepared frame contract for large-offset GPR
callee-saved save/restore slots.

Primary targets:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- Adjacent RV64 prepared frame helpers only if needed to expose the contract
  under test.

Actions:

- Add focused positive coverage for a validated GPR callee-saved slot with a
  large stack offset such as `stack80000`.
- Assert the emitted save/restore path uses prepared register and slot facts.
- Keep or add coverage for direct-offset GPR saved-register behavior.
- Keep malformed or missing prepared facts fail-closed.
- Avoid filename-shaped or register-name-only handling.

Completion check:

- Backend tests pass.
- `todo.md` records whether the implementation is still missing or was
  minimally repaired as part of the coverage slice.

## Step 3: Repair Large-Offset GPR Save/Restore Materialization

Goal: Consume validated large-offset GPR callee-saved frame-slot facts in RV64
prologue and epilogue emission.

Primary targets:

- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- Directly adjacent RV64 scalar/frame helpers if the existing return/prologue
  path requires plumbing.

Actions:

- Add or reuse an explicit large-offset address materialization helper for GPR
  saved-register frame slots.
- Emit GPR save and restore instructions through validated prepared slot facts.
- Preserve existing direct-offset GPR behavior.
- Preserve fail-closed handling for malformed frame facts and unsupported
  register banks or slot shapes.
- Do not synthesize prepared frame layout in RV64 lowering.

Completion check:

- Backend tests pass.
- The representative row no longer reports the old unsupported prepared
  callee-saved save-slot diagnostic.
- Any remaining representative failure is recorded with a distinct downstream
  owner.

## Step 4: Reconcile And Close Or Split

Goal: Decide whether the source idea is complete after the large-offset GPR
owner is gone.

Actions:

- Run the supervisor-selected representative probe for `src/20030209-1.c`.
- Run the regression guard against matching backend `test_before.log` and
  `test_after.log`.
- If a new downstream owner remains, record or create a separate idea instead
  of expanding this runbook.
- If the source idea acceptance criteria are satisfied, ask the plan owner to
  close the idea.

Completion check:

- The old large-offset GPR frame-slot rejection is gone, regression proof is
  monotonic, and any residual is either out of scope or captured in a separate
  idea.
