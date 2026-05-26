# RISC-V Prepared Edge Publication Remaining Stack Destination Sources Runbook

Status: Active
Source Idea: ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md

## Purpose

Broaden RISC-V prepared edge-publication support for the remaining
source-to-`StackSlot` destination forms without rediscovering edge-copy facts
inside the RISC-V backend.

## Goal

Implement one well-defined remaining source-to-`StackSlot` policy through
shared `edge_publications` authority, or record a concrete target-local policy
reason the candidate must remain fail-closed.

## Core Rule

Shared prepared `edge_publications` must remain the semantic authority for edge
moves; RISC-V codegen may only add target-local load, store, pointer
materialization, scratch-register, and address policy.

## Read First

- `ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md`
- Existing RISC-V prepared edge-publication lowering and tests around
  source-to-`StackSlot` moves
- Closed idea 32 behavior only as context for preserving existing
  `Register -> StackSlot` and `RematerializableImmediate -> StackSlot` I32
  support

## Scope

- Remaining source-to-`StackSlot` prepared edge-publication forms:
  `StackSlot -> StackSlot` and `PointerBasePlusOffset -> StackSlot`.
- Target-local RISC-V load, store, address materialization, pointer
  materialization, scratch-register, aliasing, and large-offset policy needed
  for the selected form.
- Focused positive and negative tests for each accepted form.

## Non-Goals

- Do not rework immediate-to-stack support from idea 32 except to preserve it.
- Do not broaden stack-source or pointer-base register-destination policy.
- Do not scan predecessor or successor block shape to recreate edge-copy facts.
- Do not move RISC-V source-load, stack-destination, pointer materialization,
  scratch, or address policy into shared prepare, BIR, or target-neutral
  helpers.
- Do not perform broad frame-layout, allocator, dynamic-stack, or memory-layout
  rewrites.

## Working Model

- Shared prepare publishes whether an edge move exists.
- RISC-V lowering consumes the prepared edge publication and decides whether
  the source home, destination home, width, offset, pointer form, aliasing
  shape, and address form are target-locally emit-able.
- Unsupported source-to-stack forms must fail closed with explicit diagnostics
  or unsupported-path behavior already used by the backend.

## Execution Rules

- Preserve existing `Register -> StackSlot` and
  `RematerializableImmediate -> StackSlot` I32 behavior.
- Select and implement one candidate policy at a time.
- Keep unsupported neighboring widths, offsets, homes, and address forms
  explicit and fail-closed.
- Prefer small implementation packets with focused tests before broader
  backend validation.
- Treat fixture-name matching, value-id matching, stack-slot-id matching,
  offset matching, and expectation-only changes as route failures.
- Keep routine execution notes in `todo.md`; edit this runbook only when the
  route itself needs a lifecycle-level correction.

## Step 1: Inventory Remaining Stack-Destination Candidates

Goal: identify the current supported and fail-closed source-to-`StackSlot`
forms before selecting the next policy.

Primary targets:

- RISC-V prepared edge-publication lowering code
- Existing RISC-V prepared edge-publication tests
- Shared prepared edge-publication lookup tests relevant to stack destinations

Actions:

- Inspect the current source-to-`StackSlot` emission path and record where
  `Register -> StackSlot` and `RematerializableImmediate -> StackSlot` I32
  support is enforced.
- List the remaining candidate forms from the source idea:
  `StackSlot -> StackSlot` and `PointerBasePlusOffset -> StackSlot`.
- For each candidate, record the missing target-local policy: source load or
  pointer materialization, destination store, scratch-register needs, offset
  materialization, aliasing constraints, width constraints, and address-form
  constraints.
- Identify focused positive and negative test surfaces for the safest next
  candidate.

Completion check:

- `todo.md` records the current supported forms, candidate matrix, and the
  selected next candidate or a concrete reason selection is blocked.

## Step 2: Define And Implement The Selected Target-Local Policy

Goal: add the smallest safe RISC-V source-to-`StackSlot` policy selected in
Step 1.

Primary targets:

- RISC-V prepared edge-publication codegen helpers for source-to-`StackSlot`
  moves
- Any target-local load, pointer materialization, store, scratch, aliasing, or
  address helper needed for the selected form

Actions:

- Implement only the selected candidate's target-local lowering policy.
- Ensure emission is reached only through shared `edge_publications` lookup
  authority.
- Keep unsupported source homes, destination homes, widths, offsets, pointer
  forms, aliasing cases, and address forms explicit and fail-closed.
- Preserve existing `Register -> StackSlot` and
  `RematerializableImmediate -> StackSlot` I32 paths.

Completion check:

- The selected form emits through prepared publication facts, existing supported
  forms still work, and unsupported neighboring forms still fail closed.

## Step 3: Add Focused Positive And Negative Coverage

Goal: prove the new policy depends on shared publication facts and rejects
unsupported neighboring forms.

Primary targets:

- Focused RISC-V prepared edge-publication tests
- Relevant shared prepared lookup tests

Actions:

- Add a positive test for the selected source-to-`StackSlot` form.
- Add a negative test showing RISC-V cannot proceed when the shared
  publication fact is absent or ignored.
- Add fail-closed coverage for the nearest unsupported neighboring form when
  practical.
- Avoid weakening existing tests for `Register -> StackSlot` and
  `RematerializableImmediate -> StackSlot` I32.

Completion check:

- Focused tests fail before the implementation when appropriate and pass after
  the policy is implemented.

## Step 4: Validate RISC-V Backend Behavior

Goal: establish that the policy is local, regression-free, and not overfit to
one fixture.

Actions:

- Run the focused RISC-V prepared edge-publication test subset.
- Run relevant shared prepared lookup tests.
- Run an appropriate backend bucket chosen by the supervisor for the touched
  code.
- Inspect the diff for fixture-name matching, predecessor/successor block
  scans, expectation downgrades, target-neutral policy movement, and broad
  unrelated rewrites.

Completion check:

- Fresh proof exists in the canonical executor log, no existing supported path
  regressed, and the diff satisfies the source idea's reviewer reject signals.

## Step 5: Close Or Record Follow-Up Policy

Goal: decide whether the source idea is complete or needs another narrow policy
follow-up.

Actions:

- If the accepted policy satisfies the source idea's completion bar, prepare
  close-readiness notes in `todo.md`.
- If meaningful remaining forms need separate durable work, propose focused
  follow-up ideas under `ideas/open/` through the plan-owner lifecycle path.
- Do not close merely because this runbook is exhausted; close only when the
  source idea's acceptance criteria are met or intentionally concluded.

Completion check:

- `todo.md` states whether the active source idea is ready for closure,
  deactivation, or another plan-owner split.
