# AArch64 Block Label Emission Ordering Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md
Activated: 2026-05-22

## Purpose

Confirm whether the parked closure-ready block-label emission owner is still
satisfied on the current tree, and either prepare it for lifecycle closure or
classify any fresh first bad fact without widening the source scope.

## Goal

Prove that AArch64 generated function/block emission preserves prepared CFG
label, fallthrough, branch-target, and return/epilogue semantics for the
representative multi-block return shape.

## Core Rule

Do not change implementation code or test expectations just to make idea 352
look closed. If current proof exposes a new failure, localize the first bad
fact and hand it to the correct owner instead of expanding this runbook.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- `todo.md`
- Existing AArch64 branch, return, instruction-dispatch, and call-boundary
  backend tests before selecting any new proof subset.

## Current Scope

- AArch64 block ordering and label placement.
- Fallthrough boundaries and branch-target preservation.
- Return/epilogue placement relative to later reachable blocks.
- External representative: `c_testsuite_aarch64_backend_src_00176_c`.

## Non-Goals

- Recursive call argument preservation, local/formal publication, indexed
  aggregate writeback, frame layout, scalar cast publication, variadic/byval
  work, runner policy, timeout policy, proof-log policy, CTest registration,
  expectation changes, or unsupported-classification changes.
- Special-casing `00176`, `partition`, one block id, one label suffix, one
  return sequence, one branch target, or one emitted instruction neighborhood.

## Working Model

Idea 352 is parked as closure-ready. The latest source note says focused proof
passed 7/7 on 2026-05-22 and no live in-scope first bad fact remained, but the
strict close gate did not accept archival closure because before/after pass
counts did not increase. This runbook is therefore a verification and closure
eligibility route, not a new broad implementation route.

## Execution Rules

- Treat `todo.md` as the live packet state.
- Keep source-idea edits out of routine execution unless lifecycle closure or
  a durable handoff note is required.
- If the old unlabeled post-epilogue reachable-code failure reappears, repair
  the general emission boundary and add focused backend coverage before using
  `00176` as proof.
- If `00176` fails for a different first bad fact, stop and record the
  lifecycle handoff target instead of folding it into this plan.
- For any code-changing packet, require build proof plus the narrow backend
  subset selected by the supervisor.

## Steps

### Step 1: Refresh Current Block-Emission Proof

Goal: establish whether the current tree still has no in-scope block-label
emission first bad fact.

Actions:

- Rebuild the default preset.
- Run a focused subset covering `00176` plus adjacent branch, return,
  instruction-dispatch, and call-boundary guardrails.
- Inspect any failure for the exact first bad fact before proposing code
  changes.

Completion check:

- The subset passes, or the first failing artifact is classified as either
  the old idea 352 owner or an out-of-scope handoff.

### Step 2: If In-Scope Failure Reappears, Repair Semantically

Goal: repair only a real AArch64 block-emission semantic regression.

Actions:

- Localize the boundary that loses prepared CFG semantics.
- Repair label placement, block order, fallthrough separation, branch target
  emission, or return/epilogue placement generally.
- Add or update focused backend coverage for the multi-block return followed
  by later reachable block shape.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- `00176` advances past the block-label emission failure or is reclassified
  by a new first bad fact.

### Step 3: Closure Or Handoff Decision

Goal: leave lifecycle state consistent with the current evidence.

Actions:

- If Step 1 remains green and no in-scope owner is live, ask the plan-owner
  close path to run the required regression guard for archival closure.
- If a different first bad fact is found, record a concise handoff in
  `todo.md` and let the supervisor route lifecycle work to the proper idea.
- If a repaired in-scope failure lands, run the matching before/after
  regression guard required for closure consideration.

Completion check:

- The source idea is either closed through the close gate, left parked with a
  documented guard reason, or handed off to a separate owner with no source
  scope drift.
