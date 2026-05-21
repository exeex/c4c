# AArch64 Block Label Emission Ordering Plan

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md
Activated After: ideas/open/349_aarch64_recursive_call_argument_preservation.md

## Purpose

Repair AArch64 function/block emission so generated labels, fallthrough
boundaries, and return/epilogue placement preserve prepared basic-block
control-flow semantics.

Goal: prevent later reachable code from being emitted after a return block's
epilogue without a correct label and branch path.

Core Rule: repair general AArch64 block emission semantics; do not
special-case `00176`, `partition`, one block id, one label suffix, or one
emitted return neighborhood.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- `ideas/open/349_aarch64_recursive_call_argument_preservation.md` for the
  split boundary: stale final `swap` call publication is no longer the current
  `00176` first bad fact.
- AArch64 function emission, basic-block ordering, label placement,
  fallthrough handling, branch target lowering, and return/epilogue emission
  before editing.

## Current Targets

- Current representative: `c_testsuite_aarch64_backend_src_00176_c`.
- First bad fact family: prepared `partition` metadata has `block_3` as a
  return block followed by later blocks, while generated assembly prints the
  `block_3` return/epilogue before later unlabeled executable `swap` code.
- Focused backend coverage for a multi-block AArch64 function where a return
  block and later reachable block sequence must remain correctly labeled and
  reachable.
- Adjacent AArch64 branch, return, call-publication, and selected-address
  guardrails chosen by the supervisor.

## Non-Goals

- Do not reopen recursive call argument preservation, preserved-home
  publication, caller-save reloads, or stale argument-register repairs unless
  fresh evidence reaches that exact boundary again.
- Do not take over the current `00181` stack-preserved symbol/local
  publication crash.
- Do not broaden into indexed aggregate address/writeback, variadic/byval
  publication, scalar cast publication, fixed-formal entry publication,
  frame-slot layout, runner behavior, timeout policy, proof-log behavior,
  expectation changes, unsupported-classification changes, or CTest
  registration.
- Do not use filename-only, function-name-only, block-number-only,
  label-name-only, diagnostic-string-only, c-testsuite-number-specific, or
  emitted-text-only fixes.

## Working Model

- Prepared CFG order and terminators describe which blocks are reachable and
  how control transfers between them.
- A return block's epilogue must not be emitted as if it were the terminal
  function tail when later reachable blocks still need labels and branch paths.
- The repair should preserve AArch64 block labels and branches according to
  prepared CFG facts instead of relying on incidental generated order.

## Execution Rules

- Start from prepared/generated evidence for the current `00176` boundary
  before modifying code.
- Prefer a shared block ordering, label placement, fallthrough, branch-target,
  or epilogue-emission repair over emitted-instruction pattern matching.
- Preserve existing AArch64 return lowering, branch lowering, call argument
  publication, selected-address handling, and frame-layout behavior.
- Treat any fix that recognizes only `00176`, `partition`, one block id, one
  label suffix, or one emitted return sequence as route drift.
- Escalate to a separate source idea if fresh evidence reaches stack/local
  publication, indexed aggregate writeback, call preservation, variadic/byval
  publication, frame layout, semantic admission, or unrelated runtime mismatch
  work.

## Steps

### Step 1: Localize Block Emission Boundary

Goal: identify the exact AArch64 owner that emits the `partition` return block
and later reachable blocks in an invalid label/order relationship.

Primary target: prepared block records, selected block order, label emission,
fallthrough decisions, branch target lowering, and return/epilogue placement.

Actions:

- Trace `00176` `partition` from prepared block metadata through selected
  AArch64 block emission.
- Identify where `block_3`'s return/epilogue becomes placed before later
  unlabeled executable code from `block_4`/`block_5`.
- Decide whether the owner is block ordering, missing label emission,
  fallthrough suppression, branch target selection, epilogue placement, or
  another CFG-to-assembly handoff with direct evidence.

Completion check:

- `todo.md` records the concrete first bad boundary, representative
  prepared/generated evidence, and the narrow proof subset for the first
  implementation packet.

### Step 2: Add Focused Multi-Block Emission Coverage

Goal: prove return blocks and later reachable blocks retain correct labels and
control-flow paths without depending only on c-testsuite output.

Actions:

- Add or extend focused backend coverage for a multi-block AArch64 function
  where a return block is followed in prepared order by later reachable blocks.
- Assert generated assembly has labels and branches that keep reachable code
  out of unlabeled post-epilogue fallthrough.
- Keep test contracts independent of `00176`, `partition`, one block id, one
  label suffix, or one emitted instruction neighborhood.

Completion check:

- Focused coverage fails without the repair or an existing focused test is
  identified that already exposes the invalid block label/emission ordering.

### Step 3: Repair AArch64 Label/Order Emission

Goal: make generated AArch64 preserve prepared CFG reachability and
return/epilogue boundaries when emitting blocks.

Actions:

- Repair the localized owner from Step 1 in the smallest shared block-order,
  label-placement, fallthrough, branch-target, or epilogue-emission helper.
- Ensure later reachable blocks receive labels and branch paths instead of
  appearing as unlabeled code after a return/epilogue.
- Preserve existing branch lowering, return lowering, call publication,
  selected-address, and frame-layout behavior.

Completion check:

- Focused coverage from Step 2 passes, and supervisor-selected adjacent
  branch/return/call/publication guardrails show no regression.

### Step 4: Prove Representative And Reclassify Residuals

Goal: verify `00176` advances past the unlabeled post-epilogue `partition`
failure and classify any new first bad fact.

Actions:

- Run the supervisor-selected external proof for `00176` after focused proof
  is stable.
- Confirm `00176` advances past the current block label/emission ordering
  failure.
- If `00176` remains red, reclassify the new first bad fact instead of
  widening this plan by assumption.

Completion check:

- `todo.md` records whether `00176` passed, advanced past the block
  label/emission ordering failure, or exposed a new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope block emission work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent branch, return, call-publication, and selected-address
  guardrails remain stable.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining block emission boundary and
  exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 352 or a clear
  remaining AArch64 block label/emission route that does not broaden beyond
  the source idea.
