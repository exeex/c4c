# RISC-V Prepared Edge Publication Typed Stack Source Policy Runbook

Status: Active
Source Idea: ideas/open/40_riscv_prepared_edge_publication_typed_stack_source_policy.md

## Purpose

Define and implement the next safe typed scalar `StackSlot -> Register`
prepared edge-publication policy for RISC-V, only where shared prepared
publication facts provide enough authority for type, signedness, width, and
register bank.

## Goal

Support one typed scalar concrete stack-source form for RISC-V prepared edge
publication, or record the exact prepared-authority gap that keeps the selected
form fail-closed.

## Core Rule

Shared `edge_publications` remain the only semantic authority for edge moves.
Do not infer signedness, floatingness, register bank, or source identity from
fixture names, value ids, stack slot ids, offsets, block shape, or test names.

## Read First

- `ideas/open/40_riscv_prepared_edge_publication_typed_stack_source_policy.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- shared prepared edge-publication and value-home lookup code
- focused RISC-V prepared edge-publication tests
- `todo.md`

## Current Scope

- Audit typed scalar `StackSlot -> Register` source facts visible to RISC-V
  prepared edge-publication lowering.
- Choose exactly one first typed scalar form: signed sub-word, unsigned
  sub-word, unsigned I32, or F32.
- Add only the RISC-V target-local load, extension, register-bank, and
  fail-closed policy needed for that selected form.
- Preserve existing 4-byte `lw`, 8-byte `ld`, and large-offset concrete
  stack-source behavior.

## Non-Goals

- Do not implement dynamic-address stack-source support.
- Do not implement aggregate-width stack-source support.
- Do not broaden `StackSlot` destinations or `PointerBasePlusOffset -> Register`.
- Do not move RISC-V typed load or extension policy into shared prepare, BIR,
  or target-neutral helpers without a separate source idea.
- Do not rewrite broad frame layout, allocator, memory layout, or unrelated
  target code.
- Do not weaken tests, diagnostics, or unsupported fail-closed behavior.

## Working Model

- Existing concrete stack-source support covers size-based 4-byte and 8-byte
  integer-like loads and large-offset address materialization.
- Typed scalar support needs authority beyond size: signedness, scalar type,
  destination register bank, and extension/load opcode choice.
- If the selected typed form lacks target-neutral authority, keep it fail-closed
  and record the exact missing fact instead of guessing locally.

## Execution Rules

- Start with an inventory packet before implementation.
- Pick one typed scalar form only after the inventory proves the authority
  boundary.
- Keep new tests focused on prepared publication facts and neighboring
  fail-closed cases.
- Reject testcase-shaped shortcuts and expectation downgrades.
- Run focused RISC-V prepared edge-publication proof after code packets and a
  backend bucket before lifecycle closure.

## Ordered Steps

### Step 1: Inventory Typed Stack-Source Authority

Goal: map the current RISC-V prepared edge-publication `StackSlot -> Register`
surface and determine which typed scalar form, if any, has enough prepared
authority to implement first.

Primary targets: RISC-V edge-publication lowering, shared prepared
edge-publication/value-home structures and lookups, and focused RISC-V tests.

Actions:
- Inventory current RISC-V concrete stack-source behavior for 4-byte, 8-byte,
  and large-offset forms.
- Map available prepared facts for source stack slot, byte width, scalar type,
  signedness, destination register bank, and destination register view.
- Identify focused tests that cover existing behavior and nearby unsupported
  typed forms.
- Recommend one selected first typed form or record the exact missing prepared
  authority that blocks all candidates.

Completion check:
- `todo.md` records helper/call/test inventory, available and missing prepared
  facts, chosen first typed form or blocker, and the first implementation
  packet with focused proof.

### Step 2: Implement Or Preserve Fail-Closed Selected Typed Form

Goal: implement the selected typed scalar `StackSlot -> Register` policy only
where prepared authority is complete, or make the fail-closed reason explicit
where authority is absent.

Primary target: RISC-V prepared edge-publication lowering.

Actions:
- Use shared `edge_publications` and prepared value-home facts as the only
  semantic source of edge-copy authority.
- Select the RISC-V load/extension/register-bank behavior for the chosen typed
  form.
- Preserve existing 4-byte `lw`, 8-byte `ld`, and large-offset behavior.
- Keep unsupported signedness, type, width, and register-bank combinations
  explicit and fail closed.
- Add focused positive and negative tests for the selected form or fail-closed
  policy.

Completion check:
- Focused RISC-V prepared edge-publication and shared prepared lookup proof
  passes, with no weakened neighboring unsupported behavior.

### Step 3: Harden Neighboring Unsupported Cases

Goal: ensure nearby typed scalar forms remain explicit and fail closed unless
the selected implementation has real prepared authority for them too.

Primary target: focused tests and RISC-V unsupported-path diagnostics or
records.

Actions:
- Add or update narrow negative coverage for unsupported signedness, width,
  type, or register-bank combinations adjacent to the selected form.
- Verify tests fail if shared publication facts are missing or ignored.
- Do not broaden into dynamic-address, aggregate, pointer-base, or
  source-to-stack behavior.

Completion check:
- Focused negative and positive tests prove the selected typed policy boundary.

### Step 4: Validate Typed Stack-Source Policy

Goal: prove the selected typed stack-source policy is complete or that the
documented fail-closed blocker is source-idea-complete.

Primary target: focused RISC-V tests, shared prepared lookup tests, and backend
bucket.

Actions:
- Run a fresh build.
- Run focused RISC-V prepared edge-publication and shared prepared lookup tests.
- Run an appropriate backend bucket before lifecycle closure.
- Search for evidence of local edge-copy fact rediscovery or testcase-shaped
  matching introduced by this route.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  source idea can close or clearly records the remaining prepared-authority
  blocker.
