# AArch64 Block Label Emission Ordering Refresh Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md
Activated from: parked closure-ready source idea with no active plan present

## Purpose

Refresh and, only if still current, resolve the AArch64 block label/emission
ordering owner recorded in the source idea.

## Goal

Determine whether the current tree still has an AArch64 function/block
emission failure where reachable code appears after a return/epilogue without
a correct label and branch path, then either close the idea or hand off a
freshly localized residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
general AArch64 block-ordering, label-placement, fallthrough, branch-target,
or epilogue-emission failure.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- The lifecycle note saying the source scope is closure-ready, with the
  remaining `00176` failure previously reclassified to local/formal
  frame-slot publication rather than block label/emission ordering.
- The prior active runbook for this idea if historical detail is needed from
  git history.
- Current prepared BIR, selected records, and generated AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00176_c` only as representative evidence,
  not as a testcase-specific repair target.

## Current Scope

- AArch64 prepared CFG order, labels, fallthrough boundaries, branch targets,
  and return/epilogue emission.
- Multi-block functions where a return block is followed by later reachable
  blocks in prepared order.
- `00176` `partition` only as an external representative after focused
  block-emission evidence is checked.
- Adjacent branch, return, call-publication, and selected-address guardrails
  chosen by the supervisor.

## Non-Goals

- Do not reopen recursive call argument preservation, preserved-home
  publication, caller-save reloads, or stale argument-register repairs unless
  fresh evidence reaches that exact boundary again.
- Do not reopen local/formal frame-slot publication, indexed aggregate
  address/writeback, variadic/byval publication, scalar cast publication,
  fixed-formal entry publication, broad frame layout, semantic admission,
  runner behavior, timeout policy, proof-log behavior, expectation changes,
  unsupported-classification changes, or CTest registration.
- Do not special-case `00176`, `partition`, one block id, one label suffix,
  one emitted return sequence, one branch target, one function name, or one
  emitted instruction neighborhood.

## Working Model

The historical failure was an invalid generated block layout for `00176`
`partition`: prepared metadata had a return block followed by later reachable
blocks, while generated AArch64 placed a return/epilogue before later
unlabeled executable code. The source idea says that owner was repaired and
that the later representative failure moved to scalar fixed formal-to-local
frame-slot publication. This runbook therefore starts with a refresh and only
proceeds to implementation if the block label/emission owner is live again.

## Execution Rules

- Treat `ideas/open/352_aarch64_block_label_emission_ordering.md` as the
  durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If focused block-emission guardrails and the representative remain green or
  fail only outside this source scope, request lifecycle close or
  deactivation instead of implementation work.
- If the representative fails for an out-of-scope owner, record the
  classification in `todo.md` and return for lifecycle routing rather than
  widening this plan.
- Any repair must add or preserve focused coverage for the general
  block-emission shape and keep adjacent branch, return, call-publication, and
  selected-address behavior stable.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.

## Step 1: Refresh Current First Bad Fact

Goal: determine whether AArch64 block label/emission ordering is still a live
first bad fact.

Primary target: the supervisor-selected focused representative for this idea,
normally `c_testsuite_aarch64_backend_src_00176_c` plus existing backend
contracts for AArch64 branch, return, block-label, call-publication, and
selected-address behavior.

Actions:

- Rebuild the current tree before classification.
- Run the focused representative and nearby backend coverage selected by the
  supervisor.
- If anything fails, inspect current prepared BIR, selected records, and
  generated AArch64 around the first invalid block order, missing label,
  fallthrough, branch target, or return/epilogue boundary.
- Decide whether any observed failure still shows an in-scope block
  label/emission ordering fault.

Completion check:

- `todo.md` records either a current block label/emission bad fact with
  concrete generated-code evidence, or a current out-of-scope / already-green
  classification for supervisor lifecycle routing.

## Step 2: Localize The Emission Boundary

Goal: if Step 1 finds an in-scope failure, identify exactly where prepared CFG
facts stop being preserved in generated AArch64.

Primary target: prepared block records, selected block order, label emission,
fallthrough decisions, branch target lowering, and return/epilogue placement.

Actions:

- Compare prepared block metadata, selected AArch64 records, and emitted
  assembly for the first invalid block relationship.
- Identify whether the owner is block ordering, missing label emission,
  fallthrough suppression, branch target selection, epilogue placement, or
  another CFG-to-assembly handoff with direct evidence.
- Record the smallest focused backend shape that should fail without the
  repair.

Completion check:

- The failing boundary is named in terms of prepared CFG fact, expected label
  or branch path, observed emitted assembly, and first affected consumer.

## Step 3: Repair General Block Emission Semantics

Goal: repair the localized AArch64 block-emission rule without
testcase-shaped matching.

Actions:

- Implement the smallest shared block-order, label-placement, fallthrough,
  branch-target, or epilogue-emission change needed for the localized owner.
- Add or update focused backend coverage for a multi-block AArch64 function
  where a return block and later reachable blocks must remain correctly
  labeled and reachable.
- Preserve existing branch lowering, return lowering, call publication,
  selected-address, and frame-layout behavior.

Completion check:

- Focused coverage proves reachable code cannot be emitted as unlabeled
  post-epilogue fallthrough through the repaired general rule.
- The implementation does not mention `00176`, `partition`, one block id, one
  label suffix, or one emitted instruction sequence as a control path.

## Step 4: Prove And Reclassify

Goal: prove the repaired or refreshed owner and hand off any remaining first
bad fact.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include `00176` representative proof if it remains the external witness.
- If the representative still fails, classify the new first bad fact and keep
  it separate from this block label/emission source scope.

Completion check:

- `todo.md` records the proof command and result.
- The source idea is either ready for close consideration, or the next first
  bad fact is clearly classified for lifecycle handoff.
