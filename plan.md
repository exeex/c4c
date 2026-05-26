# RISC-V Pointer-Base Edge Publication Consumer Plan

Status: Active
Source Idea: ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md

## Purpose

Activate the RISC-V follow-up for prepared edge-publication consumption of
`PointerBasePlusOffset -> Register` moves. The route extends the shared
`edge_publications` consumer without local edge-fact rediscovery.

## Goal

Make RISC-V consume shared prepared edge publications for pointer-base source
homes moving into register destinations, with an explicit target-local address
materialization policy.

## Core Rule

Shared `edge_publications` remains the only semantic authority for the edge
move. RISC-V may own pointer-base address materialization, scratch policy,
instruction selection, and final assembly formatting.

## Read First

- `ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- Shared prepared lookup tests for edge publication authority.

## Current Scope

- Add `PointerBasePlusOffset -> Register` RISC-V prepared edge-publication
  consumption.
- Keep support routed through the shared prepared lookup path.
- Define and test the RISC-V-local address materialization rule.
- Preserve existing `Register -> Register`, `RematerializableImmediate ->
  Register`, and focused `StackSlot -> Register` behavior.

## Non-Goals

- Do not implement source-to-`StackSlot` destination moves.
- Do not broaden stack-source policy beyond the existing focused route.
- Do not scan predecessor or successor blocks to rediscover edge facts.
- Do not move RISC-V address materialization or scratch policy into shared
  prepare, BIR, or target-neutral helpers.
- Do not perform broad RISC-V codegen, memory-layout, or pointer-analysis
  rewrites unrelated to this source home.

## Working Model

The existing RISC-V consumer handles register, immediate, and focused stack
source homes by rendering target-local source moves after shared lookup
authority approves the publication. This plan adds the pointer-base source
materialization needed to compute the selected address into the destination
register.

Unsupported homes should remain explicit and fail closed.

## Execution Rules

- Keep each implementation packet tied to observable pointer-base source
  behavior.
- Add tests that fail if shared publication facts are missing or ignored.
- Keep target-specific address materialization details inside the RISC-V
  backend.
- Escalate to review if the route starts matching fixture labels, edge names,
  value ids, or testcase shapes.
- Run focused RISC-V prepared edge-publication proof after code changes.
- Run an appropriate backend bucket before closure or milestone handoff.

## Steps

### Step 1: Inventory RISC-V Pointer-Base Path

Goal: identify the current RISC-V consumer shape and the exact pointer-base
source data available from prepared homes.

Actions:
- Inspect the existing RISC-V edge-publication consumer.
- Identify helper boundaries for source rendering, destination register
  emission, and address materialization.
- Confirm the `PointerBasePlusOffset` fields available to the RISC-V backend.
- Record the selected implementation target and proof command in `todo.md`.

Completion Check:
- `todo.md` records the current consumer path, the pointer-base source-home
  data model, and the focused proof subset.

### Step 2: Define Pointer-Base Materialization Policy

Goal: choose the smallest semantic RISC-V address materialization rule that is
safe for `PointerBasePlusOffset -> Register` edge-publication moves.

Actions:
- Decide how base register, offset, scratch needs, and emitted instruction
  sequence should be represented for this source home.
- Keep unsupported pointer-base shapes explicit and fail closed if policy is
  not yet defined for them.
- Avoid changing shared prepared ownership or target-neutral data structures
  unless a blocker proves the existing contract is insufficient.
- Record any deferred pointer-base forms in `todo.md`.

Completion Check:
- `todo.md` records the selected pointer-base materialization policy and any
  pointer-base forms that remain unsupported.

### Step 3: Implement PointerBasePlusOffset -> Register Consumption

Goal: consume shared `edge_publications` for RISC-V pointer-base sources moving
into register destinations.

Actions:
- Add RISC-V-local pointer-base source rendering or address materialization for
  prepared edge-publication moves.
- Emit the selected instruction sequence into the destination register after
  shared lookup authority accepts the publication.
- Preserve existing register, immediate, and focused stack-source behavior.
- Keep stack-slot destinations fail closed.

Completion Check:
- Focused positive coverage proves `PointerBasePlusOffset -> Register` emits
  through the shared publication path.
- Existing register, immediate, and focused stack-source tests still pass.

### Step 4: Prove Fail-Closed Authority

Goal: prove the implementation depends on shared publication facts and does
not accept unsupported homes or local rediscovery.

Actions:
- Add or extend negative tests for missing lookup authority and absent
  publication facts.
- Keep explicit negative coverage for stack-slot destinations and unsupported
  pointer-base shapes.
- Avoid expectation downgrades for already-supported register-destination
  paths.

Completion Check:
- Focused tests fail if the shared lookup is bypassed or missing.
- Unsupported homes remain explicit fail-closed behavior.

### Step 5: Validate the RISC-V Pointer-Base Slice

Goal: establish fresh proof for the RISC-V pointer-base change and relevant
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

### Step 6: Handoff or Close

Goal: decide whether idea 26 is complete and record remaining RISC-V
edge-publication gaps.

Actions:
- Summarize supported behavior: `PointerBasePlusOffset -> Register`
  register-destination moves through shared `edge_publications`.
- Preserve caveats for source-to-stack destinations and stack-source policy
  broadening.
- Recommend closure if acceptance criteria are met, or identify the exact
  blocker if pointer-base support remains incomplete.

Completion Check:
- `todo.md` contains a concrete handoff decision and remaining-gap note.
- The source idea can be closed only if pointer-base register-destination
  support and validation are complete.
