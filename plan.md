# AArch64 Aggregate Va Arg Helper Lowering Runbook

Status: Active
Source Idea: ideas/open/321_aarch64_aggregate_va_arg_helper_lowering.md
Activated from: idea 320 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` backend blocker by lowering AArch64 aggregate
`va_arg` helper machine records into executable assembly instead of printing
raw descriptive helper text.

## Goal

Make `PreparedVariadicEntryHelperKind::VaArgAggregate` /
`VariadicVaArgAggregate` produce executable AArch64 source selection,
aggregate copy, and `va_list` progression code while preserving prior backend
repairs.

## Core Rule

Progress must be a general AArch64 aggregate `va_arg` helper lowering
capability. Do not reopen F128 transport addressability, HFA argument ABI
lowering, scalar ALU immediate materialization, raw `va_start` helper
lowering, frame adjustment materialization, stack-slot memory spelling,
frame-layout consistency, semantic admission, expectations, unsupported
classifications, runners, timeout policy, proof-log contents, or CTest
registration.

## Read First

- `ideas/open/321_aarch64_aggregate_va_arg_helper_lowering.md`
- `ideas/closed/320_aarch64_f128_transport_addressability.md`
- `ideas/closed/319_aarch64_hfa_aggregate_argument_runtime.md`
- `ideas/closed/318_aarch64_scalar_alu_immediate_materialization.md`
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
  - generated assembly contains un-commented `va.arg.aggregate`,
    `va.arg.aggregate.source`, and `va.arg.aggregate.progress` lines that the
    assembler rejects as unexpected tokens.
- Initial suspected owner family:
  - `src/backend/mir/aarch64/codegen/variadic.cpp`
  - `PreparedVariadicEntryHelperKind::VaArgAggregate`
  - `MachinePrinterMnemonicKind::VariadicVaArgAggregate`
  - aggregate copy/address helpers used by AArch64 machine printing
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

- Do not reopen idea 320's F128 transport addressability owner.
- Do not reopen idea 319's HFA, floating, long-double, or aggregate ABI
  argument classification and lowering owner.
- Do not reopen idea 318's scalar ALU immediate materialization owner.
- Do not reopen idea 317's raw `va_start` helper-text lowering owner.
- Do not reopen idea 315's large frame setup and teardown materialization
  owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication
  spelling owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual unless
  generated evidence proves it is the same aggregate `va_arg` lowering fault.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not fix global initializer emission unless it becomes the next first bad
  fact after aggregate `va_arg` helper lowering is repaired.

## Working Model

The representative now gets past prior assembler blockers, HFA/aggregate
argument ABI corruption, scalar immediate materialization, raw `va_start`
text, large stack/frame materialization, and F128 transport addressability.
The next first bad fact occurs when the AArch64 variadic helper printer emits
aggregate `va_arg` records as descriptive text. The repair should replace
those records with executable AArch64 code that selects the correct source,
copies the aggregate object, and advances the `va_list` state.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact aggregate helper records and required source/progress
  semantics before editing code.
- Prefer focused backend coverage for the helper output before relying only on
  the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, HFA ABI,
  F128 transport, frame, printer, scalar ALU, runner, or expectation coverage.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize Aggregate Va Arg Helper Records

Goal: identify the exact helper records, data fields, and AArch64 printer
surface that currently emit raw `va.arg.aggregate` text.

Primary target: generated `00204.c` artifacts and
`src/backend/mir/aarch64/codegen/variadic.cpp`.

Actions:

- Trace the raw `va.arg.aggregate` lines from generated assembly back to the
  prepared variadic helper entries and machine mnemonic.
- Distinguish source selection, aggregate copy, and `va_list` progression
  responsibilities.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the raw helper record shapes, owning code surfaces,
  representative tests, and smallest focused proof command.

### Step 2: Lower Aggregate Va Arg Helper Output

Goal: replace raw aggregate `va_arg` helper text with executable AArch64 code.

Primary target: code surface identified by Step 1.

Actions:

- Implement the narrow aggregate helper lowering for source selection,
  aggregate copy, and progression.
- Reuse existing AArch64 memory, copy, scratch, and address materialization
  helpers when available.
- Preserve unrelated F128 transport, argument ABI, scalar ALU, frame, runner,
  expectation, and timeout behavior.

Completion check:

- Focused proof shows raw `va.arg.aggregate` helper text is gone from emitted
  assembly, or `todo.md` records the next first bad fact with evidence.

### Step 3: Add Focused Aggregate Va Arg Coverage

Goal: make aggregate `va_arg` helper lowering observable in local backend
tests.

Primary target: existing AArch64 machine-printer, variadic, instruction
dispatch, or prepared-BIR tests that already cover adjacent variadic helper
behavior.

Actions:

- Add or extend coverage for aggregate `va_arg` helper output.
- Assert executable AArch64 output and absence of raw helper mnemonics.
- Preserve adjacent scalar, HFA, `va_start`, and ordinary memory helper
  behavior.

Completion check:

- Local coverage exercises aggregate `va_arg` helper lowering and verifies
  adjacent behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 helper/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, 318, 319, and 320, and the
  `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if raw aggregate `va_arg` helper text
  remains in emitted assembly.

Completion check:

- `todo.md` records fresh proof. The current raw `va.arg.aggregate` helper
  text blocker is gone, or the remaining blocker is explicitly localized for
  the next packet.
