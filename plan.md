# AArch64 F128 Transport Addressability Runbook

Status: Active
Source Idea: ideas/open/320_aarch64_f128_transport_addressability.md
Activated from: idea 319 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` backend blocker by localizing and fixing the AArch64
`f128_transport` machine-node addressability path that currently prevents the
machine printer from spelling an `fp128` memory transport.

## Goal

Make `f128_transport` memory addresses printable or materializable through a
general AArch64 backend rule, while preserving the HFA/aggregate argument ABI
repairs from idea 319 and the earlier assembler-blocker repairs.

## Core Rule

Progress must be a general AArch64 `fp128` transport/addressability capability.
Do not reopen HFA argument ABI lowering, scalar ALU immediate materialization,
raw `va_start` helper lowering, large frame adjustment materialization,
stack-slot memory spelling, frame-layout consistency, semantic admission,
expectations, unsupported classifications, runners, timeout policy, proof-log
contents, or CTest registration.

## Read First

- `ideas/open/320_aarch64_f128_transport_addressability.md`
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
  - `00204.c` reaches the AArch64 machine-node printer and fails with
    `cannot print AArch64 machine node family=f128_transport
    opcode=f128_transport: f128 memory transport address is not printable`.
- Initial suspected owner family:
  - `src/backend/mir/aarch64/codegen/instruction.cpp`
  - `src/backend/prealloc/special_carriers.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - AArch64 machine-printer address materialization helpers
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

- Do not reopen idea 319's HFA, floating, long-double, or aggregate ABI
  argument classification and lowering owner.
- Do not reopen idea 318's scalar ALU immediate materialization owner.
- Do not reopen idea 317's raw `va_start` helper-text lowering owner.
- Do not reopen idea 315's large frame setup and teardown materialization
  owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication
  spelling owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual unless
  generated evidence proves the `f128_transport` blocker is the same fault.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not fix global initializer emission unless it becomes the next first bad
  fact after this owner advances.

## Working Model

The representative now gets past prior assembler blockers and the HFA/aggregate
argument ABI corruption. The next first bad fact occurs when a generated
`f128_transport` machine node tries to move an `fp128` value through memory and
the AArch64 printer rejects its address as unprintable. The repair should find
where that transport address is represented and either produce a printable
memory operand or materialize the address through an established scratch path.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact `f128_transport` node and address shape before editing
  code.
- Prefer focused backend coverage for the implicated transport/addressability
  path before relying only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, HFA ABI,
  variadic, frame, printer, or scalar ALU coverage to improve counts.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend addressability broadly.

## Ordered Steps

### Step 1: Localize F128 Transport Address Shape

Goal: identify the exact `f128_transport` node, memory address shape, and
AArch64 backend surface that owns the unprintable address.

Primary target: generated `00204.c` artifacts and the AArch64
`f128_transport` transport/carrier/printer surfaces.

Actions:

- Trace the failing instruction index from the generated artifact to the
  machine-node record and the address operand.
- Distinguish transport record construction, carrier selection, call-boundary
  move handling, and final printer address spelling.
- Record in `todo.md` the owning code surface, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the failing address shape, owning code surface,
  representative tests, and smallest focused proof command.

### Step 2: Repair F128 Transport Addressability

Goal: fix the localized `f128_transport` addressability path generally.

Primary target: code surface identified by Step 1.

Actions:

- Implement the narrow transport/addressability repair for `fp128` memory
  moves.
- Reuse existing AArch64 memory operand, scratch address materialization, or
  carrier helpers when available.
- Preserve unrelated argument ABI, scalar ALU, variadic helper, frame, runner,
  expectation, and timeout behavior.

Completion check:

- Focused proof shows the original `f128 memory transport address is not
  printable` failure is gone, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Transport Coverage

Goal: make the repaired `f128_transport` addressability path observable in
local backend tests.

Primary target: existing AArch64 machine-printer, instruction dispatch,
carrier, or call-boundary tests that already cover adjacent transport behavior.

Actions:

- Add or extend coverage for the implicated `f128_transport` memory address
  shape.
- Preserve adjacent scalar, vector, and ordinary memory transport behavior.
- Assert the backend addressability contract rather than one incidental
  testcase name or instruction index.

Completion check:

- Local coverage exercises the repaired `f128_transport` path and verifies
  adjacent behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 transport/backend
  coverage, prior-owner guardrails from ideas 314, 315, 317, 318, and 319, and
  the `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if the same `f128_transport`
  addressability failure remains.

Completion check:

- `todo.md` records fresh proof. The current `f128_transport` addressability
  blocker is gone, or the remaining blocker is explicitly localized for the
  next packet.
