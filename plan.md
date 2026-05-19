# AArch64 Vararg Consumption Source And Progression Runbook

Status: Active
Source Idea: ideas/open/323_aarch64_vararg_consumption_source_progression.md
Activated from: idea 322 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` runtime blocker by fixing AArch64 `va_arg`
source selection, value transport, and `va_list` progression after `va_start`
has initialized a real local `va_list` object.

## Goal

Make AArch64 vararg consumption load and advance from the correct initialized
`va_list` source for aggregate, floating, and long-double consumers.

## Core Rule

Progress must be a general AArch64 `va_arg` consumption capability. Do not
reopen `va_start` destination publication, aggregate helper text lowering, F128
transport addressability, HFA argument call-lane lowering, scalar ALU immediate
materialization, frame adjustment materialization, stack-slot memory spelling,
semantic admission, expectations, unsupported classifications, runners,
timeout policy, proof-log contents, or CTest registration unless generated-code
evidence proves that surface owns the current consumer fault.

## Read First

- `ideas/open/323_aarch64_vararg_consumption_source_progression.md`
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
  - generated `myprintf` reaches scalar and string varargs, then emits corrupt
    long-double/floating output and segfaults while consuming the initialized
    `va_list`.
- Initial suspected owner family:
  - AArch64 variadic `va_arg` prepared access plans and helper records
  - `src/backend/mir/aarch64/codegen/variadic.cpp`
  - source selection, value copy/load, alignment, and progression helpers for
    aggregate, floating, and long-double consumers
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

- Do not reopen idea 322's `va_start` destination address publication owner
  unless generated evidence proves the materialized address is the wrong
  writable object.
- Do not reopen idea 321's aggregate `va_arg` helper text lowering owner.
- Do not reopen idea 320's F128 transport addressability owner.
- Do not reopen idea 319's HFA argument call-lane owner except where current
  consumer-side source/progression evidence requires HFA or floating retrieval
  rules.
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
  they become the next first bad fact after `va_arg` consumption is repaired.

## Working Model

The representative now gets past prior assembler blockers, HFA/aggregate
argument ABI corruption, scalar immediate materialization, raw helper text,
large stack/frame materialization, F128 transport addressability, aggregate
`va_arg` helper lowering, and `va_start` destination publication. The next
first bad fact occurs during vararg consumption: initialized `va_list` fields
are read and advanced, scalar/string output appears, and long-double/floating
values corrupt before a segfault. The repair should localize which consumer
source selection, value transport, or progression rule is wrong and fix that
rule generally.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact consumer record, access plan, and emitted load/progression
  sequence before editing code.
- Prefer focused backend coverage for the vararg consumption contract
  before relying only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, `va_start`
  destination publication, aggregate helper lowering, F128 transport, frame,
  printer, scalar ALU, runner, or expectation coverage.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize Vararg Consumption Fault

Goal: identify the exact post-`va_start` `va_arg` consumer path that corrupts
long-double/floating output or advances the `va_list` incorrectly.

Primary target: generated `00204.c` artifacts and
`src/backend/mir/aarch64/codegen/variadic.cpp`.

Actions:

- Trace the generated `myprintf` consumer sequence after the repaired
  `va_start` initialization.
- Map failing loads, copies, alignment, and progression updates back to
  prepared access plans, machine records, and printer helpers.
- Distinguish source selection, value transport, and `va_list` progression
  responsibilities.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the bad consumer record/path, owning code surfaces,
  representative tests, and smallest focused proof command.

### Step 2: Repair Consumer Source And Progression

Goal: make the localized `va_arg` consumer use the correct initialized source,
copy/load the value correctly, and advance the `va_list` according to AAPCS64.

Primary target: code surface identified by Step 1.

Actions:

- Implement the localized source selection, value transport, alignment, or
  progression repair.
- Reuse existing AArch64 access-plan, frame-slot, stack-offset, register,
  scratch, and address materialization helpers when available.
- Preserve unrelated `va_start`, aggregate helper text, F128 transport,
  argument ABI, scalar ALU, frame, runner, expectation, and timeout behavior.

Completion check:

- Focused proof shows the localized consumer path emits correct executable
  source/progression code, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Vararg Consumer Coverage

Goal: make the repaired `va_arg` consumer behavior observable in local backend
tests.

Primary target: existing AArch64 machine-printer, variadic, instruction
dispatch, or prepared-BIR tests that already cover adjacent variadic helper
behavior.

Actions:

- Add or extend coverage for the repaired aggregate/floating/long-double
  consumer path.
- Assert source selection, value movement, and `va_list` progression are
  observable and executable.
- Preserve adjacent scalar, HFA, aggregate helper, `va_start`, and ordinary
  memory helper behavior.

Completion check:

- Local coverage exercises the repaired consumer contract and verifies adjacent
  behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 helper/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, 318, 319, 320, 321, and
  322, and the `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if the localized consumer still reads,
  copies, or advances from the wrong source.

Completion check:

- `todo.md` records fresh proof. The current vararg consumer blocker is gone,
  or the remaining blocker is explicitly localized for the next packet.
