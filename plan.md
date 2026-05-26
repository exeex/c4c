# RISC-V Prepared Edge Publication Dynamic Stack Source Policy Runbook

Status: Active
Source Idea: ideas/open/41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md

## Purpose

Decide whether RISC-V prepared edge publications can safely load
`StackSlot -> Register` sources whose effective address is dynamic or otherwise
not represented as a concrete stack offset.

## Goal

Implement one safe dynamic-address stack-source form through shared prepared
edge-publication authority, or record the exact prepared-authority blocker that
keeps dynamic stack sources fail closed.

## Core Rule

Shared `edge_publications` remain the only semantic authority for edge moves.
Do not rediscover dynamic stack-source facts from block shape, predecessor or
successor scans, fixture labels, value ids, stack slot ids, offsets, or test
names.

## Read First

- `ideas/open/41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- shared prepared edge-publication and value-home lookup code
- focused RISC-V prepared edge-publication tests
- `todo.md`

## Current Scope

- Inventory dynamic stack-source homes visible to RISC-V prepared
  edge-publication lowering.
- Decide what prepared authority must identify the dynamic address, base
  anchor, offset expression, load width, and scratch-register needs.
- Define target-local RISC-V address materialization and scratch-register
  policy for one safe dynamic stack-source form if enough authority exists.
- Preserve existing concrete 4-byte, 8-byte, and large-offset stack-source
  behavior.

## Non-Goals

- Do not implement typed scalar extension policy for concrete stack offsets.
- Do not implement aggregate-width stack-source support.
- Do not broaden `StackSlot` destinations.
- Do not broaden `PointerBasePlusOffset -> Register`.
- Do not rewrite broad dynamic-stack frame layout outside the edge-publication
  source-addressing policy.
- Do not recreate edge-copy facts by scanning predecessor or successor blocks.
- Do not weaken existing concrete-offset or large-offset behavior.

## Working Model

- Existing RISC-V stack-source publication supports concrete stack offsets:
  4-byte loads, 8-byte loads, and large-offset address materialization.
- Dynamic stack sources require authority beyond a concrete offset: address
  identity, base anchor, offset expression, load width, and scratch policy.
- If shared prepared facts do not identify those pieces, the correct outcome is
  explicit fail-closed behavior with a recorded blocker.

## Execution Rules

- Start with an inventory packet before implementation.
- Pick at most one first dynamic-address stack-source form after the inventory
  proves the prepared-authority boundary.
- Keep tests focused on prepared publication facts and neighboring
  fail-closed cases.
- Reject testcase-shaped shortcuts, expectation downgrades, and helper renames
  that preserve the old unsupported behavior while claiming support.
- Run focused RISC-V prepared edge-publication proof after code packets and a
  backend bucket before lifecycle closure.

## Ordered Steps

### Step 1: Inventory Dynamic Stack-Source Authority

Goal: map the current RISC-V prepared edge-publication dynamic stack-source
surface and determine whether any dynamic-address `StackSlot -> Register` form
has enough prepared authority to implement first.

Primary targets: RISC-V edge-publication lowering, shared prepared
edge-publication/value-home structures and lookups, and focused RISC-V tests.

Actions:
- Inventory current concrete stack-source behavior for 4-byte, 8-byte, and
  large-offset forms so the dynamic route preserves it.
- Search for prepared homes or publication records that can represent dynamic
  stack-source addresses.
- Map available prepared facts for base anchor, offset expression, load width,
  source identity, destination register, and scratch-register needs.
- Identify focused tests covering existing concrete behavior and nearby
  unsupported dynamic-address behavior.
- Recommend one selected first dynamic form or record the exact prepared
  authority blocker.

Completion check:
- `todo.md` records current behavior, available and missing prepared facts,
  focused tests, and the recommended first implementation packet or blocker.

### Step 2: Implement Or Preserve Fail-Closed Dynamic Form

Goal: implement the selected dynamic-address stack-source policy only where
prepared authority is complete, or make the fail-closed reason explicit where
authority is absent.

Primary target: RISC-V prepared edge-publication lowering.

Actions:
- Consume shared `edge_publications` and prepared value-home facts as the only
  semantic source of edge-copy authority.
- Select RISC-V address materialization, base anchor, load width, and scratch
  behavior for the chosen dynamic form.
- Preserve existing concrete 4-byte, 8-byte, and large-offset behavior.
- Keep unsupported dynamic bases, missing address facts, widths, and scratch
  needs explicit and fail closed.
- Add focused positive and negative tests for the selected form or fail-closed
  policy.

Completion check:
- Focused RISC-V prepared edge-publication and shared prepared lookup proof
  passes, with no weakened concrete-offset or large-offset behavior.

### Step 3: Harden Neighboring Unsupported Cases

Goal: ensure nearby dynamic-address forms remain explicit and fail closed
unless the selected implementation has real prepared authority for them too.

Primary target: focused tests and RISC-V unsupported-path diagnostics or
records.

Actions:
- Add or update narrow negative coverage for unsupported dynamic bases, missing
  address facts, widths, or scratch-register needs adjacent to the selected
  form.
- Verify tests fail if shared publication facts are missing or ignored.
- Do not broaden into typed scalar, aggregate, pointer-base, or source-to-stack
  behavior.

Completion check:
- Focused negative and positive tests prove the selected dynamic policy
  boundary.

### Step 4: Validate Dynamic Stack-Source Policy

Goal: prove the selected dynamic stack-source policy is complete or that the
documented fail-closed blocker is source-idea-complete.

Primary target: focused RISC-V tests, shared prepared lookup tests, and backend
bucket.

Actions:
- Run a fresh build.
- Run focused RISC-V prepared edge-publication and shared prepared lookup
  tests.
- Run an appropriate backend bucket before lifecycle closure.
- Search for evidence of local edge-copy fact rediscovery or testcase-shaped
  matching introduced by this route.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  source idea can close or clearly records the remaining prepared-authority
  blocker.
