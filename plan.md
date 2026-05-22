# AArch64 Block Label Emission Ordering Closure Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md
Activated: 2026-05-22

## Purpose

Refresh the parked closure-ready block-label emission owner on the current
tree and leave lifecycle state ready for either closure, deactivation, or a
fresh handoff.

## Goal

Confirm that AArch64 generated function/block emission still preserves
prepared CFG label, fallthrough, branch-target, and return/epilogue semantics
for the representative multi-block return shape.

## Core Rule

Do not turn historical `00176` evidence into new implementation work unless
fresh artifacts show a current in-scope block-ordering, label-placement,
fallthrough, branch-target, or epilogue-emission first bad fact.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- `todo.md`
- Existing AArch64 branch, return, instruction-dispatch, and call-boundary
  backend tests before selecting any new proof subset.
- `test_before.log` and `test_after.log` if present, because prior close
  attempts were rejected only by strict monotonic guard behavior.

## Current Scope

- AArch64 block ordering and label placement.
- Fallthrough boundaries and branch-target preservation.
- Return/epilogue placement relative to later reachable blocks.
- External representative: `c_testsuite_aarch64_backend_src_00176_c`.
- Adjacent branch, return, instruction-dispatch, and call-boundary guardrails
  selected by the supervisor.

## Non-Goals

- Do not reopen recursive call argument preservation, local/formal frame-slot
  publication, indexed aggregate writeback, stack-preserved symbol/local
  publication, scalar cast publication, variadic/byval work, frame layout,
  runner behavior, timeout policy, proof-log policy, CTest registration,
  expectation changes, or unsupported classifications.
- Do not special-case `00176`, `partition`, one block id, one label suffix,
  one return sequence, one branch target, or one emitted instruction
  neighborhood.
- Do not claim progress through helper renames, diagnostic rewrites,
  comment-only changes, generated-text reshuffling, or proof-log movement that
  leaves reachable code after a return/epilogue without a correct label and
  branch path.

## Working Model

Idea 352 repaired the earlier generated `partition` block emission bug and is
parked as closure-ready. The latest source note says a 2026-05-22 refresh
passed 7/7, including `c_testsuite_aarch64_backend_src_00176_c`, with no live
in-scope block-label emission first bad fact. Closure was not accepted because
matching before/after logs both passed 7/7 and the close gate requires a
strict pass-count increase. This runbook is a closure-refresh route, not a
new broad implementation route.

## Execution Rules

- Keep routine packet progress and proof details in `todo.md`.
- Preserve source-idea intent; edit the source idea only for lifecycle closure,
  durable deactivation, or a necessary handoff note.
- If the old unlabeled post-epilogue reachable-code failure reappears,
  localize the shared emission boundary before implementation and add focused
  backend coverage before relying on `00176`.
- If `00176` fails for a different first bad fact, record the handoff in
  `todo.md` and stop for lifecycle routing instead of expanding this plan.
- For any code-changing packet, require build proof plus the focused subset
  selected by the supervisor; closure still requires the lifecycle close gate.

## Ordered Steps

### Step 1: Refresh Current Block-Emission Proof

Goal: establish whether the current tree still has no in-scope block-label
emission first bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00176_c` plus adjacent
AArch64 branch, return, instruction-dispatch, and call-boundary guardrails.

Actions:

- Rebuild the default preset.
- Run the supervisor-selected focused proof for the block-emission owner.
- If any test fails, inspect only enough semantic BIR, prepared records, and
  generated AArch64 to identify the earliest observable first bad fact.
- Classify the first bad fact against this source idea before proposing code
  changes.

Completion check:

- `todo.md` records the exact proof command, pass/fail result, and whether the
  first bad fact is in scope for idea 352.

### Step 2: Repair Only If A Fresh In-Scope Failure Exists

Goal: repair a current, localized AArch64 block-emission semantic regression.

Primary target: the concrete block-ordering, label-placement, fallthrough,
branch-target, or epilogue-emission boundary found in Step 1.

Actions:

- Localize where generated emission diverges from prepared CFG semantics.
- Repair the smallest general emission rule at that boundary.
- Add or update focused backend coverage for a multi-block function where a
  return block is followed by later reachable blocks in prepared order.
- Keep call preservation, local/formal publication, selected-address, and
  frame-layout owners out of this repair unless fresh evidence requires a
  separate lifecycle split.

Completion check:

- Focused backend coverage proves the repaired emission behavior.
- `00176` advances past the block-label emission failure or is reclassified by
  a new first bad fact.

### Step 3: Closure Or Handoff Decision

Goal: leave lifecycle state consistent with the current evidence.

Actions:

- If Step 1 remains green and no in-scope first bad fact exists, request the
  plan-owner close path to run the required regression guard for archival
  closure.
- If the close gate rejects closure without a live in-scope failure, leave a
  parked/deactivation result instead of inventing implementation work.
- If a different first bad fact is found, record a concise handoff target in
  `todo.md`.
- If an in-scope repair lands, run the matching before/after regression guard
  required for closure consideration.

Completion check:

- The source idea is closed through the close gate, parked with the guard
  reason preserved, or handed off to a separate owner without source-scope
  drift.
