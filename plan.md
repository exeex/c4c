# AArch64 Codegen Prepared Boundary Recovery Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-prepared-boundary-recovery.md

## Purpose

Recover the ownership boundary between Prepared/MIR contract interpretation and
AArch64 instruction selection before later header or `.cpp` consolidation work.

## Goal

Make AArch64 codegen consume a clearer target contract by auditing the
`dispatch_*` and `calls_*` families and moving one narrow generic
Prepared/MIR responsibility out of `src/backend/mir/aarch64/codegen`, or
documenting why the first move should be deferred.

## Core Rule

Do not weaken tests, expectations, backend contracts, or add target-specific
special cases outside AArch64 to make the boundary move pass.

## Read First

- `ideas/open/aarch64-codegen-prepared-boundary-recovery.md`
- `src/backend/mir/aarch64/codegen/dispatch_*.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_*.hpp`
- `src/backend/mir/aarch64/codegen/calls_*.cpp`
- `src/backend/mir/aarch64/codegen/calls_*.hpp`
- `src/backend/prealloc/module.hpp`

## Current Targets

- `src/backend/mir/aarch64/codegen/dispatch_*`
- `src/backend/mir/aarch64/codegen/calls_*`
- generic Prepared facts or MIR support near `src/backend/prealloc/`,
  `src/backend/bir/`, or shared MIR support when the audit proves the
  responsibility belongs there

## Non-Goals

- Do not merge `.cpp` files in this runbook.
- Do not perform header-family consolidation here.
- Do not start the C++ family consolidation idea.
- Do not move responsibilities earlier unless the fact is target-independent or
  useful beyond one AArch64 local helper.
- Do not claim progress through expectation rewrites or unsupported-path
  downgrades.

## Working Model

Classify each inspected helper/function into one bucket:

- target-specific instruction selection or printing; stays in AArch64
- generic Prepared contract interpretation; candidate for `prealloc` or shared
  MIR support
- missing Prepared fact; candidate to compute earlier in `bir` or `prealloc`
- local publication/helper glue; candidate for later consolidation after the
  ownership boundary is clearer

## Execution Rules

- Keep source-idea edits unnecessary unless the durable intent changes.
- Record the living classification in `todo.md` or a review artifact; do not
  bury it only in chat.
- Prefer one low-risk move with a focused proof over broad churn.
- Keep changes behavior-preserving unless the source idea explicitly justifies
  a contract improvement.
- Run a fresh build or targeted backend proof for any code-changing step.
- Escalate to supervisor/reviewer if the only available move would be
  testcase-shaped or would hide target-independent logic in AArch64.

## Ordered Steps

### Step 1: Audit Dispatch and Calls Responsibilities

Goal: Produce a classification of current `dispatch_*` and `calls_*`
responsibilities before moving code.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_*`
- `src/backend/mir/aarch64/codegen/calls_*`

Actions:

- Inspect declarations and definitions in the target families.
- Identify helpers that interpret value homes, storage encodings, edge copies,
  call boundary moves, publication effects, or diagnostics.
- Classify each meaningful helper/function using the working-model buckets.
- Record the classification in `todo.md` or a review artifact with enough
  detail for a later reviewer to reject testcase-shaped drift.
- Select one narrow low-risk generic responsibility as the candidate move, or
  explain why no safe first move should happen in this runbook.

Completion check:

- `todo.md` or a review artifact contains the classification and candidate
  move/defer rationale.
- The selected candidate is not a named-case shortcut, expectation rewrite, or
  unsupported downgrade.

### Step 2: Move One Generic Prepared/MIR Responsibility

Goal: Move the selected generic responsibility out of AArch64 codegen while
preserving target behavior.

Primary target:

- The candidate identified in Step 1, plus the narrow shared/prealloc/BIR/MIR
  destination justified by the classification.

Actions:

- Add or expose the target-independent fact/helper at the proper layer.
- Replace the AArch64-local interpretation with consumption of the clearer
  contract.
- Keep target-specific instruction selection and printing in AArch64.
- Avoid broad renames or file consolidation unrelated to the boundary move.
- Update only tests that require stronger supported behavior; do not weaken
  expectations.

Completion check:

- The generic responsibility no longer lives primarily in
  `src/backend/mir/aarch64/codegen`.
- AArch64 codegen consumes a clearer Prepared/MIR contract for that
  responsibility.
- Fresh build or targeted backend tests pass and are recorded in `todo.md`.

### Step 3: Boundary Review and Handoff

Goal: Decide whether the active source idea is satisfied or needs another
runbook before dependent header consolidation starts.

Actions:

- Compare the final diff and classification against the source idea acceptance
  criteria.
- Confirm no `.cpp` merge or header-family consolidation slipped into this
  runbook.
- Identify any remaining generic Prepared/MIR responsibilities that should
  become a follow-up runbook or open idea.
- Recommend whether Header Family Consolidation is unblocked.

Completion check:

- The supervisor can tell whether the source idea is complete, blocked, or
  needs another runbook.
- The next lifecycle action is explicit in `todo.md`.
