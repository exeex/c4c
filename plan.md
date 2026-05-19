# AArch64 Variadic HFA And Floating Residual Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Activated from: idea 325 closure after HFA/floating residual classification

## Purpose

Repair the next `00204.c` runtime blocker by classifying and fixing the
AArch64 variadic HFA/floating argument corruption that remains after
local/value-home publication repairs.

## Goal

Make generated AArch64 variadic HFA/floating argument paths preserve the
expected runtime values through call-boundary transport, register-save-area or
overflow selection, lane materialization, and consumer reads.

## Core Rule

Progress must be a general AArch64 HFA/floating variadic capability. Do not
reopen local/value-home publication, frame/formal publication,
aggregate/floating `va_arg`, `va_start` destination publication, aggregate
helper text lowering, F128 transport addressability, scalar ALU immediate
materialization, frame adjustment materialization, stack-slot memory spelling,
semantic admission, expectations, unsupported classifications, runners,
timeout policy, proof-log contents, or CTest registration unless generated-code
evidence proves that surface owns the current HFA/floating first bad fact.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `ideas/closed/325_aarch64_variadic_local_value_home_publication.md`
- `ideas/closed/324_aarch64_variadic_frame_formal_publication.md`
- `ideas/closed/323_aarch64_vararg_consumption_source_progression.md`
- `ideas/closed/322_aarch64_va_start_destination_address_materialization.md`
- `ideas/closed/321_aarch64_aggregate_va_arg_helper_lowering.md`
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
  - after fixed-size string argument cases, `fa_hfa11(hfa11)` prints `0.0`
    instead of `11.1`, followed by corrupted floating/HFA output and a later
    segmentation fault.
- Initial suspected owner family:
  - HFA argument call-lane lowering
  - aggregate/floating `va_arg` source selection and progression
  - floating register-save-area or overflow-area addressing
  - HFA lane materialization and call-boundary transport
  - generated consumer reads for HFA/floating values
- Prior-owner guardrails:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  - `backend_cli_dump_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_block_entry_00204`
  - `backend_aarch64_machine_printer`
  - `backend_aarch64_instruction_dispatch`
  - `backend_aarch64_target_instruction_records`

## Non-Goals

- Do not reopen idea 325's local/value-home publication owner unless generated
  evidence again shows an unpublished ordinary local, constant, pattern
  operand, branch condition, call operand, or predecessor/join source.
- Do not reopen idea 324's frame-size coverage or fixed-formal publication
  owner unless generated evidence again shows uncovered stack references or
  clobbered fixed formals.
- Do not reopen idea 322's `va_start` destination address publication owner.
- Do not reopen idea 321's aggregate `va_arg` helper text lowering owner.
- Do not reopen idea 320's F128 transport addressability owner.
- Do not reopen idea 318's scalar ALU immediate materialization owner.
- Do not reopen idea 315's large frame setup and teardown materialization
  owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication
  spelling owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual unless
  generated evidence proves it is the same vararg consumer fault.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not fix global initializer emission or unrelated runtime mismatches unless
  they become the next first bad fact after HFA/floating correctness is
  repaired.

## Working Model

The representative now gets past prior assembler blockers, scalar immediate
materialization, raw helper text, large stack/frame materialization, F128
transport addressability, aggregate `va_arg` helper lowering, `va_start`
destination publication, frame/formal publication, and local/value-home
publication. The next first bad fact is HFA/floating value corruption:
`fa_hfa11(hfa11)` prints `0.0` instead of `11.1`. The repair should first
prove which generated-code owner loses or misroutes that value, then fix that
rule generally without weakening prior guardrails.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact HFA/floating value, lane, source area, prepared record,
  call-boundary move, and emitted stack/register sequence before editing code.
- Prefer focused backend coverage for HFA/floating transport before relying
  only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff,
  frame/formal publication, `va_start` destination publication, aggregate
  helper lowering, F128 transport, aggregate `va_arg` source/progression,
  scalar ALU, local/value-home publication, runner, or expectation coverage.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize HFA/Floating First Bad Fact

Goal: identify the exact HFA/floating value path that produces `0.0` instead
of `11.1` for `fa_hfa11(hfa11)`.

Primary target: generated `00204.c` artifacts and AArch64 HFA/floating
prepared records, call-lane, `va_arg`, register-save-area, overflow-area, and
consumer surfaces.

Actions:

- Trace where the expected `11.1` HFA lane is passed, saved, selected,
  materialized, and consumed.
- Map the first corrupted emitted value back to prepared records, machine
  records, dispatch paths, helper state, and printer output.
- Distinguish HFA call-lane lowering, aggregate/floating `va_arg` selection or
  progression, register-save-area addressing, overflow-area addressing, lane
  materialization, and unrelated local publication responsibilities.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the bad HFA/floating record/path, owning code surfaces,
  representative tests, and smallest focused proof command.

### Step 2: Repair Classified HFA/Floating Owner

Goal: make generated AArch64 variadic HFA/floating paths preserve and consume
the expected values.

Primary target: code surface identified by Step 1.

Actions:

- Implement the localized HFA/floating call-lane, `va_arg`, register-save-area,
  overflow-area, lane materialization, or consumer repair.
- Reuse existing AArch64 frame-slot, stack-offset, register, scratch,
  prepared-home, and address materialization helpers when available.
- Preserve unrelated local/value-home, frame/formal, `va_start`, aggregate
  helper text, F128 transport, scalar ALU, runner, expectation, and timeout
  behavior.

Completion check:

- Focused proof shows the classified HFA/floating values survive through their
  generated path, or `todo.md` records the next first bad fact with evidence.

### Step 3: Add Focused HFA/Floating Coverage

Goal: make the repaired HFA/floating behavior observable in local backend
tests.

Primary target: existing AArch64 machine-printer, variadic, instruction
dispatch, frame, call-lane, `va_arg`, or prepared-BIR tests that already cover
adjacent variadic function behavior.

Actions:

- Add or extend coverage proving HFA/floating values are selected,
  materialized, transported, and consumed through the repaired owner.
- Add or extend coverage for the exact source area, lane, call-boundary move,
  or consumer path identified in Step 1.
- Preserve adjacent scalar, local publication, aggregate helper, `va_start`,
  aggregate `va_arg`, and ordinary memory helper behavior.

Completion check:

- Local coverage exercises the repaired HFA/floating contract and verifies
  adjacent behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 helper/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, 318, 319, 320, 321, and
  322, 323, 324, and 325, and the `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if generated code still corrupts or drops
  HFA/floating values in the classified path.

Completion check:

- `todo.md` records fresh proof. The current HFA/floating blocker is gone, or
  the remaining blocker is explicitly localized for the next packet.
