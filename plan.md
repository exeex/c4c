# RV64 Multi-Source Prepared Move Bundle Classification Runbook

Status: Active
Source Idea: ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md

## Purpose

Resolve prepared move bundles that contain multiple register sources targeting
one stack value before RV64 object emission is asked to guess how to
materialize them.

## Goal

Make the multi-source prepared move-bundle owner explicit: either publish a
coherent producer/classifier contract that RV64 can consume, or reject the
shape before object emission with a precise producer-owned diagnostic.

## Core Rule

Use explicit prepared move-bundle, source-home, destination-home, storage,
authority, ordering, and classification facts. Do not infer ownership or
ordering from row names, source order, ABI formulas, argument indexes, variable
names, or raw BIR text.

## Read First

- ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md
- ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/mir/riscv/
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/

## Current Targets

- Representative row: `src/20010518-1.c`
- Current rejection: `unsupported_move_bundle_target_shape`
- Current object-emission facts: `authority=none`, `move_count=2`,
  `parallel_copy=no`, two register-source `i32` moves, and both moves target
  the same stack-slot destination value.
- Required owner: producer/classifier boundary that decides whether this shape
  has coherent sequencing, parallel-copy authority, an explicit split into
  independent moves, or a producer-owned reject.

## Non-Goals

- Do not materialize the multi-source shape in RV64 object emission without
  explicit producer authority and sequencing.
- Do not drop moves, choose one source by order, or merge moves without a
  published contract.
- Do not infer ordering or ownership from row names, source order, ABI formulas,
  argument indexes, variable names, or textual BIR matching.
- Do not broaden into general parallel-copy scheduling or cycle resolution
  unless the explicit producer facts prove that minimal owner is required.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, reason strings, or scan accounting as proof of capability
  progress.

## Working Model

- Idea 514 retired a runbook that produced precise diagnostics for
  register-source stack-destination prepared move bundles.
- `src/20010518-1.c` remains a distinct multi-source producer/classification
  problem: RV64 sees multiple register-source moves targeting one stack value
  without authority or sequencing that would make materialization safe.
- The next implementation slice should first identify the producer or
  classifier that emits this ambiguous bundle, then decide whether to publish a
  coherent contract or reject earlier.

## Execution Rules

- Start by reproducing `src/20010518-1.c` and recording explicit producer and
  object-emission facts in `todo.md`.
- Keep the first implementation boundary at the producer/classifier contract;
  do not patch RV64 object emission around missing authority.
- Preserve the precise diagnostics established by idea 514 for unsupported
  register-source stack-destination shapes.
- Add focused coverage at the producer/classifier or backend boundary that
  proves semantic facts, not row names.
- For code-changing steps, run a fresh build, focused representative proof,
  focused coverage, and the relevant backend subset chosen by the supervisor.

## Step 1: Rebuild Multi-Source Bundle Evidence

Goal: Reconfirm the exact multi-source prepared move-bundle facts for
`src/20010518-1.c`.

Actions:

- Reproduce focused dumps or probes for `src/20010518-1.c`.
- Record source homes, destination home, storage plan, scalar types, widths,
  banks, move count, ordering facts, authority, `parallel_copy`, and rejection
  point.
- Identify where the bundle becomes `authority=none`, `move_count=2`, and
  `parallel_copy=no`.
- Confirm no source is stack-backed under explicit prepared facts.

Completion check:

- `todo.md` records the representative facts and the producer/classifier point
  that creates or preserves the ambiguous multi-source bundle.

## Step 2: Define The Producer Classification Contract

Goal: Decide the smallest semantic contract that can safely handle multiple
register sources targeting one stack value.

Actions:

- Trace producer and classifier code that builds prepared move-bundle authority
  and parallel-copy facts.
- Decide whether the shape should publish coherent parallel-copy authority,
  split into explicit independent moves, or reject before RV64 object emission.
- Define fail-closed variants for missing authority, contradictory destination
  ownership, unsupported banks, unsupported widths, and missing sequencing.
- Keep RV64 object emission as a consumer unless the producer contract is
  already explicit and coherent.

Completion check:

- `todo.md` names the exact producer/classifier boundary, the accepted or
  rejected predicate, and adjacent shapes that must remain unsupported.

## Step 3: Repair Classification Or Producer-Owned Rejection

Goal: Implement the minimal producer/classifier behavior for the multi-source
shape.

Actions:

- If the shape is authorized, publish the explicit authority, sequencing, or
  split contract needed by downstream RV64 code.
- If the shape is not authorized, reject it before object emission with a
  diagnostic that identifies the producer/classifier owner.
- Preserve existing single-move and stack-to-stack behavior.
- Keep unsupported adjacent shapes fail-closed.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused representative proof shows `src/20010518-1.c` no longer reaches RV64
  object emission with ambiguous `authority=none`, `move_count=2`,
  `parallel_copy=no` ownership.
- `todo.md` records the handled boundary and remaining unsupported shapes.

## Step 4: Add Focused Coverage

Goal: Prove the producer/classifier contract without depending on the gcc
torture row name.

Actions:

- Add or extend focused tests for the accepted producer contract or the refined
  producer-owned reject path.
- Add adjacent reject coverage for missing authority, missing sequencing,
  contradictory destinations, unsupported banks, or unsupported widths.
- Keep assertions tied to semantic prepared facts and diagnostics.

Completion check:

- Focused coverage passes.
- Reject coverage proves adjacent unsupported cases still fail closed.
- `git diff --check` passes for touched files.

## Step 5: Validate And Handoff

Goal: Prove the slice is stable enough for closure or the next bounded
follow-up.

Actions:

- Run the build and focused tests required by the supervisor.
- Run the relevant backend subset for the touched producer/classifier scope.
- Run regression guard when the supervisor asks for close readiness.
- Update `todo.md` with validation results, residual risks, and whether the
  source idea acceptance criteria are satisfied.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The source idea can be closed, or any remaining work is explicitly separated
  from the multi-source prepared move-bundle producer/classification scope.
