# AArch64 Block Label Emission Ordering Closure Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md

## Purpose

Reconfirm that the AArch64 block label/emission ordering owner remains
satisfied, then prepare the active lifecycle state for a close decision.

## Goal

Establish whether any current first bad fact still belongs to AArch64
basic-block ordering, label placement, fallthrough boundary handling, or
return/epilogue emission.

## Core Rule

Do not reopen adjacent AArch64 owners unless fresh generated-code evidence
shows the first bad fact is still reachable through this idea's block emission
scope.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- Any current `todo.md` packet notes before execution begins
- Generated AArch64 for `c_testsuite_aarch64_backend_src_00176_c` if the
  representative regresses

## Current Targets

- AArch64 block ordering, label placement, fallthrough boundaries, branch
  targets, and return/epilogue placement.
- Focused multi-block cases where a return block is followed by later
  reachable blocks in prepared order.
- Representative proof around `00176` only after focused backend evidence
  stays aligned with the shared owner.

## Non-Goals

- Do not work on recursive call argument preservation, preserved-home
  publication, caller-save reloads, or stale argument-register fixes.
- Do not work on local/formal frame-slot publication, indexed aggregate
  address/writeback, variadic/byval publication, scalar casts, frame layout,
  runner behavior, timeout policy, CTest registration, unsupported
  classification, expectations, or proof-log policy.
- Do not special-case `00176`, `partition`, a block id, a label suffix, a
  branch target, or one emitted instruction neighborhood.

## Working Model

The source idea is closure-ready. Earlier repair work preserved the generated
`.LBB90_6` / `.LBB90_7` branch-label path, and a 2026-05-22 refresh found no
live in-scope first bad fact. The remaining lifecycle problem is archival
closure proof, not known implementation work.

## Execution Rules

- Start with read-only classification before editing implementation files.
- If the focused proof is green and no in-scope first bad fact appears, report
  the result for plan-owner close handling rather than expanding the runbook.
- If a fresh in-scope failure appears, localize it to the concrete block-order,
  label-placement, fallthrough, branch-target, or epilogue-emission boundary
  before proposing code changes.
- Any code-changing step must keep focused backend proof and adjacent AArch64
  branch, return, call-publication, and selected-address guardrails stable.

## Steps

### Step 1: Refresh In-Scope Evidence

Goal: Confirm whether the current tree still has any AArch64 block
label/emission ordering first bad fact.

Primary target: focused AArch64 block-emission and `00176` representative
evidence.

Actions:

- Build the current tree.
- Run the focused proof selected by the supervisor for this closure-refresh
  packet.
- If `00176` or a focused block-emission guard fails, inspect generated and
  prepared artifacts only enough to decide whether the first bad fact is in
  this idea's scope.
- Record the proof command, pass/fail counts, and classification result in
  `todo.md`.

Completion check:

- `todo.md` names the exact proof command and states either that no in-scope
  first bad fact remains, or identifies the concrete block-emission boundary
  requiring Step 2 implementation.

### Step 2: Repair Only If A Fresh Block-Emission Failure Exists

Goal: Repair a current, localized block-emission ordering failure if Step 1
finds one.

Primary target: the localized AArch64 block order, label placement,
fallthrough, branch-target, or return/epilogue-emission path.

Actions:

- Make the smallest semantic repair that preserves prepared CFG ordering in
  generated AArch64.
- Add or update focused backend coverage for the failing multi-block shape.
- Do not broaden into adjacent owners without a lifecycle split.
- Re-run the Step 1 focused proof and any supervisor-selected adjacent
  guardrails.

Completion check:

- Focused coverage proves the repaired block-emission behavior, `00176`
  advances or passes, and adjacent AArch64 branch, return, call-publication,
  and selected-address guardrails remain stable.

### Step 3: Prepare Lifecycle Close Decision

Goal: Hand the source idea back for closure if the block-emission scope remains
satisfied.

Primary target: canonical lifecycle state and close-gate evidence.

Actions:

- Ensure `todo.md` summarizes the latest Step 1 or Step 2 result.
- If no in-scope first bad fact remains, request plan-owner closure handling
  with matching `test_before.log` and `test_after.log` evidence available or
  explicitly missing.
- If close is rejected only by regression-guard monotonicity, keep this source
  idea parked/open rather than expanding implementation scope.

Completion check:

- The supervisor can tell whether to close, deactivate, or keep the idea
  parked based on `todo.md`, canonical logs, and the source idea's acceptance
  criteria.
