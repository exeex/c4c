# AArch64 Hanoi Starting-State Output Mismatch Refresh Runbook

Status: Active
Source Idea: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md

## Purpose

Refresh the parked Hanoi starting-state owner and decide whether any current
first bad fact still belongs to the source idea.

## Goal

Prove whether `00181` still has an in-scope starting-state value-flow failure,
or classify the current tree as out of scope for this idea.

## Core Rule

Do not reopen recursive post-call pointer/formal, scalar formal reload,
callee-saved pointer-formal, address-valued publication, semantic string-load,
or ABI-wide aggregate work unless fresh generated-code evidence makes that
boundary the current first bad fact.

## Read First

- `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`
- Prior parked notes for ideas 357, 358, 359, 361, and 362 when interpreting
  any `00181` residual.

## Current Scope

- `c_testsuite_aarch64_backend_src_00181_c`
- Focused backend coverage for the starting-state value-flow shape repaired by
  this source idea
- Nearby guardrails that protect ideas 357, 358, 359, 361, and 362 from
  accidental regression

## Non-Goals

- Do not change implementation files during lifecycle activation.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00181`, `Hanoi`, the literal value `3`, one tower name,
  one stack offset, one ABI register, or one emitted instruction neighborhood.
- Do not expand this runbook into materialized pointer store writeback,
  pointer-derived load scaling, recursive formal preservation, semantic string
  loads, frontend admission, or ABI composite work without a lifecycle split.

## Working Model

Idea 360 originally owned the failure where `00181` printed an incorrect
initial Tower of Hanoi state before recursive post-call behavior was
observable. Prior execution repaired the source-tower initialization/value-flow
shape and later refresh proof reported `00181` plus adjacent guardrails green.
This runbook exists only to refresh that conclusion from the current tree.

## Execution Rules

- Start from generated-code and focused-test evidence, not from the historical
  failure alone.
- If the initial printed state is still correct, do not claim new in-scope work
  under this source idea.
- If `00181` fails later, classify the new first bad fact against the parked
  follow-up ideas before asking to broaden this route.
- Keep routine proof details in `todo.md`; edit this plan only if the refresh
  route itself needs correction.

## Ordered Steps

### Step 1: Refresh the Current `00181` First Bad Fact

Goal: determine whether the current tree still exposes the Hanoi starting-state
output mismatch owned by this source idea.

Primary target: `c_testsuite_aarch64_backend_src_00181_c`

Actions:

- Rebuild the default preset.
- Run the supervisor-selected focused proof for `00181` and nearby value-flow
  guardrails.
- If `00181` fails, capture the earliest observable mismatch and compare it to
  the source idea's starting-state boundary.
- If needed, inspect semantic BIR, prepared BIR, and generated AArch64 only
  enough to classify the first bad fact.

Completion check:

- `todo.md` records the exact proof command, result, and whether the first bad
  fact is in scope for idea 360.

### Step 2: Preserve Adjacent Repair Boundaries

Goal: ensure the refresh result does not regress or blur the repairs that made
the starting-state issue observable.

Primary target: focused backend guardrails for ideas 357, 358, 359, 361, and
362.

Actions:

- Include nearby guardrails selected by the supervisor for pointer-formal
  callee-saved homes, scalar formal post-call reloads, stack-preserved pointer
  formal homes, materialized pointer StoreLocal writeback, and pointer-derived
  load/address scaling.
- Treat any regression in those owners as an out-of-scope handoff unless fresh
  evidence shows it is required to explain the initial Hanoi state.

Completion check:

- `todo.md` records whether adjacent guardrails passed and names any out-of-
  scope residual that should be handled by another source idea.

### Step 3: Lifecycle Decision Handoff

Goal: leave the supervisor with a clear close, deactivate, or split decision.

Actions:

- If no in-scope starting-state failure remains, recommend deactivation or
  closure attempt under the close gate.
- If an in-scope starting-state value-flow failure remains, hand the localized
  owner to an executor packet.
- If the first bad fact belongs elsewhere, recommend the matching existing
  source idea or a new split.

Completion check:

- `todo.md` contains a concise latest-packet summary, proof result, and
  suggested next lifecycle action.
