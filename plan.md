# AArch64 Va Start Destination Address Materialization Runbook

Status: Active
Source Idea: ideas/open/322_aarch64_va_start_destination_address_materialization.md
Activated from: idea 321 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` runtime blocker by ensuring AArch64 `va_start`
field stores write through a materialized local `va_list` destination address.

## Goal

Make AArch64 `VaStart` lowering materialize or publish the writable local
`va_list` destination before storing `__stack`, `__gr_top`, `__vr_top`,
`__gr_offs`, and `__vr_offs` fields.

## Core Rule

Progress must be a general AArch64 `va_start` destination address
materialization capability. Do not reopen aggregate `va_arg` helper lowering,
F128 transport addressability, HFA argument ABI lowering, scalar ALU immediate
materialization, frame adjustment materialization, stack-slot memory spelling,
semantic admission, expectations, unsupported classifications, runners,
timeout policy, proof-log contents, or CTest registration.

## Read First

- `ideas/open/322_aarch64_va_start_destination_address_materialization.md`
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
  - generated `myprintf` reaches runtime but segfaults because `va_start`
    field stores use `x21` before that register is materialized as a writable
    local `va_list` address.
- Initial suspected owner family:
  - `src/backend/mir/aarch64/codegen/variadic.cpp`
  - `make_variadic_va_start_record()`
  - `print_va_start_lowering_lines()`
  - `homes.destination_va_list`
  - frame-slot and address materialization helpers used by AArch64 machine
    printing
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

- Do not reopen idea 321's aggregate `va_arg` helper source, copy, or
  progression lowering owner.
- Do not reopen idea 320's F128 transport addressability owner.
- Do not reopen idea 319's HFA, floating, long-double, or aggregate ABI
  argument classification and lowering owner.
- Do not reopen idea 318's scalar ALU immediate materialization owner.
- Do not reopen idea 315's large frame setup and teardown materialization
  owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication
  spelling owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual unless
  generated evidence proves it is the same `va_start` destination publication
  fault.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not fix global initializer emission, later runtime mismatches, or later
  `stdarg` progression faults unless they become the next first bad fact after
  `va_start` destination materialization is repaired.

## Working Model

The representative now gets past prior assembler blockers, HFA/aggregate
argument ABI corruption, scalar immediate materialization, raw helper text,
large stack/frame materialization, F128 transport addressability, and aggregate
`va_arg` helper lowering. The next first bad fact occurs when the AArch64
`va_start` helper stores fields through a destination register that has not
been published as the address of the local `va_list` object. The repair should
make the `VaStart` record or printer carry and materialize the actual writable
destination address before field stores execute.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact `VaStart` destination representation before editing code.
- Prefer focused backend coverage for the destination materialization contract
  before relying only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, HFA ABI,
  aggregate `va_arg`, F128 transport, frame, printer, scalar ALU, runner, or
  expectation coverage.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize Va Start Destination Publication

Goal: identify the exact `VaStart` records, destination homes, and AArch64
printer surface that currently store through an unmaterialized register.

Primary target: generated `00204.c` artifacts and
`src/backend/mir/aarch64/codegen/variadic.cpp`.

Actions:

- Trace the generated `myprintf` `va_start` stores back to prepared variadic
  helper entries, `homes.destination_va_list`, and the machine printer record.
- Distinguish record construction, register allocation, frame-slot
  publication, and final field-store emission responsibilities.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the unmaterialized destination record shape, owning code
  surfaces, representative tests, and smallest focused proof command.

### Step 2: Materialize Va Start Destination Address

Goal: ensure `VaStart` field stores use a real writable local `va_list`
address.

Primary target: code surface identified by Step 1.

Actions:

- Implement destination address materialization or publication for `VaStart`.
- Reuse existing AArch64 frame-slot, stack-offset, register, scratch, and
  address materialization helpers when available.
- Preserve unrelated aggregate `va_arg`, F128 transport, argument ABI, scalar
  ALU, frame, runner, expectation, and timeout behavior.

Completion check:

- Focused proof shows `VaStart` field stores are based on a materialized local
  `va_list` address, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Va Start Destination Coverage

Goal: make `va_start` destination address materialization observable in local
backend tests.

Primary target: existing AArch64 machine-printer, variadic, instruction
dispatch, or prepared-BIR tests that already cover adjacent variadic helper
behavior.

Actions:

- Add or extend coverage for `VaStart` lowering with a local `va_list`
  destination.
- Assert field stores use a materialized address and do not rely on an
  uninitialized destination register.
- Preserve adjacent scalar, HFA, aggregate `va_arg`, and ordinary memory helper
  behavior.

Completion check:

- Local coverage exercises `va_start` destination materialization and verifies
  adjacent behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 helper/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, 318, 319, 320, and 321, and
  the `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if `VaStart` field stores still use an
  unmaterialized destination register.

Completion check:

- `todo.md` records fresh proof. The current unmaterialized `va_start`
  destination blocker is gone, or the remaining blocker is explicitly
  localized for the next packet.
