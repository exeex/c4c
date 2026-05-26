# RISC-V Prepared Edge Publication Aggregate Stack Source Policy Runbook

Status: Active
Source Idea: ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md

## Purpose

Decide whether RISC-V prepared edge publications should support
aggregate-width `StackSlot -> Register` stack-source moves.

## Goal

Implement one safe aggregate-width stack-source form through shared prepared
edge-publication authority, or record the exact policy and prepared-authority
blocker that keeps aggregate stack sources fail closed.

## Core Rule

Shared `edge_publications` remain the only semantic authority for edge moves.
Do not treat aggregate stack sources as scalar loads based on byte size,
fixture names, value ids, stack slot ids, offsets, or test names.

## Read First

- `ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- shared prepared edge-publication and value-home lookup code
- focused RISC-V prepared edge-publication tests
- `todo.md`

## Current Scope

- Audit aggregate-width `StackSlot -> Register` prepared publication facts and
  destination expectations visible to RISC-V lowering.
- Decide whether any narrow aggregate form has enough authority to emit safely.
- Define target-local load/copy sequencing, scratch-register, alignment, and
  fail-closed policy for one selected aggregate form if authority exists.
- Preserve existing scalar 4-byte, 8-byte, and large-offset stack-source
  behavior.

## Non-Goals

- Do not implement typed scalar stack-source load or extension policy.
- Do not implement dynamic-address stack-source support.
- Do not broaden `StackSlot` destinations.
- Do not broaden `PointerBasePlusOffset -> Register`.
- Do not rewrite broad ABI, frame-layout, or aggregate layout behavior outside
  the selected prepared edge-publication source form.
- Do not recreate edge-copy facts by scanning predecessor or successor blocks.
- Do not weaken existing scalar or large-offset stack-source behavior.

## Working Model

- Existing RISC-V stack-source publication supports concrete scalar stack
  offsets with size 4 or 8, including large concrete offsets.
- Aggregate-width stack sources need authority beyond a scalar size check:
  copy width, lane or destination-register semantics, scratch policy,
  alignment, partial-copy rules, and ABI/layout constraints.
- If shared prepared facts do not identify those pieces, the correct outcome is
  explicit fail-closed behavior with a recorded blocker.

## Execution Rules

- Start with an inventory packet before implementation.
- Pick at most one first aggregate-width stack-source form after the inventory
  proves the prepared-authority boundary.
- Keep tests focused on prepared publication facts and neighboring
  fail-closed cases.
- Reject testcase-shaped shortcuts, expectation downgrades, helper renames, or
  classification-only changes that preserve the old unsupported behavior while
  claiming support.
- Run focused RISC-V prepared edge-publication proof after code packets and a
  backend bucket before lifecycle closure.

## Ordered Steps

### Step 1: Inventory Aggregate Stack-Source Authority

Goal: map the current RISC-V prepared edge-publication aggregate stack-source
surface and determine whether any aggregate-width `StackSlot -> Register` form
has enough prepared authority to implement first.

Primary targets: RISC-V edge-publication lowering, shared prepared
edge-publication/value-home structures and lookups, focused RISC-V tests, and
any prepared aggregate/layout records needed to interpret aggregate width or
lanes.

Actions:
- Inventory current scalar concrete stack-source behavior for 4-byte, 8-byte,
  and large-offset forms so the aggregate route preserves it.
- Search for prepared homes or publication records that can represent
  aggregate-width stack-source copies.
- Map available prepared facts for source stack slot, copy width, destination
  register or lane expectations, alignment, partial-copy rules, ABI/layout
  constraints, and scratch-register needs.
- Identify focused tests covering existing scalar behavior and nearby
  unsupported aggregate-width behavior.
- Recommend one selected first aggregate form or record the exact policy and
  prepared-authority blocker.

Completion check:
- `todo.md` records current behavior, available and missing prepared facts,
  focused tests, and the recommended first implementation packet or blocker.

### Step 2: Implement Or Preserve Fail-Closed Aggregate Form

Goal: implement the selected aggregate-width stack-source policy only where
prepared authority is complete, or make the fail-closed reason explicit where
authority is absent.

Primary target: RISC-V prepared edge-publication lowering and focused tests.

Actions:
- Consume shared `edge_publications` and prepared value-home facts as the only
  semantic source of edge-copy authority.
- Select target-local load/copy sequencing, scratch-register, alignment, and
  partial-copy behavior for the chosen aggregate form if complete authority
  exists.
- Preserve existing scalar 4-byte, 8-byte, and large-offset behavior.
- Keep unsupported aggregate widths, destination register expectations,
  partial copies, alignments, and scratch needs explicit and fail closed.
- Add focused positive and negative tests for the selected form or fail-closed
  policy.

Completion check:
- Focused RISC-V prepared edge-publication and shared prepared lookup proof
  passes, with no weakened scalar or large-offset behavior.

### Step 3: Harden Neighboring Unsupported Cases

Goal: ensure nearby aggregate-width forms remain explicit and fail closed
unless the selected implementation has real prepared authority for them too.

Primary target: focused tests and RISC-V unsupported-path diagnostics or
records.

Actions:
- Add or update narrow negative coverage for unsupported aggregate widths,
  destination register expectations, partial-copy rules, alignments, or scratch
  needs adjacent to the selected form.
- Verify tests fail if shared publication facts are missing or ignored.
- Do not broaden into typed scalar, dynamic-address, pointer-base, or
  source-to-stack behavior.

Completion check:
- Focused negative and positive tests prove the selected aggregate policy
  boundary.

### Step 4: Validate Aggregate Stack-Source Policy

Goal: prove the selected aggregate stack-source policy is complete or that the
documented fail-closed blocker is source-idea-complete.

Primary target: focused RISC-V tests, shared prepared lookup tests, and backend
bucket.

Actions:
- Run a fresh build.
- Run focused RISC-V prepared edge-publication and shared prepared lookup
  tests.
- Run an appropriate backend bucket before lifecycle closure.
- Search for evidence of local edge-copy fact rediscovery, scalar-load
  overfitting, or testcase-shaped matching introduced by this route.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  source idea can close or clearly records the remaining policy/prepared
  authority blocker.
