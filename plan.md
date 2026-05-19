# AArch64 Variadic Frame And Formal Publication Runbook

Status: Active
Source Idea: ideas/open/324_aarch64_variadic_frame_formal_publication.md
Activated from: idea 323 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` runtime blocker by fixing AArch64 variadic function
frame-slot publication and fixed-formal preservation after prior `va_start`
and `va_arg` lowering repairs.

## Goal

Make generated AArch64 variadic functions allocate stack frames that cover all
emitted local/spill slots and preserve incoming fixed formals before variadic
setup or loop code reuses argument registers.

## Core Rule

Progress must be a general AArch64 variadic frame/local/formal publication
capability. Do not reopen aggregate `va_arg` source/progression, `va_start`
destination publication, aggregate helper text lowering, F128 transport
addressability, HFA argument call-lane lowering, scalar ALU immediate
materialization, frame adjustment materialization, stack-slot memory spelling,
semantic admission, expectations, unsupported classifications, runners,
timeout policy, proof-log contents, or CTest registration unless generated-code
evidence proves that surface owns the current frame/formal fault.

## Read First

- `ideas/open/324_aarch64_variadic_frame_formal_publication.md`
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
  - generated `myprintf` allocates only `896` bytes but later references stack
    slots such as `[sp, #9696]`, and it clobbers the incoming format pointer
    with `mov x0, x21` before the format loop.
- Initial suspected owner family:
  - AArch64 frame-size accounting and local stack-slot publication
  - fixed formal argument publication/preservation for variadic functions
  - prepared homes, frame slots, and machine-printer lowering for AArch64
    variadic function entries
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

- Do not reopen idea 323's aggregate/floating `va_arg` source selection or
  `FpOffset` progression owner unless generated evidence proves those paths
  still own the first bad fact.
- Do not reopen idea 322's `va_start` destination address publication owner.
- Do not reopen idea 321's aggregate `va_arg` helper text lowering owner.
- Do not reopen idea 320's F128 transport addressability owner.
- Do not reopen idea 319's HFA argument call-lane owner except where current
  frame/formal evidence directly requires it.
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
  they become the next first bad fact after frame/formal publication is
  repaired.

## Working Model

The representative now gets past prior assembler blockers, HFA/aggregate
argument ABI corruption, scalar immediate materialization, raw helper text,
large stack/frame materialization, F128 transport addressability, aggregate
`va_arg` helper lowering, and `va_start` destination publication. The next
first bad fact is frame/formal correctness in generated `myprintf`: emitted
stack-slot offsets exceed the allocated frame, and the incoming fixed formal
for the format string is not reliably preserved before the format loop. The
repair should localize whether prepared homes, frame-size computation,
stack-slot publication, formal preservation, or printer lowering owns that
fault and fix that rule generally.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact frame slot, formal home, prepared record, and emitted
  stack/register sequence before editing code.
- Prefer focused backend coverage for frame-size coverage and fixed-formal
  preservation before relying only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff, `va_start`
  destination publication, aggregate helper lowering, F128 transport,
  aggregate `va_arg` source/progression, frame, printer, scalar ALU, runner,
  or expectation coverage.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize Frame And Formal Publication Fault

Goal: identify the exact frame-size, stack-slot, local-publication, or
fixed-formal preservation path that emits out-of-frame accesses or clobbers the
incoming format pointer.

Primary target: generated `00204.c` artifacts and
AArch64 frame/formal publication surfaces.

Actions:

- Trace the generated `myprintf` entry, frame allocation, fixed-formal
  publication, and first out-of-frame stack references.
- Map oversized stack offsets and the `mov x0, x21` formal clobber back to
  prepared homes, frame slots, machine records, and printer helpers.
- Distinguish frame-size accounting, local stack-slot publication, spill-slot
  materialization, and fixed-formal preservation responsibilities.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the bad frame/formal record/path, owning code surfaces,
  representative tests, and smallest focused proof command.

### Step 2: Repair Frame And Formal Publication

Goal: make generated AArch64 variadic functions allocate enough frame space
for emitted stack slots and preserve incoming fixed formals before generated
user code consumes them.

Primary target: code surface identified by Step 1.

Actions:

- Implement the localized frame-size, local-publication, spill-home, or
  fixed-formal preservation repair.
- Reuse existing AArch64 frame-slot, stack-offset, register, scratch,
  prepared-home, and address materialization helpers when available.
- Preserve unrelated `va_start`, aggregate helper text, aggregate `va_arg`,
  F128 transport, argument ABI, scalar ALU, runner, expectation, and timeout
  behavior.

Completion check:

- Focused proof shows emitted stack references are inside the allocated frame
  and fixed formals remain available to generated user code, or `todo.md`
  records the next first bad fact with evidence.

### Step 3: Add Focused Frame And Formal Coverage

Goal: make the repaired frame/local/formal behavior observable in local
backend tests.

Primary target: existing AArch64 machine-printer, variadic, instruction
dispatch, frame, or prepared-BIR tests that already cover adjacent variadic
function behavior.

Actions:

- Add or extend coverage for frame allocation covering emitted stack slots in
  a variadic function.
- Add or extend coverage proving incoming fixed formals are preserved or
  published before variadic setup and loop code can reuse argument registers.
- Preserve adjacent scalar, HFA, aggregate helper, `va_start`, aggregate
  `va_arg`, and ordinary memory helper behavior.

Completion check:

- Local coverage exercises the repaired frame/formal contract and verifies
  adjacent behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 helper/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, 318, 319, 320, 321, and
  322 and 323, and the `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if generated code still references stack
  slots outside the allocated frame or consumes clobbered fixed formals before
  publication.

Completion check:

- `todo.md` records fresh proof. The current frame/formal blocker is gone, or
  the remaining blocker is explicitly localized for the next packet.
