# RISC-V Stack Source Edge Publication Consumer Plan

Status: Active
Source Idea: ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md

## Purpose

Activate the RISC-V follow-up for prepared edge-publication consumption of
`StackSlot -> Register` moves. The route extends the existing shared
`edge_publications` consumer without local edge-fact rediscovery.

## Goal

Make RISC-V consume shared prepared edge publications for stack-slot sources
moving into register destinations, while preserving existing register and
immediate source support.

## Core Rule

Shared `edge_publications` remains the only semantic authority for the edge
move. RISC-V may own source rendering, stack-slot addressing, load instruction
choice, scratch policy, and final assembly formatting.

## Read First

- `ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- Shared prepared lookup tests for edge publication authority.

## Current Scope

- Add `StackSlot -> Register` RISC-V prepared edge-publication consumption.
- Keep support routed through the shared prepared lookup path.
- Add focused positive and negative coverage.
- Preserve `Register -> Register` and `RematerializableImmediate -> Register`
  behavior.

## Non-Goals

- Do not implement pointer-base source homes.
- Do not implement source-to-`StackSlot` destination moves.
- Do not scan predecessor or successor blocks to rediscover edge facts.
- Do not move RISC-V stack-slot load/address policy into shared prepare, BIR,
  or target-neutral helpers.
- Do not perform broad RISC-V codegen rewrites unrelated to this source home.

## Working Model

The existing RISC-V consumer already handles simple source homes by rendering
target-local source operands after shared lookup authority approves the
publication. This plan adds the stack-slot source rendering needed to load
from the selected frame home into the destination register.

Unsupported homes should remain explicit and fail closed.

## Execution Rules

- Keep each implementation packet tied to an observable source-home behavior.
- Add tests that fail if shared publication facts are missing or ignored.
- Keep target-specific stack-slot load/address details inside the RISC-V
  backend.
- Escalate to review if the route starts matching fixture labels, value ids, or
  testcase shapes.
- Run focused RISC-V prepared edge-publication proof after code changes.
- Run an appropriate backend bucket before closure or milestone handoff.

## Steps

### Step 1: Inventory RISC-V Stack Source Path

Goal: identify the exact current RISC-V consumer shape and where stack-slot
source rendering belongs.

Actions:
- Inspect the existing RISC-V edge-publication consumer.
- Identify helper boundaries for source rendering and destination register
  emission.
- Confirm the focused tests needed for a stack-slot source positive case and
  missing-authority negative cases.
- Record the selected implementation target and proof command in `todo.md`.

Completion Check:
- `todo.md` records the current consumer path, selected source-home family, and
  the focused proof subset.

### Step 2: Implement StackSlot -> Register Consumption

Goal: consume shared `edge_publications` for RISC-V stack-slot sources moving
into register destinations.

Actions:
- Add RISC-V-local stack-slot source rendering or load emission for prepared
  edge-publication moves.
- Emit the selected load/address sequence into the destination register after
  shared lookup authority accepts the publication.
- Preserve existing register and immediate source behavior.
- Keep pointer-base sources and stack-slot destinations fail-closed.

Completion Check:
- Focused positive coverage proves `StackSlot -> Register` emits through the
  shared publication path.
- Existing register and immediate source tests still pass.

### Step 3: Prove Fail-Closed Authority

Goal: prove the implementation depends on shared publication facts and does
not accept unsupported homes.

Actions:
- Add or extend negative tests for missing lookup authority and absent
  publication facts.
- Keep explicit negative coverage for pointer-base sources and stack-slot
  destinations.
- Avoid expectation downgrades for already-supported register-destination
  paths.

Completion Check:
- Focused tests fail if the shared lookup is bypassed or missing.
- Unsupported homes remain explicit fail-closed behavior.

### Step 4: Validate the RISC-V Stack Source Slice

Goal: establish fresh proof for the RISC-V stack-source change and relevant
shared prepared lookup behavior.

Actions:
- Run a build before focused proof.
- Run focused RISC-V prepared edge-publication tests, RISC-V route tests,
  shared prepared lookup tests, and preallocation publication tests.
- If the blast radius or accumulated RISC-V changes justify it, run an
  appropriate backend bucket before handoff.
- Record proof commands and results in `todo.md`.

Completion Check:
- `test_after.log` or accepted canonical evidence records green focused proof.
- Broader backend validation is recorded before closure when required.

### Step 5: Handoff or Close

Goal: decide whether idea 25 is complete and record any remaining RISC-V
edge-publication gaps.

Actions:
- Summarize supported behavior: `StackSlot -> Register` register-destination
  moves through shared `edge_publications`.
- Preserve caveats for pointer-base sources and source-to-stack destinations.
- Recommend closure if acceptance criteria are met, or identify the exact
  blocker if stack-source support remains incomplete.

Completion Check:
- `todo.md` contains a concrete handoff decision and remaining-gap note.
- The source idea can be closed only if stack-source support and validation are
  complete.
