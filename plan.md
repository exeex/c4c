# AArch64 Block Label Emission Ordering Refresh Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md

## Purpose

Reactivate the closure-ready block label/emission ordering idea long enough to
confirm whether any in-scope AArch64 block-ordering first bad fact remains.

## Goal

Prove the current tree still preserves prepared basic-block control-flow order
for AArch64 functions where a return block is followed by later reachable
blocks, or localize and repair the first renewed block emission failure.

## Core Rule

Do not treat `00176`, `partition`, one label suffix, or one emitted epilogue
neighborhood as the fix target. Any repair must preserve general AArch64 block
emission semantics from prepared CFG facts.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- AArch64 block emission, branch, return, and selected instruction dispatch
  code paths touched by the previous repair
- Existing focused tests for AArch64 branch labels, return emission,
  instruction dispatch, call-boundary guardrails, and
  `c_testsuite_aarch64_backend_src_00176_c`

## Current Scope

- Refresh the current first bad fact for the source idea.
- Keep the check focused on block ordering, label placement, fallthrough
  boundaries, branch targets, and return/epilogue placement.
- If the old owner remains absent, hand the active state back for close
  decision rather than broadening into another AArch64 residual.

## Non-Goals

- Do not reopen recursive call argument preservation, local/formal frame-slot
  publication, indexed aggregate writeback, variadic/byval publication, scalar
  cast publication, frame-slot layout, or runner/expectation policy without a
  fresh first-bad-fact split.
- Do not weaken expectations, unsupported classifications, timeout behavior,
  proof-log policy, CTest registration, or external test contracts.
- Do not edit the source idea unless the lifecycle decision itself requires a
  durable status note.

## Working Model

The previous repair should already keep later reachable blocks labeled and
reachable instead of emitting executable code after a return/epilogue without a
valid branch path. The current runbook is therefore a refresh-first runbook:
prove no in-scope block emission failure remains, then let the plan owner
decide closure; only move into repair work if fresh generated-code evidence
reopens this exact owner.

## Execution Rules

- Start from generated/prepared evidence, not from source filename matching.
- Use focused backend coverage before relying on the external `00176`
  representative.
- Keep any new repair behavior-preserving for adjacent branch, return,
  call-publication, and selected-address guardrails.
- If the first bad fact belongs to another owner, stop and report the handoff
  candidate instead of absorbing it into this plan.

## Step 1: Refresh Block Emission Evidence

Goal: determine whether the current tree still has an in-scope AArch64 block
label/emission ordering failure.

Primary target: focused AArch64 block/branch/return guardrails plus
`c_testsuite_aarch64_backend_src_00176_c`.

Actions:

- Rebuild the default preset.
- Run the focused proof selected by the supervisor for block ordering,
  branch/return behavior, instruction dispatch, call-boundary guardrails, and
  `00176`.
- If the proof is green, record that no current block label/emission ordering
  first bad fact is live.
- If the proof is red, inspect generated and prepared artifacts enough to
  classify the first bad fact.

Completion check:

- Green proof with no in-scope first bad fact, or a concise classification of
  the renewed first bad fact and its evidence boundary.

## Step 2: Repair Renewed Block Ordering Failure

Goal: repair only if Step 1 proves the block label/emission ordering owner is
live again.

Primary target: AArch64 function/block emission order, label placement,
fallthrough boundaries, branch targets, or return/epilogue placement.

Actions:

- Localize where generated assembly diverges from prepared CFG semantics.
- Repair the general emission rule rather than a `00176` or label-name
  shortcut.
- Add or update focused backend coverage for the multi-block return-then-
  reachable-block shape.
- Preserve adjacent branch, return, call-publication, and selected-address
  guardrails.

Completion check:

- Focused backend coverage proves the repaired block ordering shape, and
  `00176` advances past the block emission failure or is reclassified by a new
  first bad fact.

## Step 3: Close Or Handoff Decision

Goal: leave the lifecycle in a state the supervisor can close or redirect.

Actions:

- If Step 1 is green and no in-scope first bad fact exists, return the active
  state to the supervisor for a plan-owner close decision.
- If Step 2 repairs the owner, report proof and any residual first bad fact.
- If the first bad fact is out of scope, identify the owner and recommend a
  lifecycle split or activation switch.

Completion check:

- The next lifecycle action is explicit: close candidate, repaired-with-proof,
  or out-of-scope handoff.
