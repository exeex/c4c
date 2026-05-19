# AArch64 Scalar ALU Immediate Materialization Runbook

Status: Active
Source Idea: ideas/open/318_aarch64_scalar_alu_immediate_materialization.md
Activated from: idea 317 closure after Step 4 residual classification

## Purpose

Repair AArch64 scalar ALU immediate paths so non-encodable constants are
materialized through legal instruction sequences before arithmetic operations.

## Goal

Make scalar ALU helpers handle constants such as `503808` without emitting
illegal single-instruction `mov` forms, while preserving direct encodable
immediate and register-register paths.

## Core Rule

Progress must be a general scalar ALU immediate materialization capability. Do
not reopen variadic `va_start` helper lowering, large frame adjustment
materialization, stack-slot memory spelling, frame-layout consistency,
semantic admission, expectations, unsupported classifications, runners,
timeout policy, proof-log contents, or CTest registration.

## Read First

- `ideas/open/318_aarch64_scalar_alu_immediate_materialization.md`
- `ideas/closed/317_aarch64_variadic_va_start_helper_lowering.md`
- `ideas/closed/315_aarch64_large_frame_adjustment_materialization.md`
- `ideas/closed/314_aarch64_large_stack_offset_addressing.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_before.log` / `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current residual fact:
  - `00204.c`: generated assembly contains illegal scalar ALU materialization
    in `subim503808`: `mov w9, #503808`.
- Suspected owner surface:
  - `src/backend/mir/aarch64/codegen/alu.cpp`
- Prior-owner guardrails:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
  - `backend_cli_dump_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_block_entry_00204`
  - `backend_aarch64_machine_printer`
  - `backend_aarch64_instruction_dispatch`
  - `backend_aarch64_target_instruction_records`

## Non-Goals

- Do not reopen idea 317's raw `va_start` helper-text lowering owner.
- Do not reopen idea 315's large frame setup and teardown materialization
  owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication
  spelling owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not rely on filenames, function names, literal `503808`, diagnostics,
  scratch-register names, or c-testsuite numbers.

## Working Model

Idea 317 moved `00204.c` past raw variadic helper text. The next assembler
blocker is an illegal scalar constant materialization in arithmetic helper
code. The owner should route non-encodable constants through the target's
legal integer materialization sequence before the ALU operation, while keeping
encodable immediates and register-register operations on their existing direct
paths.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact scalar ALU path before editing code.
- Prefer local backend coverage for non-encodable and encodable ALU constants
  before relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, variadic,
  frame, or printer coverage to improve counts.
- If the representative advances to frame-layout consistency, runtime
  mismatch, linker behavior, timeout, or another non-ALU blocker, record the
  new first bad fact and return it to lifecycle classification unless
  generated-code evidence proves this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared target-machine behavior broadly.

## Ordered Steps

### Step 1: Localize Scalar ALU Immediate Path

Goal: identify the concrete code path that emits illegal single-instruction
materialization for non-encodable scalar ALU constants.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`.

Actions:

- Inspect generated `00204.c` assembly around `subim503808` and any nearby
  selected records or printer traces that identify the source operation.
- Inspect scalar ALU helper paths enough to distinguish encodable immediates,
  register-register operations, and scratch-register constant publication.
- Compare existing machine-printer, dispatch, and target-instruction coverage
  against the missing non-encodable constant contract.
- Record in `todo.md` the owning code surface, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the missing scalar ALU immediate materialization fact, the
  owning code surface, representative tests, and the smallest focused proof
  command for the repair.

### Step 2: Materialize Non-Encodable ALU Constants

Goal: route scalar ALU constants that cannot be printed as direct immediates
through legal AArch64 materialization sequences before arithmetic use.

Primary target: code surface identified by Step 1.

Actions:

- Implement a narrow materialization path for non-encodable scalar ALU
  constants.
- Preserve direct encodable immediate and register-register paths.
- Reuse existing target constant materialization helpers when available.
- Avoid variadic helper, frame adjustment, frame-layout, runner, expectation,
  and timeout changes.

Completion check:

- Focused backend proof shows illegal scalar ALU immediates such as
  `mov w9, #503808` no longer reach generated assembly, or `todo.md` records
  the next first bad fact with evidence.

### Step 3: Add Focused Backend Coverage

Goal: make scalar ALU immediate materialization observable in local backend
tests.

Primary target: existing AArch64 ALU, printer, dispatch, or target-instruction
tests that already cover adjacent scalar arithmetic behavior.

Actions:

- Add or extend coverage for at least one non-encodable 32-bit scalar ALU
  constant.
- Preserve coverage proving encodable immediates remain on the direct path.
- Assert the materialization contract rather than one incidental generated
  line, scratch register, or function name.

Completion check:

- Local backend coverage exercises both non-encodable materialization and
  preservation of existing direct behavior.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails from ideas 314, 315, and 317, and the `00204.c`
  c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if illegal scalar ALU immediates still
  reach generated assembly.

Completion check:

- `todo.md` records fresh proof. The current illegal `mov w9, #503808`
  assembler diagnostic is gone, or the remaining blocker is explicitly
  localized for the next packet.
