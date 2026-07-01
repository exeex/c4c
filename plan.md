# RV64 Fixed Prepared Stack-Frame Emission Runbook

Status: Active
Source Idea: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md

## Purpose

Repair the RV64 object-emission consumer path for coherent prepared fixed
stack-frame plans, including large frame sizes and many fixed slots.

## Goal

Teach RV64 object emission to consume explicit prepared fixed-frame size,
alignment, slot offset, width, and alignment facts without reconstructing frame
layout from source shape, testcase identity, or raw slot-count guesses.

## Core Rule

Consume prepared fixed-frame authority directly. If the prepared frame plan is
missing, contradictory, dynamic, incomplete, or outside the fixed-frame scope,
keep the route fail-closed with a diagnostic that identifies the unsupported
fact family.

## Read First

- ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md
- docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
- build/agent_state/424_step2_infrastructure_classification/20030209-1/

## Current Targets

- Representative row: `src/20030209-1.c`
- Failure bucket: `unsupported_stack_frame`
- Owning layer: RV64 object emission
- Known prepared facts:
  - `frame_size=80000`
  - `frame_alignment=8`
  - `10000` fixed slots
  - no dynamic stack
  - prepared frame plan republishes the same fixed-frame shape

## Non-Goals

- Do not repair or invent producer frame-layout facts, slot numbering, offsets,
  widths, alignments, or ABI homes.
- Do not implement dynamic stack allocation, VLAs, alloca, varargs, F128, broad
  FPR ABI, or vector stack-frame support.
- Do not broaden callee-saved GPR save-slot support beyond behavior already
  made consumable by the separate save-slot route.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, scan accounting, or default test contracts.

## Working Model

- The prepared contract already owns the fixed frame size, alignment, slot
  count, and per-slot addressing facts for the target representative.
- RV64 object emission should validate that the prepared frame plan is coherent
  and use those facts for stack-pointer adjustment and fixed-slot addressing.
- Large frame adjustments and large slot offsets may require multi-instruction
  immediate materialization; that materialization must remain general and
  fact-driven.
- Prepared FPR save-slot facts observed near the representative are
  compatibility watchouts only. They are not permission to expand this runbook
  into broad FPR or F128 ABI support.

## Execution Rules

- Keep implementation changes in RV64 object emission and nearby helpers needed
  to consume existing prepared fixed-frame facts.
- Prefer reusable helpers for frame-plan validation, large immediate
  materialization, and fixed-slot address formation only when they reduce real
  duplication or make proof clearer.
- Add ordinary-C coverage that proves large fixed-frame stack adjustment and
  fixed-slot addressing through behavior or object emission, not through a
  named gcc_torture shortcut.
- For code-changing steps, use a build proof, focused representative proof,
  focused coverage proof, and an appropriate `^backend_` subset.

## Step 1: Rebuild Fixed-Frame Evidence

Goal: Reconfirm the representative failure and the prepared fixed-frame fact
contract before editing code.

Actions:

- Inspect the idea 424 handoff docs and the
  `build/agent_state/424_step2_infrastructure_classification/20030209-1/`
  artifacts.
- Run or reuse focused dumps for `src/20030209-1.c` as needed to show the
  prepared fixed-frame plan and the current RV64 object-route rejection.
- Record the frame size, alignment, fixed-slot shape, rejection point, and
  proof artifacts in `todo.md`.

Completion check:

- `todo.md` identifies the prepared fixed-frame facts, the current unsupported
  diagnostic, and the exact code path that rejects the coherent frame plan.

## Step 2: Trace RV64 Fixed-Frame Consumption

Goal: Locate the minimal RV64 object-emission path that should validate and
consume coherent fixed-frame plans.

Actions:

- Trace prepared frame-plan data from prepared output into MIR or
  object-emission lowering.
- Identify existing stack-pointer adjustment, immediate materialization, and
  frame-slot addressing helpers that should be reused or generalized.
- Identify unsupported variants that must continue to fail closed, including
  dynamic, incomplete, contradictory, FPR/F128-only, or producer-missing facts.
- Record the intended helper boundaries and blast radius in `todo.md`.

Completion check:

- `todo.md` names the producer fact path, the RV64 consumer entry point, the
  helpers to reuse or add, and the unsupported variants that remain rejected.

## Step 3: Implement Fixed-Frame Validation And Adjustment

Goal: Accept coherent prepared fixed-frame plans and emit correct stack-pointer
adjustment for large fixed frames.

Actions:

- Add RV64 object-emission validation for prepared fixed-frame plans with
  explicit frame size, frame alignment, and fixed-slot authority.
- Emit stack-pointer prologue and epilogue adjustments from the prepared frame
  size, including large sizes that require multi-instruction materialization.
- Keep dynamic, missing, contradictory, over-aligned, or otherwise unsupported
  frame plans fail-closed.
- Add or update focused object-emission tests for fixed-frame validation and
  large adjustment instruction shape where appropriate.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused representative proof advances past the old
  `requires a supported prepared stack frame` rejection for fixed-frame
  validation and stack adjustment.
- Focused object-emission tests pass if they were added or changed.
- `todo.md` records proof commands and artifacts.

## Step 4: Implement Fixed-Slot Addressing

Goal: Use prepared fixed-slot offsets, widths, and alignments for frame-slot
loads, stores, and address formation in the repaired route.

Actions:

- Route fixed-frame slot references through prepared slot facts instead of
  reconstructing offsets from slot numbers, source layout, or row shape.
- Handle large slot offsets through the same general address materialization
  strategy used for the large frame.
- Preserve fail-closed diagnostics for slots with missing offsets, unsupported
  widths, unsupported alignments, dynamic ownership, or incoherent frame-plan
  membership.
- Add or update focused coverage for fixed-slot loads and stores in a large
  frame.

Completion check:

- Focused representative proof advances through fixed-slot addressing for the
  repaired fixed-frame capability.
- Focused object-emission or backend coverage proves fixed-slot addressing for
  at least one large-frame path.
- `todo.md` records the covered slot/addressing shapes and remaining
  unsupported variants.

## Step 5: Add Ordinary-C Coverage

Goal: Prove the repaired route with nearby ordinary-C backend coverage that is
not tied to one gcc_torture row.

Actions:

- Add a focused ordinary-C case that requires a large fixed frame and accesses
  fixed stack slots through prepared facts.
- Register the object-route backend test needed to exercise the repaired
  consumer path.
- Do not rely on `src/20030209-1.c`, `80000`, `10000`, raw slot numbers, or
  one exact frame layout as the only proof.

Completion check:

- The new focused backend test passes.
- The focused representative route remains advanced past the repaired
  fixed-frame failure.
- `git diff --check` passes for touched files.

## Step 6: Validate And Handoff

Goal: Prove the slice and prepare the source idea for closure or any necessary
follow-up split.

Actions:

- Run the build, focused representative proof, focused coverage proof, and
  `ctest --test-dir build -j --output-on-failure -R "^backend_"`.
- Confirm unsupported variants still fail closed and no expectation,
  unsupported-marker, allowlist, or scan-accounting downgrade was used as
  progress.
- Record any remaining non-goal failures, such as FPR/F128 save slots, dynamic
  stack, producer frame-layout gaps, or broad ABI support, in `todo.md`.

Completion check:

- Backend validation passes.
- `todo.md` contains the final proof summary and closure recommendation.
