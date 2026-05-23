# AArch64 Pointer-Derived Load Address Scaling Refresh Runbook

Status: Active
Source Idea: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md

## Purpose

Refresh the parked pointer-derived load/address-scaling owner after the Hanoi
starting-state route closed, and decide whether any current first bad fact
still belongs to this source idea.

## Goal

Prove whether `00181` still has an in-scope pointer-derived load or address
scaling timeout failure, or classify the current tree as out of scope for this
idea.

## Core Rule

Do not reopen materialized pointer store writeback, direct `LoadGlobal`
select-store handling, recursive formal post-call repairs, semantic string
loads, frontend admission, ABI composite work, variadic/floating residuals, or
unrelated scalar compare/select residuals unless fresh generated-code evidence
makes that boundary the current first bad fact.

## Read First

- `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
- Parked notes for ideas 360 and 361 when interpreting any `00181` residual.
- `test_before.log` and `test_after.log` if present, because the latest source
  note identifies close-gate proof basis as the remaining lifecycle issue.

## Current Scope

- `c_testsuite_aarch64_backend_src_00181_c`
- Focused backend coverage for pointer-derived memory operands and prepared
  memory operand records
- Guardrails for the idea 360 starting-state output and idea 361 materialized
  pointer store writeback behavior
- Neighbor representatives `00170` and `00189`
- A fresh proof record suitable for the supervisor and close-gate policy

## Non-Goals

- Do not change implementation files during lifecycle activation.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00181`, `Hanoi`, a tower name, one source line, one
  stack offset, one ABI register, one emitted address calculation, or one
  instruction neighborhood.
- Do not expand this runbook into direct `LoadGlobal` select-store handling,
  materialized pointer store writeback, recursive formal preservation, semantic
  string loads, frontend admission, or ABI composite work without a lifecycle
  split.

## Working Model

Idea 362 originally owned the post-writeback `00181` timeout where
pointer-derived loads and address scaling could lose the live index/result
carrier while materializing immediate scales. Prior execution repaired that
owner and later refresh proof reported `00181`, `00170`, `00189`, and focused
memory operand contracts green. This runbook exists to refresh that conclusion
from the current tree, especially now that the adjacent Hanoi starting-state
source idea has closed.

## Execution Rules

- Start from fresh proof and generated-code evidence, not from the historical
  timeout alone.
- If `00181` passes, do not claim new in-scope implementation work under this
  source idea.
- If `00181` fails, capture the earliest observable failure and compare the
  source load operation, pointer/index carrier, expected address, emitted
  address calculation, and timeout-causing consumer against this idea's scope.
- If the first bad fact belongs elsewhere, recommend the matching existing
  source idea or a new lifecycle split instead of broadening this runbook.
- Preserve canonical proof-log names when a delegated executor writes logs:
  `test_before.log` and `test_after.log`.
- Keep routine proof details in `todo.md`; edit this plan only if the refresh
  route itself needs correction.

## Step 1: Refresh the Current `00181` First Bad Fact

Goal: determine whether the current tree still exposes the pointer-derived
load/address-scaling timeout owned by this source idea.

Primary target: `c_testsuite_aarch64_backend_src_00181_c`

Actions:

- Rebuild the default preset.
- Run the supervisor-selected focused proof for `00181` and nearby memory
  operand guardrails.
- If `00181` fails, capture the earliest observable mismatch, timeout,
  segfault, or compile diagnostic.
- Inspect semantic BIR, prepared BIR, and generated AArch64 only enough to
  classify whether the failure is a pointer-derived load/address-scaling owner.

Completion check:

- `todo.md` records the exact proof command, result, and whether the first bad
  fact is in scope for idea 362.

## Step 2: Preserve Adjacent Repair Boundaries

Goal: ensure the refresh result does not regress or blur the repairs around
the Hanoi memory/value-flow chain.

Primary target: focused guardrails for ideas 360 and 361 plus neighboring
AArch64 memory representatives.

Actions:

- Include guardrails selected by the supervisor for memory operand contracts,
  prepared memory operand records, materialized pointer store writeback, and
  current-memory starting-state behavior.
- Treat any regression in direct `LoadGlobal` select-store handling or
  materialized pointer store writeback as an out-of-scope handoff unless fresh
  evidence shows it is required to explain a current pointer-derived
  load/address-scaling failure.
- Keep `00170` and `00189` in the proof subset to protect nearby memory
  lowering behavior.

Completion check:

- `todo.md` records whether adjacent guardrails passed and names any
  out-of-scope residual that should be handled by another source idea.

## Step 3: Lifecycle Decision Handoff

Goal: leave the supervisor with a clear close, deactivate, or split decision.

Actions:

- If no in-scope pointer-derived load/address-scaling failure remains,
  recommend deactivation or closure attempt under the close gate.
- If an in-scope failure remains, hand the localized owner to an executor
  packet.
- If the first bad fact belongs elsewhere, recommend the matching existing
  source idea or a new split.

Completion check:

- `todo.md` contains a concise latest-packet summary, proof result, and
  suggested next lifecycle action.
