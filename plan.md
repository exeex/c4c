# RV64 Register-Source Stack-Destination Prepared Move Bundles Runbook

Status: Active
Source Idea: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Activated From: post-516 return to parent after `ideas/closed/516_rv64_multi_source_prepared_move_bundle_classification.md`

## Purpose

Return to idea 514 after the split multi-source producer/classification owner
closed, and finish the remaining register-source to stack-destination
prepared move-bundle decision without reopening the stack-to-stack materializer
from idea 513.

## Goal

Either materialize the owned register-source to stack-destination prepared move
shape from explicit prepared facts, or reject/reclassify it with a precise
owner-specific diagnostic that does not rely on the old stack-to-stack
explanation.

## Core Rule

Consume only explicit prepared move-bundle, source-home, destination-home,
storage-plan, width, bank, sequencing, and authority facts. Do not infer
register or stack locations from row names, argument indexes, source order,
ABI formulas, local names, or raw BIR text.

## Read First

- ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
- ideas/closed/516_rv64_multi_source_prepared_move_bundle_classification.md
- ideas/closed/513_rv64_stack_to_stack_prepared_move_materialization.md
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/mir/riscv/
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/

## Current Targets

- Primary remaining representative: `src/pr27073.c`
- Post-516 regression representative: `src/20010518-1.c`
- Current rejection family: `unsupported_move_bundle_target_shape`
- Owning layer: RV64 object emission consuming prepared move-bundle authority,
  with producer classification checked before materialization
- Required authority:
  - prepared move bundle identifies the value movement to emit
  - source home or storage plan explicitly identifies a register source
  - destination home explicitly identifies a stack destination
  - scalar type, width, bank, move count, and sequencing are coherent enough
    for RV64 object emission to materialize without guessing

## Non-Goals

- Do not reopen idea 513's coherent same-scalar stack-slot to stack-slot
  materializer.
- Do not reopen idea 516's multi-source producer/classification route unless
  post-516 proof shows a regression in that closed contract.
- Do not infer source or destination locations from representative row names,
  argument indexes, source order, ABI formulas, local names, or textual BIR
  matching.
- Do not silently accept multi-move bundles by dropping moves, picking one
  source by order, or merging moves without an explicit contract.
- Do not broaden into general parallel-copy scheduling, cycle resolution, or
  unrelated conversion work.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, reason strings, or scan accounting as proof of capability
  progress.

## Working Model

- Idea 513 closed the coherent same-scalar stack-slot to stack-slot move
  materializer and deliberately left register-source or ambiguous shapes
  fail-closed.
- Idea 514's retired runbook split the ambiguous multi-source
  `src/20010518-1.c` owner into idea 516.
- Idea 516 closed by making the multi-source bundle reject before RV64 object
  emission consumes an ambiguous `authority=none`, `move_count=2`,
  `parallel_copy=no` bundle.
- The remaining idea 514 question is the minimal single register-source to
  stack-destination shape, especially `src/pr27073.c`, plus a post-516 check
  that the multi-source regression representative still obeys the closed
  producer/classifier boundary.

## Execution Rules

- Start by reproducing the current `src/pr27073.c` and `src/20010518-1.c`
  facts and record them in `todo.md`.
- Keep the single-move and multi-source evidence separate. Treat
  `src/20010518-1.c` as a post-516 regression guard unless it clearly exposes
  a new producer/classification gap.
- Keep implementation in RV64 object emission only when producer facts are
  already explicit and coherent. If producer classification is wrong or
  incomplete, stop and record the producer-owned blocker.
- Add focused backend coverage for any accepted register-source to
  stack-destination path and for adjacent rejected shapes.
- For code-changing steps, run a fresh build, focused representative proof,
  focused backend coverage, and the relevant backend subset chosen by the
  supervisor.

## Step 1: Rebuild Post-516 Register-Source Evidence

Goal: Reconfirm the current representative facts after idea 516 closed and
identify the exact remaining idea 514 owner.

Actions:

- Reproduce focused dumps or probes for `src/pr27073.c`.
- Reproduce a focused post-516 check for `src/20010518-1.c`.
- Record each prepared move bundle, source home, destination home, storage
  plan, scalar type, width, bank, cardinality, authority, and current rejection
  site.
- Confirm whether `src/20010518-1.c` now rejects at the closed idea 516
  producer/classifier boundary rather than at RV64 object-emission
  materialization.
- Confirm that `src/pr27073.c` is not the coherent stack-slot to stack-slot
  shape accepted by idea 513 and not the multi-source shape closed by idea 516.

Completion check:

- `todo.md` records the post-516 representative facts, the exact current
  rejection sites, and whether RV64 has enough explicit authority to attempt
  the single-move materialization decision.

## Step 2: Define The Minimal Single-Move Contract

Goal: Decide whether the single register-source to stack-destination shape is
authorized for RV64 materialization or must be rejected by a more precise
owner.

Actions:

- Trace current move-bundle classification and materialization in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and neighboring helpers.
- Identify existing register-source move support, stack-destination storage
  helpers, scratch policy, width-specific store helpers, and diagnostics.
- Define the minimal accepted predicate for a single explicit register-source
  to stack-destination value move if the facts are coherent.
- Define fail-closed variants for missing source authority, missing
  destination authority, unsupported register bank, unsupported width,
  contradictory storage facts, or producer misclassification.

Completion check:

- `todo.md` names the accepted or rejected semantic boundary, the predicate or
  diagnostic owner, and the adjacent shapes that must remain fail-closed.

## Step 3: Materialize Or Reclassify The Single-Move Shape

Goal: Handle the smallest authorized single-move register-source to
stack-destination shape without overfitting to `src/pr27073.c`.

Actions:

- If Step 2 proves the shape is authorized, add the narrow RV64
  object-emission path that stores the explicit register source into the
  explicit stack destination.
- If Step 2 proves the shape is not authorized, repair the producer
  classification or diagnostic so the unsupported owner is precise.
- Preserve idea 513 stack-to-stack behavior and idea 516 multi-source
  producer/classifier behavior.
- Keep unsupported banks, unsupported widths, missing facts, contradictory
  facts, and multi-source bundles fail-closed unless a later plan explicitly
  owns them.

Completion check:

- A fresh build passes.
- Focused representative proof shows `src/pr27073.c` either advances past the
  old ambiguous gate or reports the correct owner-specific diagnostic.
- `todo.md` records the handled boundary and remaining unsupported shapes.

## Step 4: Add Focused Backend Coverage

Goal: Cover accepted and rejected register-source to stack-destination prepared
move shapes without depending on gcc_torture row names.

Actions:

- Add or extend focused backend object-emission tests for the accepted
  single-move register-source to stack-destination shape, if implemented.
- Add reject coverage for missing authority, malformed destination facts,
  unsupported banks or widths, contradictory storage facts, or the
  owner-specific reject path identified in Step 2.
- Add or preserve coverage proving idea 516's multi-source closed boundary is
  not weakened.
- Keep assertions tied to semantic facts and diagnostics, not scan accounting
  or expectation rewrites.

Completion check:

- Focused backend tests pass.
- Reject coverage proves adjacent unsupported cases still fail closed.
- `git diff --check` passes for touched files.

## Step 5: Validate And Handoff

Goal: Prove the lifecycle slice is stable enough for closure or for a clearly
bounded follow-up.

Actions:

- Run the build and focused tests required by the supervisor.
- Run the relevant backend subset for the touched RV64 object-emission or
  producer-classification scope.
- Run regression guard when the supervisor asks for close readiness.
- Update `todo.md` with validation results, residual risks, and whether the
  source idea acceptance criteria are satisfied.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The runbook can be closed, or remaining work is explicitly separated from
  the register-source stack-destination prepared move-bundle scope.
