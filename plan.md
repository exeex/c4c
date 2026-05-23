# AArch64 Block Label Emission Ordering Refresh Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md
Activated from: remaining open closure-ready candidate after ideas 360, 362, and 326 were closed

## Purpose

Refresh the parked AArch64 block label/emission ordering owner and determine
whether the current tree still has any in-scope CFG emission failure before
attempting closure or handing off a residual.

## Goal

Prove whether `00176` or adjacent AArch64 branch/return/control guardrails
still expose the block-ordering, label-placement, fallthrough, or
return/epilogue placement failure owned by this source idea.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows
the current first bad fact belongs to AArch64 block label/emission ordering.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- The lifecycle handoff notes in ideas 349 and 353 when interpreting any
  `00176` residual.
- Generated prepared-BIR and AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00176_c` only as evidence, not as a
  testcase-specific repair target.

## Current Scope

- `c_testsuite_aarch64_backend_src_00176_c`
- AArch64 block traversal, branch compare/control, return lowering, and
  instruction dispatch guardrails
- Generated assembly where a prepared return block is followed by later
  reachable blocks
- A fresh proof record suitable for supervisor closure or handoff

## Non-Goals

- Do not reopen recursive call argument preservation, preserved-home
  publication, caller-save reloads, local/formal frame-slot publication,
  indexed aggregate writeback, variadic/byval publication, scalar cast
  publication, or broad frame layout without a lifecycle split.
- Do not change implementation files during lifecycle activation.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00176`, `partition`, one block id, one label suffix, one
  branch target, one return sequence, or one emitted instruction neighborhood.

## Working Model

Idea 352 originally owned an AArch64 emission-ordering failure where generated
`partition` code placed later reachable code after a return/epilogue without a
correct reachable label path. Prior execution repaired that owner and later
refresh proof reported `00176` plus adjacent branch, return, dispatch, and
call-boundary guardrails green. The file remains under `ideas/open/` because
closure was rejected by the strict monotonic regression guard, not because the
latest source note identifies a live implementation defect.

## Execution Rules

- Start from fresh proof and generated-code evidence, not from the historical
  `partition` failure alone.
- If `00176` passes and focused guardrails remain green, request lifecycle
  closure rather than implementation work.
- If `00176` fails, inspect only enough generated BIR, prepared BIR, and
  AArch64 to identify the earliest observable first bad fact.
- Classify any residual against the source idea before delegating code work.
- Treat failures in local/formal frame-slot publication, indexed aggregate
  writeback, or call preservation as out-of-scope handoffs unless fresh
  evidence proves they are required to repair block label/emission ordering.
- Preserve canonical proof-log names when an executor writes logs:
  `test_before.log` and `test_after.log`.

## Step 1: Refresh The Current `00176` Block-Emission First Bad Fact

Goal: determine whether the current tree still exposes the block-ordering,
label-placement, fallthrough, branch-target, or epilogue-emission failure owned
by this source idea.

Primary target: `c_testsuite_aarch64_backend_src_00176_c`

Actions:

- Rebuild the default preset.
- Run the supervisor-selected focused proof for `00176` and nearby AArch64
  branch, return, dispatch, and call-boundary guardrails.
- If `00176` fails, capture the earliest observable mismatch, segfault,
  timeout, compile diagnostic, or generated-code contradiction.
- Inspect generated artifacts only enough to classify whether the failure is
  an in-scope block label/emission ordering owner.

Completion check:

- `todo.md` records the exact proof command, result, and whether any current
  first bad fact remains in scope for idea 352.

## Step 2: Preserve Adjacent Repair Boundaries

Goal: ensure the refresh does not blur the boundaries around the prior
call-preservation and local/formal publication chain.

Primary target: guardrails for branch/control lowering, return lowering,
instruction dispatch, call-boundary publication, and `00176`.

Actions:

- Confirm the branch/control and return-lowering guardrails selected by the
  supervisor remain green.
- Treat stale recursive-call argument reuse and scalar fixed-formal local slot
  reads as out-of-scope owners unless they are preceded by a current in-scope
  block-emission contradiction.
- Do not adjust expectations or unsupported classifications to satisfy the
  refresh.

Completion check:

- `todo.md` names any adjacent guardrail failure and whether it should be
  handled by another source idea.

## Step 3: Lifecycle Decision Handoff

Goal: leave the supervisor with a clear close, deactivate, split, or executor
packet decision.

Actions:

- If no in-scope block label/emission ordering failure remains, recommend a
  closure attempt under the current close gate.
- If an in-scope failure remains, hand the localized owner to an executor
  packet with focused coverage and proof requirements.
- If the first bad fact belongs elsewhere, recommend the matching existing
  source idea or a new split.

Completion check:

- `todo.md` contains a concise latest-packet summary, proof result, and
  suggested next lifecycle action.
