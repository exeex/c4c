# RISC-V Prepared Edge Publication Stack Source Policy Runbook

Status: Active
Source Idea: ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md

## Purpose

Broaden RISC-V prepared edge-publication support for safe remaining
`StackSlot -> Register` stack-source forms without rediscovering edge-copy facts
inside the RISC-V backend.

## Goal

Implement one well-defined remaining stack-source policy through shared
`edge_publications` authority, or record a concrete fail-closed reason for the
candidate form.

## Core Rule

Shared prepared `edge_publications` must remain the semantic authority for edge
moves; RISC-V codegen may only add target-local load, extension, scratch, and
address materialization policy.

## Read First

- `ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md`
- Existing RISC-V prepared edge-publication lowering and tests around
  `StackSlot -> Register` moves
- Closed behavior from ideas 25 and 28 only as context for preserving existing
  concrete-offset 4-byte `lw` and 8-byte `ld` support

## Scope

- Remaining `StackSlot -> Register` source forms for RISC-V prepared edge
  publications.
- Target-local RISC-V load, extension, scratch-register, and address
  materialization policy.
- Focused positive and negative tests for each accepted form.

## Non-Goals

- Do not broaden `PointerBasePlusOffset -> Register` policy.
- Do not broaden source-to-`StackSlot` destination policy.
- Do not scan predecessor or successor block shape to recreate edge-copy facts.
- Do not move RISC-V stack-source policy into shared prepare, BIR, or
  target-neutral helpers.
- Do not perform broad frame-layout or memory-layout rewrites.

## Working Model

- Shared prepare publishes whether an edge move exists.
- RISC-V lowering consumes the prepared edge publication and decides whether
  the source home, destination home, width, offset, and address form are
  target-locally emit-able.
- Unsupported stack-source forms must fail closed with explicit diagnostics or
  unsupported-path behavior already used by the backend.

## Execution Rules

- Preserve existing concrete-offset 4-byte `lw` and 8-byte `ld` behavior.
- Add one policy at a time; keep unsupported neighboring forms explicit.
- Prefer small implementation packets with focused tests before broader backend
  validation.
- Treat fixture-name matching, value-id matching, and expectation-only changes
  as route failures.
- Keep routine execution notes in `todo.md`; edit this runbook only when the
  route itself needs a lifecycle-level correction.

## Step 1: Inventory Remaining Stack-Source Candidates

Goal: identify the current supported and fail-closed `StackSlot -> Register`
forms before selecting the next policy.

Primary targets:

- RISC-V prepared edge-publication lowering code
- Existing RISC-V prepared edge-publication tests
- Shared prepared edge-publication lookup tests relevant to stack sources

Actions:

- Inspect the current `StackSlot -> Register` emission path and record where
  4-byte `lw` and 8-byte `ld` concrete-offset support is enforced.
- List remaining candidate forms from the source idea: sub-word integer loads,
  unsigned 32-bit loads, floating 32-bit loads, large-offset stack loads,
  dynamic-address stack sources, and aggregate-width sources.
- For each candidate, record the missing target-local policy: load opcode,
  extension behavior, scratch-register needs, offset materialization, and
  address-form constraints.
- Identify focused positive and negative test surfaces for the safest next
  candidate.

Completion check:

- `todo.md` records the current supported forms, candidate matrix, and the
  selected next candidate or a concrete reason selection is blocked.

## Step 2: Define And Implement The Selected Target-Local Policy

Goal: add the smallest safe RISC-V stack-source policy selected in Step 1.

Primary targets:

- RISC-V edge-publication codegen helpers for `StackSlot -> Register`
- Any target-local load, extension, scratch, or address helper needed for the
  selected form

Actions:

- Implement only the selected candidate's target-local lowering policy.
- Ensure emission is reached only through shared `edge_publications` lookup
  authority.
- Keep unsupported widths, offsets, address forms, and register classes
  explicit and fail-closed.
- Preserve existing 4-byte `lw` and 8-byte `ld` paths.

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

- Add a positive test for the selected stack-source form.
- Add a negative test showing RISC-V cannot proceed when the shared publication
  fact is absent or ignored.
- Add fail-closed coverage for the nearest unsupported neighboring form when
  practical.
- Avoid weakening existing tests for 4-byte `lw` and 8-byte `ld`.

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
  scans, expectation downgrades, and broad unrelated rewrites.

Completion check:

- Fresh proof exists in the canonical executor log, no existing supported path
  regressed, and the diff satisfies the source idea's reviewer reject signals.

## Step 5: Close Or Record Follow-Up Policy

Goal: decide whether the source idea is complete or needs another narrow
policy follow-up.

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
