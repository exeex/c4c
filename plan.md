# AArch64 Scalar Parameter ALU Authority

Status: Active
Source Idea: ideas/open/290_aarch64_scalar_parameter_alu_authority.md
Activated From: ideas/closed/289_aarch64_function_pointer_indirect_call_values.md

## Purpose

Repair the AArch64 native backend path where scalar function parameters should
arrive in the registers consumed by selected scalar ALU operations.

## Goal

Make scalar function parameters feed AArch64 selected ALU operands from
authoritative incoming parameter registers instead of stale callee-saved or
fallback registers.

## Core Rule

Fix semantic parameter-to-ALU value authority. Do not improve this route by
changing tests, expected outputs, runner behavior, CTest registration,
allowlists, unsupported classifications, timeout policy, parser, sema, or
c-testsuite filename/function/expression-specific shortcuts.

## Read First

- `ideas/open/290_aarch64_scalar_parameter_alu_authority.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00124.c`
- AArch64 scalar ALU lowering and operand-resolution surfaces under
  `src/backend/mir/aarch64/codegen/`
- Prepared value-location, storage, parameter, and regalloc facts under
  `src/backend/prealloc/`

## Current Targets

- AArch64 function-entry scalar parameter homes.
- Prepared value/storage authority for scalar parameters.
- Selected scalar ALU operand register selection.
- Focused proof case: `src/00124.c`.

## Non-Goals

- Do not repair function-pointer materialization or indirect-call callee setup;
  that owner is closed in
  `ideas/closed/289_aarch64_function_pointer_indirect_call_values.md`.
- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, or CTest registration.
- Do not change parser or sema behavior.
- Do not add named-case shortcuts for `src/00124.c`, one function name, one
  parameter name, or one arithmetic expression.
- Do not take on broad aggregate ABI, libc behavior, call-boundary argument
  authority, or unrelated call-chain repair.

## Working Model

The function-pointer closure review showed `src/00124.c` already returns the
selected function pointer and performs an indirect `blr`. The remaining failure
is in the called function: scalar arithmetic consumes stale callee-saved
registers instead of incoming `w0`/`w1` parameter registers. The first repair
should prove that owner, then repair the prepared parameter-to-selected-ALU
authority path.

## Execution Rules

- Keep packets small and prove each packet with a fresh build or
  supervisor-selected narrow test command.
- Inspect prepared BIR, value homes, storage plans, regalloc facts, and final
  AArch64 assembly before changing code.
- Preserve the existing valid function-pointer return and indirect `blr` shape.
- Document unrelated aggregate, call-boundary, frontend, or libc blockers in
  `todo.md` instead of broadening this route.
- Escalate to reviewer scrutiny if the implementation appears to special-case
  `00124`, one function name, one parameter name, or one emitted ALU shape.

## Steps

### Step 1: Establish Scalar Parameter ALU Failure

Goal: Prove and localize `src/00124.c` to stale scalar parameter values feeding
selected AArch64 ALU operations, not function-pointer value loss.

Primary target: `tests/c/external/c-testsuite/src/00124.c`.

Actions:

- Reproduce the current AArch64 backend behavior for `src/00124.c` with the
  supervisor-selected narrow command.
- Inspect generated assembly for the function-pointer return and indirect call
  to confirm that path remains valid.
- Inspect generated assembly for the called scalar function and identify which
  registers are used for incoming parameters and ALU operands.
- Inspect prepared value locations, storage plans, and regalloc facts for the
  scalar parameters and ALU input values.
- Record the observed failure shape and owned backend surfaces in `todo.md`.

Completion check: `todo.md` names the scalar parameter/ALU authority failure,
the backend lowering/emission surface to change, and the proof command/log
used for the baseline.

### Step 2: Repair Parameter Home Publication

Goal: Ensure scalar function parameters have authoritative prepared homes that
reflect the incoming AArch64 ABI registers.

Primary target: prepared value-location, storage, and regalloc facts for
function-entry scalar parameters.

Actions:

- Identify where scalar parameters are represented in prepared value homes and
  storage plans.
- Repair missing, stale, or placement-only facts so parameter values retain
  incoming ABI register authority.
- Add focused backend tests that assert parameter homes/storage facts preserve
  the expected register identity.
- Build and run the supervisor-selected proof for the focused parameter facts
  and `src/00124.c` if requested.

Completion check: prepared facts for the failing scalar parameters identify
the incoming parameter registers needed by downstream AArch64 ALU lowering.

### Step 3: Repair Selected ALU Operand Consumption

Goal: Make AArch64 selected scalar ALU nodes consume scalar parameter operands
from authoritative prepared registers.

Primary target: AArch64 scalar ALU operand resolution and selected instruction
record construction.

Actions:

- Inspect the selected scalar ALU path that currently chooses stale
  callee-saved or fallback registers.
- Repair operand selection to honor authoritative prepared parameter/value
  facts.
- Preserve existing scalar immediate, local value, and call-result behavior.
- Add focused backend tests for selected scalar ALU operands sourced from
  function parameters.
- Build and run the supervisor-selected narrow proof for `src/00124.c`.

Completion check: `src/00124.c` no longer fails from stale scalar parameter
values in the returned function-pointer call chain.

### Step 4: Validate Owner Boundary

Goal: Confirm the repair is not an overfit to one testcase or one emitted
instruction shape.

Primary target: focused scalar parameter/ALU backend tests plus `src/00124.c`.

Actions:

- Re-run the supervisor-selected owner subset for `src/00124.c`.
- Re-run focused backend tests covering parameter homes and selected ALU
  operand authority.
- Inspect whether any remaining failure belongs to aggregate ABI, call-boundary
  arguments, frontend/sema, or another separate owner.
- Record separated blockers in `todo.md` instead of broadening this route.

Completion check: proof supports closure of this source idea, or `todo.md`
records the exact remaining source-idea gap.

### Step 5: Closure Review

Goal: Confirm the route is not overfit and decide whether the source idea is
complete.

Primary target: `src/00124.c` and focused scalar parameter/ALU backend tests.

Actions:

- Re-run the supervisor-selected closure proof.
- Confirm no pass-count improvement came from tests, expectations, runner
  behavior, allowlists, unsupported classifications, CTest registration, or
  timeout policy.
- Confirm the generated function-pointer return and indirect-call shape remain
  intact.
- If remaining failures expose a separate semantic owner, record or split it
  instead of absorbing it into this route.

Completion check: focused proof supports closure of this source idea, or
`todo.md` records the exact remaining source-idea gap.
