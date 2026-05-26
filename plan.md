# RISC-V Stack Destination Edge Publication Plan

Status: Active
Source Idea: ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md

## Purpose

Implement the next RISC-V prepared edge-publication surface: source-to-`StackSlot`
destination moves, keeping the shared `edge_publications` table as the only
semantic authority.

## Goal

Prove at least one RISC-V source-to-`StackSlot` destination edge-publication move
through shared lookup authority, with unsupported source and destination homes
remaining explicit and fail closed.

## Core Rule

Do not rediscover edge-copy facts from RISC-V predecessor or successor block
shape. Every supported store must be driven by shared prepared
`edge_publications`.

## Read First

- `ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Current Scope

- Target destination home family: source-to-`StackSlot` edge-publication moves.
- Initial preferred source family: `Register -> StackSlot`, unless Step 1 finds
  a concrete blocker.
- Keep existing `Register -> Register`,
  `RematerializableImmediate -> Register`, `StackSlot -> Register`, and
  `PointerBasePlusOffset -> Register` consumers supported.
- Keep RISC-V stack destination store and addressing policy target-local.

## Non-Goals

- Do not broaden pointer-base source materialization in this plan.
- Do not broaden stack-source load policy in this plan.
- Do not move RISC-V store/addressing policy into shared prepare, BIR, or
  target-neutral helpers.
- Do not downgrade existing register-destination edge-publication tests.
- Do not add RISC-V-local edge-publication discovery.

## Working Model

The RISC-V backend already consumes shared prepared edge publications for
register destinations. This plan extends the same lookup path to selected
`StackSlot` destinations by rendering a target-local store for the selected
source home. For the first implementation packet, prefer the narrow semantic
store rule `Register -> StackSlot` with concrete stack-slot offset and 4-byte
size, emitted as `sw <src>, <offset>(sp)`.

## Execution Rules

- Record routine progress and packet evidence in `todo.md`.
- Keep changes scoped to the RISC-V prepared edge-publication consumer and its
  focused tests.
- Add positive coverage for the selected supported stack-destination form.
- Add negative coverage proving missing shared publication authority,
  unsupported source homes, malformed stack destinations, and non-move
  publications fail closed.
- Use the same focused validation bucket as prior RISC-V edge-publication
  packets, then request broader backend validation before closure.

## Steps

### Step 1: Inventory RISC-V Stack Destination Path

Goal: Confirm the current RISC-V edge-publication consumer path and the exact
stack destination facts available to codegen.

Actions:

- Inspect the existing RISC-V source rendering and edge-publication emission
  helpers.
- Identify where destination homes are checked and where a `StackSlot`
  destination can be routed without changing shared prepare.
- Confirm the fields available for stack-slot destination offset and size.
- Record the selected first source-home family in `todo.md`.

Completion check:

- `todo.md` names the target helper files/functions, selected first
  source-home family, focused proof command, and unsupported forms that remain
  fail closed.

### Step 2: Implement Register-to-StackSlot Consumption

Goal: Add focused `Register -> StackSlot` edge-publication stores through shared
lookup authority.

Actions:

- Extend the RISC-V prepared edge-publication consumer to recognize selected
  stack-slot destinations.
- Render `Register -> StackSlot` stores as `sw <src>, <offset>(sp)` only when
  the destination stack slot has concrete supported store facts.
- Preserve all existing register-destination consumers.
- Fail closed for unsupported source homes, malformed stack destinations,
  source-to-stack forms not selected by this step, and non-move publications.

Completion check:

- Focused positive and negative tests cover the selected stack-destination
  behavior.
- The delegated proof command passes and writes `test_after.log`.

### Step 3: Review Route Quality

Goal: Check that the implementation remains aligned with the source idea and
does not overfit the focused test shape.

Actions:

- Request reviewer scrutiny after the first implementation commit.
- Treat testcase-shaped matching, local edge rediscovery, downgraded
  register-destination expectations, or target-neutral store-policy movement as
  blocking failures.

Completion check:

- A review artifact under `review/` reports no blocking findings, or the route
  is corrected before continuing.

### Step 4: Validate the Stack Destination Slice

Goal: Prove the stack-destination extension does not regress the relevant
backend surface.

Actions:

- Run the focused RISC-V prepared edge-publication proof selected in Step 1.
- Run matching regression guard when before/after logs have the same scope.
- Run an appropriate backend bucket before closure.
- Preserve canonical root logs as `test_before.log` and `test_after.log` only.

Completion check:

- `todo.md` records focused proof, matching guard status when applicable,
  broader backend evidence, and any accepted baseline evidence.

### Step 5: Handoff or Close

Goal: Decide whether idea 27 is complete or whether stack-destination policy
needs another focused follow-up idea.

Actions:

- Record exactly which source-to-`StackSlot` forms are now supported.
- Preserve explicit caveats for unsupported source homes or stack-destination
  forms.
- If remaining work is adjacent but outside the completed slice, create or
  recommend separate durable follow-up ideas instead of expanding this plan.

Completion check:

- The source idea can be closed with concrete support and caveats, or the
  active route is intentionally rewritten/deactivated with durable follow-up
  scope.
