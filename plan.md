# RV64 FPR Callee-Saved Frame Slots Runbook

Status: Active
Source Idea: ideas/open/564_rv64_fpr_callee_saved_frame_slots.md

## Purpose

Teach the RV64 object route to consume prepared non-GPR callee-saved frame
slots for FPR saves/restores when prepared frame facts already publish the
slot.

## Goal

Move `src/20000603-1.c` and `src/20030209-1.c` off the current non-GPR
callee-saved save-slot rejection for the prepared `fpr:fs1` shape, without
fabricating frame layout or callee-saved facts in RV64 lowering.

## Core Rule

RV64 may consume prepared FPR callee-saved frame-slot facts that already exist.
It must not synthesize missing prepared frame layout, weaken unsupported
diagnostics, alter expected outputs, or treat filename/register-shaped handling
as progress.

## Read First

- `ideas/open/564_rv64_fpr_callee_saved_frame_slots.md`
- `build/agent_state/548_step3_stack_frame_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20000603-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20030209-1.c/case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- Existing backend tests for RV64 stack-frame and object emission

## Scope

- RV64 frame setup/access/teardown consumption for prepared FPR callee-saved
  save slots.
- The representative `fpr:fs1` slot shape in `src/20000603-1.c` and
  `src/20030209-1.c`.
- Focused backend or target coverage that proves prepared non-GPR frame-slot
  consumption.
- Residual-owner classification if the representatives advance to later
  failures.

## Non-Goals

- Do not change prepared frame-layout production or stack-slot publication.
- Do not rewrite ordinary GPR frame handling unless a focused regression test
  proves the FPR repair shares the same helper.
- Do not touch global-data object routing, F64 global-memory consumption,
  F128 quarantine policy, or scalar/FPR residual salvage.
- Do not special-case `src/20000603-1.c`, `src/20030209-1.c`, `fs1`, or the
  current diagnostic text.
- Do not change unsupported markers, allowlists, expected outputs, or pass/fail
  accounting as proof.

## Working Model

Prepared stack-frame facts are coherent enough to expose an FPR callee-saved
slot, but RV64 object emission currently rejects non-GPR prepared callee-saved
register save slots. The expected repair is target-consumer work: add explicit
FPR save/restore materialization for prepared callee-saved frame slots while
preserving existing GPR behavior and fail-closed handling for malformed or
missing prepared facts.

## Execution Rules

- Start by reconfirming the current representative diagnostics and the exact
  RV64 frame gate that rejects `fpr:fs1`.
- Add focused coverage before or with the repair so the prepared FPR slot
  contract is directly tested.
- Keep prepared producer changes out of scope unless inspection proves the
  recorded evidence is stale and the prepared slot is no longer published.
- Preserve GPR callee-saved frame behavior and malformed-frame fail-closed
  behavior.
- Record proof commands, log paths, and any downstream residual owner in
  `todo.md`.

## Steps

### Step 1: Inspect FPR Callee-Saved Boundary

Goal: Reconfirm the current first bad facts for both representatives and
locate the RV64 object-route gate that rejects prepared `fpr:fs1` save slots.

Primary targets:

- `src/20000603-1.c`
- `src/20030209-1.c`

Actions:

- Re-run or inspect representative backend logs and capture the current
  `unsupported_stack_frame` evidence.
- Trace prepared frame facts into RV64 frame/object emission.
- Confirm whether prepared frame layout, callee-saved register identity, slot
  offset, and slot size are present before RV64 rejects the non-GPR slot.
- Record whether the boundary is still RV64 consumer-owned or has moved back to
  prepared frame production.

Completion check:

- `todo.md` records current diagnostics, log paths, owning code boundary, and a
  concrete next packet for focused coverage or repair.

### Step 2: Add Focused FPR Frame-Slot Coverage

Goal: Add tests that prove RV64 can identify and require prepared FPR
callee-saved frame-slot facts.

Actions:

- Add focused backend or target coverage for a prepared non-GPR callee-saved
  save slot using the `fpr:fs1`-style fact shape.
- Assert the prepared frame fact requirement remains explicit; missing or
  malformed prepared slots must still fail closed.
- Preserve existing GPR callee-saved frame coverage and behavior.

Completion check:

- Focused coverage fails before the repair or directly proves the repaired
  contract, backend proof is recorded in `todo.md`, and no expectations or
  unsupported markers are weakened.

### Step 3: Repair RV64 FPR Callee-Saved Slot Consumption

Goal: Extend RV64 frame lowering to save and restore prepared FPR
callee-saved slots while preserving prepared/RV64 ownership boundaries.

Actions:

- Implement the minimal RV64 target-consumer repair at the verified owner.
- Reuse existing prepared frame-slot facts and register-class information.
- Emit the appropriate FPR stack save/restore sequence for prepared FPR
  callee-saved slots.
- Do not synthesize missing prepared slots, callee-saved facts, or frame layout
  inside RV64 lowering.
- Run focused tests and the backend subset selected by the supervisor.

Completion check:

- Backend tests pass, focused FPR frame-slot coverage passes, and `todo.md`
  records the files changed plus proof commands.

### Step 4: Reconcile Representatives And Residual Owners

Goal: Prove whether both representatives moved off the non-GPR callee-saved
slot diagnostic and classify any later failure.

Actions:

- Re-run both representative allowlist rows with verbose failure logs.
- Confirm the old non-GPR callee-saved save-slot rejection no longer appears.
- If either row advances to a downstream owner, record the exact owner and keep
  distinct work in a separate source idea when needed.
- Recommend close, split, or continue for this source idea.

Completion check:

- `todo.md` records representative outcomes, log paths, residual owners if
  any, and the lifecycle recommendation for this active idea.
