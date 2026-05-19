# AArch64 HFA Aggregate Argument Runtime Runbook

Status: Active
Source Idea: ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md
Activated from: idea 318 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` runtime blocker by localizing and fixing the AArch64
ABI argument path that corrupts early `Arguments:` HFA, floating,
long-double, or aggregate output before the scalar arithmetic section runs.

## Goal

Make the implicated argument class pass through AArch64 call and callee
lowering correctly, while preserving the assembler-blocker repairs from ideas
314, 315, 317, and 318.

## Core Rule

Progress must be a general AArch64 ABI argument handling capability. Do not
reopen scalar ALU immediate materialization, raw `va_start` helper lowering,
large frame adjustment materialization, stack-slot memory spelling,
frame-layout consistency, semantic admission, expectations, unsupported
classifications, runners, timeout policy, proof-log contents, or CTest
registration.

## Read First

- `ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md`
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
  - `00204.c` fails at runtime with `RUNTIME_NONZERO` / `Segmentation fault`
    during early `Arguments:` output.
  - Printed floating/HFA/aggregate values are corrupted before execution
    reaches `Return values:`, `stdarg:`, `MOVI:`, or `opi()`.
- Initial suspected owner family:
  - AArch64 ABI classification and argument lowering for HFA, floating,
    long-double, or aggregate arguments.
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

- Do not reopen idea 318's scalar ALU immediate materialization owner.
- Do not reopen idea 317's raw `va_start` helper-text lowering owner.
- Do not reopen idea 315's large frame setup and teardown materialization
  owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication
  spelling owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual unless
  localization proves the runtime corruption is the same ABI fault.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not rely on filenames, c-testsuite numbers, one printed output line, one
  function name, one register, or one stack slot.

## Working Model

The representative now gets past earlier assembler blockers and executes into
`pcs()`. The latest stdout reaches the `Arguments:` section, then prints
implausible floating and long-double values before crashing. Since `opi()` and
`subim503808` run later, the next first bad fact should be localized around
argument classification, argument marshaling, or callee-side argument
materialization for the earliest corrupted HFA, floating, long-double, or
aggregate value.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the earliest corrupted runtime value before editing code.
- Prefer structured ABI or backend tests for the implicated argument class
  before relying only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, variadic,
  frame, printer, or scalar ALU coverage to improve counts.
- If the representative advances to a later non-argument runtime mismatch,
  linker behavior, timeout, or another blocker, record the new first bad fact
  and return it to lifecycle classification unless generated-code evidence
  proves this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared ABI machinery broadly.

## Ordered Steps

### Step 1: Localize Earliest Argument Corruption

Goal: identify the first corrupted `Arguments:` value and the AArch64 ABI
classification or lowering path that owns it.

Primary target: generated `00204.c` artifacts and the AArch64 ABI argument
classification/lowering surfaces.

Actions:

- Compare expected `00204.c` `Arguments:` output with the observed runtime
  output to find the earliest mismatched value.
- Trace the corresponding function parameter through generated assembly,
  selected records, and call/callee lowering.
- Distinguish HFA, scalar floating, long-double, and aggregate argument paths.
- Record in `todo.md` the owning code surface, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the earliest corrupted argument fact, the owning code
  surface, representative tests, and the smallest focused proof command for
  the repair.

### Step 2: Repair The Owned Argument Path

Goal: fix the localized AArch64 argument path generally for the implicated ABI
class.

Primary target: code surface identified by Step 1.

Actions:

- Implement the narrow ABI argument repair for the localized class.
- Preserve unrelated argument classes and existing scalar/integer direct paths.
- Reuse existing structured ABI records and lowering helpers when available.
- Avoid variadic helper text, frame adjustment, stack-offset spelling, scalar
  ALU, runner, expectation, and timeout changes.

Completion check:

- Focused proof shows the original argument corruption is gone, or `todo.md`
  records the next first bad fact with evidence.

### Step 3: Add Focused ABI Coverage

Goal: make the repaired argument class observable in local backend or ABI
tests.

Primary target: existing AArch64 ABI, call-lowering, instruction dispatch, or
printer tests that already cover adjacent argument behavior.

Actions:

- Add or extend coverage for the implicated HFA, floating, long-double, or
  aggregate argument class.
- Preserve coverage for adjacent unaffected argument classes.
- Assert the ABI contract rather than one incidental register, stack slot, or
  output line unless the ABI requires that location.

Completion check:

- Local coverage exercises the repaired class and verifies adjacent behavior
  remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 ABI/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, and 318, and the `00204.c`
  c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if the same early argument corruption
  remains.

Completion check:

- `todo.md` records fresh proof. The current early `Arguments:` corruption is
  gone, or the remaining blocker is explicitly localized for the next packet.
