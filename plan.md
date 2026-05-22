# AArch64 Block Label Emission Ordering Runbook

Status: Active
Source Idea: ideas/open/352_aarch64_block_label_emission_ordering.md

## Purpose

Refresh and, if needed, repair the AArch64 block label/emission ordering owner
for functions where prepared reachable blocks can appear after a return block.

Goal: prove the owner remains absent/closure-ready, or localize and repair a
fresh block-ordering first bad fact without reopening adjacent owners.

## Core Rule

Do not special-case `00176`, `partition`, one block id, one label suffix, or
one emitted return sequence. Treat any failure as a general AArch64 CFG block
ordering, label placement, fallthrough, branch-target, or epilogue-emission
problem only if generated-code evidence proves that owner is live.

## Read First

- `ideas/open/352_aarch64_block_label_emission_ordering.md`
- AArch64 block/function emission code only after Step 1 proves a live
  in-scope first bad fact.
- Prior related boundaries only for comparison:
  `ideas/open/349_aarch64_recursive_call_argument_preservation.md` and
  `ideas/open/353_aarch64_local_formal_frame_slot_publication.md`.

## Current Scope

- Prepared basic-block ordering and generated AArch64 label placement.
- Fallthrough and branch boundary emission for reachable blocks after return
  blocks in prepared order.
- Return/epilogue placement when it can make later reachable code unreachable
  or unlabeled.
- Focused proof around `00176` plus adjacent AArch64 branch, return,
  instruction-dispatch, and call-boundary guardrails.

## Non-Goals

- Recursive call argument preservation, preserved-home publication, or
  caller-save reload work from idea 349.
- Local/formal frame-slot publication work from idea 353.
- Indexed aggregate address/writeback, variadic/byval publication, scalar
  cast publication, fixed-formal entry publication, broad frame layout,
  runner behavior, timeout policy, CTest registration, expectation changes, or
  unsupported-classification changes.
- Any expectation rewrite or weaker test contract used as evidence of
  progress.

## Working Model

The source idea is parked because earlier refresh proof showed no live
block-label/emission-ordering failure and closure was rejected only by the
strict monotonic regression-log gate. This activation should first re-check the
current tree. Implementation work is justified only if fresh evidence shows
reachable generated AArch64 code is still misplaced after a return/epilogue or
missing the label/branch path required by prepared CFG semantics.

## Execution Rules

- Start with generated/prepared evidence before editing code.
- Keep proof focused until a live owner is classified.
- If the first bad fact belongs to local/formal publication, recursive call
  preservation, indexed writeback, or another adjacent owner, do not repair it
  under this plan; record the handoff in `todo.md` for supervisor routing.
- For any code-changing step, run `cmake --build --preset default` before the
  delegated focused CTest subset.
- Keep `todo.md` as the packet state; do not rewrite the source idea unless
  lifecycle deactivation or closure needs a durable note.

## Steps

### Step 1: Refresh the Current First Bad Fact

Goal: determine whether the block label/emission ordering owner is live in the
current tree.

Actions:

- Build the current tree.
- Run the focused `00176` and adjacent AArch64 CFG/emission guardrail subset
  selected by the supervisor.
- If any test fails, inspect prepared and generated AArch64 artifacts for the
  first bad fact before deciding ownership.
- Confirm whether generated labels, branch targets, fallthrough boundaries,
  and return/epilogue placement still match prepared CFG semantics.

Completion check:

- Either the focused subset passes with no in-scope owner to repair, or
  `todo.md` records the first failing test, first bad fact, and evidence that
  the failure is in or out of this source idea's scope.

### Step 2: Repair a Live Block Emission Owner

Goal: fix only a proved AArch64 block ordering, label placement, fallthrough,
branch-target, or epilogue-emission defect.

Actions:

- Localize the emission boundary that loses prepared CFG semantics.
- Repair the general lowering/emission rule without naming `00176`,
  `partition`, or one block/label pattern.
- Add or update focused backend coverage for a multi-block function where a
  return block is followed by later reachable blocks in prepared order.
- Preserve adjacent branch, return, call-publication, and selected-address
  behavior.

Completion check:

- Focused coverage fails without the repair and passes with it, or existing
  focused coverage is shown to cover the repaired rule.
- `c_testsuite_aarch64_backend_src_00176_c` advances past any current
  block-label/emission-ordering first bad fact or is reclassified by a new
  first bad fact.

### Step 3: Classify Exit State

Goal: leave the supervisor with an unambiguous lifecycle decision.

Actions:

- If Step 1 finds no live in-scope failure, record that the idea remains
  parked/closure-ready pending an acceptance-grade regression guard.
- If Step 2 repairs an owner, record proof, residual failures, and any
  out-of-scope first bad fact.
- Do not close this source idea from executor work alone; closure remains a
  plan-owner decision requiring the close gate.

Completion check:

- `todo.md` identifies whether the runbook should be closed, deactivated, or
  handed off to a separate source idea.
