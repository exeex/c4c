# AArch64 Call Argument Register Authority

Status: Active
Source Idea: ideas/open/291_aarch64_call_argument_register_authority.md
Activated From: ideas/closed/289_aarch64_function_pointer_indirect_call_values.md

## Purpose

Repair AArch64 call-boundary argument lowering so prepared register authority
for call arguments reaches the ABI registers consumed by the call.

## Goal

Make AArch64 call-boundary moves honor prepared argument register authority,
starting with the `printf` format-pointer argument mismatch in `src/00210.c`.

## Core Rule

Fix semantic call-argument register authority. Do not improve this route by
changing tests, expected outputs, runner behavior, CTest registration,
allowlists, unsupported classifications, timeout policy, parser, sema, libc
behavior, or c-testsuite filename/symbol-specific shortcuts.

## Read First

- `ideas/open/291_aarch64_call_argument_register_authority.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00210.c`
- AArch64 call lowering under `src/backend/mir/aarch64/codegen/calls.cpp`
- AArch64 global/address materialization and operand-resolution helpers under
  `src/backend/mir/aarch64/codegen/`
- Prepared call plans, value locations, storage, and regalloc facts under
  `src/backend/prealloc/`

## Current Targets

- AArch64 call-boundary move lowering for pointer/scalar call arguments.
- Prepared argument register authority versus placement fallback selection.
- The `printf` format-string pointer argument in `src/00210.c`.
- Preservation of existing attributed function-pointer indirect-call behavior.

## Non-Goals

- Do not repair function-pointer cast recovery or indirect-call callee
  materialization; that owner is closed in
  `ideas/closed/289_aarch64_function_pointer_indirect_call_values.md`.
- Do not repair scalar parameter/ALU authority; that owner is closed in
  `ideas/closed/290_aarch64_scalar_parameter_alu_authority.md`.
- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, or CTest registration.
- Do not change parser or sema behavior unless later proof shows backend call
  argument facts are correct and frontend facts are malformed.
- Do not add named-case shortcuts for `src/00210.c`, `.str0`, `printf`, one
  function name, or one emitted move shape.

## Working Model

The function-pointer closure review showed `src/00210.c` preserves attributed
function-pointer calls as valid indirect calls through `actual_function`. The
remaining runtime mismatch is outside that owner: prepared facts place the
`printf` format pointer in `x21`, but emitted call-boundary argument lowering
uses `x20` for the move into `x0`. The first repair should prove that mismatch
and identify the exact prepared call-plan/value-home surface before changing
code.

## Execution Rules

- Keep packets small and prove each packet with a fresh build or
  supervisor-selected narrow test command.
- Inspect prepared call plans, value homes, storage plans, regalloc facts, and
  final AArch64 assembly before changing code.
- Preserve existing valid attributed function-pointer indirect calls.
- Document unrelated frontend, libc, aggregate, or scalar-ALU blockers in
  `todo.md` instead of broadening this route.
- Escalate to reviewer scrutiny if the implementation appears to special-case
  `00210`, `.str0`, `printf`, one symbol, or one emitted move shape.

## Steps

### Step 1: Establish Call Argument Register Authority Failure

Goal: Prove and localize `src/00210.c` to call-argument register-authority
mismatch, not function-pointer value loss.

Primary target: `tests/c/external/c-testsuite/src/00210.c`.

Actions:

- Reproduce the current AArch64 backend behavior for `src/00210.c` with the
  supervisor-selected narrow command.
- Inspect generated assembly for attributed function-pointer calls and confirm
  they remain indirect calls through `actual_function`.
- Inspect generated assembly for the `printf` call and identify the register
  move used for the format-pointer argument.
- Inspect prepared call plans, value locations, storage plans, and regalloc
  facts for the format-pointer argument and destination ABI register.
- Record the observed failure shape and owned backend surfaces in `todo.md`.

Completion check: `todo.md` names the call-argument register-authority
failure, the backend lowering/emission surface to change, and the proof
command/log used for the baseline.

### Step 2: Repair Call-Boundary Argument Source Selection

Goal: Ensure AArch64 call-boundary moves choose the prepared source register
authority for pointer/scalar arguments.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp` and supporting
operand/register conversion helpers.

Actions:

- Identify where before-call moves resolve source registers from prepared
  move, call argument, value-home, storage, and placement facts.
- Repair source selection so explicit prepared register authority wins over
  stale placement or fallback registers when the facts agree.
- Preserve existing immediate, direct symbol-address, stack-slot, call-result,
  and indirect-callee behavior.
- Add focused backend tests for call-boundary argument moves that carry
  prepared source register authority.
- Build and run the supervisor-selected proof for the focused backend tests and
  `src/00210.c` if requested.

Completion check: the prepared source register for the `printf` format pointer
is the value moved into the destination ABI argument register.

### Step 3: Repair Destination ABI Register Readiness

Goal: Ensure call-boundary argument moves materialize into the ABI register
consumed by the call.

Primary target: prepared call argument plans and AArch64 call-boundary move
instruction records.

Actions:

- Inspect destination argument register selection for the failing call.
- Repair any missing or mismatched destination ABI register facts.
- Add focused backend tests that assert pointer/scalar call arguments move from
  authoritative prepared source registers into the expected AAPCS64 argument
  registers.
- Build and run the supervisor-selected narrow proof for `src/00210.c`.

Completion check: `src/00210.c` no longer fails from the `printf`
format-pointer argument being moved from the wrong prepared register.

### Step 4: Validate Owner Boundary

Goal: Confirm the repair is not an overfit to one testcase, one symbol, or one
emitted move shape.

Primary target: focused call-boundary backend tests plus `src/00210.c`.

Actions:

- Re-run the supervisor-selected owner proof for `src/00210.c`.
- Re-run focused backend tests covering call-argument register authority.
- Confirm existing attributed function-pointer indirect calls remain indirect
  and valid.
- Inspect whether any remaining failure belongs to frontend attribute handling,
  libc behavior, aggregate ABI, scalar ALU, or another separate owner.
- Record separated blockers in `todo.md` instead of broadening this route.

Completion check: proof supports closure of this source idea, or `todo.md`
records the exact remaining source-idea gap.

### Step 5: Closure Review

Goal: Confirm the route is not overfit and decide whether the source idea is
complete.

Primary target: `src/00210.c` and focused call-boundary backend tests.

Actions:

- Re-run the supervisor-selected closure proof.
- Confirm no pass-count improvement came from tests, expectations, runner
  behavior, allowlists, unsupported classifications, CTest registration, or
  timeout policy.
- Confirm attributed function-pointer indirect-call shape remains intact.
- If remaining failures expose a separate semantic owner, record or split it
  instead of absorbing it into this route.

Completion check: focused proof supports closure of this source idea, or
`todo.md` records the exact remaining source-idea gap.
