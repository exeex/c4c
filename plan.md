# AArch64 Variadic Local Value Home Publication Runbook

Status: Active
Source Idea: ideas/open/325_aarch64_variadic_local_value_home_publication.md
Activated from: idea 324 closure after Step 4 residual classification

## Purpose

Repair the next `00204.c` runtime blocker by fixing AArch64 variadic function
local/value-home initialization and constant/pattern operand publication after
prior frame/formal, `va_start`, and `va_arg` repairs.

## Goal

Make generated AArch64 variadic functions publish ordinary local/value homes
before generated control flow, matcher logic, and printf operand consumers read
those stack or register homes.

## Core Rule

Progress must be a general AArch64 variadic local/value-home publication
capability. Do not reopen frame/formal publication, aggregate `va_arg`
source/progression, `va_start` destination publication, aggregate helper text
lowering, F128 transport addressability, HFA argument call-lane lowering,
scalar ALU immediate materialization, frame adjustment materialization,
stack-slot memory spelling, semantic admission, expectations, unsupported
classifications, runners, timeout policy, proof-log contents, or CTest
registration unless generated-code evidence proves that surface owns the
current local/value-home fault.

## Read First

- `ideas/open/325_aarch64_variadic_local_value_home_publication.md`
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
  - generated `myprintf` reads ordinary local/value homes such as
    `[sp, #640]`, `[sp, #648]`, `[sp, #656]`, and related slots before
    same-function stores publish those values, and emits `cmp w13, #0`
    without a preceding local definition for `w13`.
- Initial suspected owner family:
  - AArch64 local/value-home publication before use
  - constant and pattern operand publication in generated variadic functions
  - prepared homes, dispatch records, and machine-printer lowering for local
    value consumers
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

- Do not reopen idea 324's frame-size coverage or fixed-formal publication
  owner unless generated evidence again shows uncovered stack references or
  clobbered fixed formals.
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
  they become the next first bad fact after local/value-home publication is
  repaired.

## Working Model

The representative now gets past prior assembler blockers, HFA/aggregate
argument ABI corruption, scalar immediate materialization, raw helper text,
large stack/frame materialization, F128 transport addressability, aggregate
`va_arg` helper lowering, and `va_start` destination publication. The next
first bad fact is ordinary local/value-home publication in generated
`myprintf`: generated code reads local homes and register values for matcher
and printf operands before those values have been initialized in the current
function. The repair should localize whether prepared homes, constant/pattern
operand lowering, dispatch records, or printer lowering owns that fault and
fix that rule generally.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact local/value home, constant or pattern operand, prepared
  record, and emitted stack/register sequence before editing code.
- Prefer focused backend coverage for publication-before-use before relying
  only on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff,
  frame/formal publication, `va_start` destination publication, aggregate
  helper lowering, F128 transport, aggregate `va_arg` source/progression,
  scalar ALU, runner, or expectation coverage.
- If the representative advances to global initializer emission, runtime
  mismatch, timeout, or another blocker, record the new first bad fact and
  return it to lifecycle classification unless generated-code evidence proves
  this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize Local Value Home Publication Fault

Goal: identify the exact local/value-home, constant, pattern operand, or
temporary publication path that emits reads before same-function initialization.

Primary target: generated `00204.c` artifacts and AArch64 prepared-home,
dispatch, and local publication surfaces.

Actions:

- Trace the generated `myprintf` first undefined local/register use and the
  stack homes read before publication.
- Map those local homes, constants, and pattern operands back to prepared
  homes, machine records, dispatch paths, and printer helpers.
- Distinguish ordinary local-value publication, constant materialization,
  pattern operand materialization, and unrelated frame/formal responsibilities.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the bad local/value-home record/path, owning code surfaces,
  representative tests, and smallest focused proof command.

### Step 2: Repair Local Value Home Publication

Goal: make generated AArch64 variadic functions publish local/value homes,
constants, and pattern operands before generated user code consumes them.

Primary target: code surface identified by Step 1.

Actions:

- Implement the localized local/value-home, constant, pattern operand, or
  temporary publication repair.
- Reuse existing AArch64 frame-slot, stack-offset, register, scratch,
  prepared-home, and address materialization helpers when available.
- Preserve unrelated frame/formal, `va_start`, aggregate helper text,
  aggregate `va_arg`, F128 transport, argument ABI, scalar ALU, runner,
  expectation, and timeout behavior.

Completion check:

- Focused proof shows local/value-home consumers read initialized values, or
  `todo.md` records the next first bad fact with evidence.

### Step 3: Add Focused Local Publication Coverage

Goal: make the repaired local/value-home publication behavior observable in
local backend tests.

Primary target: existing AArch64 machine-printer, variadic, instruction
dispatch, frame, or prepared-BIR tests that already cover adjacent variadic
function behavior.

Actions:

- Add or extend coverage proving local/value homes are published before
  control-flow tests, matcher operands, or printf operand consumers read them.
- Add or extend coverage for relevant constant or pattern operand publication
  if Step 1 identifies those as the owning path.
- Preserve adjacent scalar, HFA, aggregate helper, `va_start`, aggregate
  `va_arg`, and ordinary memory helper behavior.

Completion check:

- Local coverage exercises the repaired local publication contract and verifies
  adjacent behavior remains stable.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 helper/backend coverage,
  prior-owner guardrails from ideas 314, 315, 317, 318, 319, 320, 321, and
  322, 323, and 324, and the `00204.c` c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if generated code still consumes ordinary
  local/value homes, constants, pattern operands, or temporaries before
  publication.

Completion check:

- `todo.md` records fresh proof. The current local/value-home publication
  blocker is gone, or the remaining blocker is explicitly localized for the
  next packet.
