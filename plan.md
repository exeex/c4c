# Prepared Callee-Save Slot Placement Runbook

Status: Active
Source Idea: ideas/open/241_prepared_callee_save_slot_placement.md
Activated from: ideas/open/241_prepared_callee_save_slot_placement.md

## Purpose

Add prepared-frame authority that explicitly maps each saved callee register to
the frame slot and stack offset where that register is saved.

## Goal

Expose saved-register-to-slot placement facts so later AArch64 callee-save
save/restore lowering can consume structured prepared data instead of deriving
layout from `save_index`, register names, `frame_slot_order`, or generic slot
offset ordering.

## Core Rule

This runbook prepares callee-save slot placement only. Do not emit AArch64
callee-save store/load instructions, infer frame layout in target lowering, or
broaden into unrelated frame allocation, outgoing call areas, variadic save
areas, or preserved-value extent work.

## Read First

- `ideas/open/241_prepared_callee_save_slot_placement.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- AArch64 call/frame contracts only as consumers, not as implementation owners

## Current Targets

- A prepared/shared carrier that ties each `PreparedSavedRegister` to a
  `PreparedFrameSlotId` and concrete stack offset.
- Preservation of register bank, register name, contiguous width, occupied
  register names, save/restore ordering, slot id, and stack offset as
  structured facts.
- Prepared dumps or frame-plan observations that make the mapping visible
  before target lowering consumes it.
- Focused proof that later AArch64 callee-save lowering can consume the
  mapping without reconstructing frame layout locally.

## Non-Goals

- Do not emit AArch64 callee-save stores or loads in this prerequisite.
- Do not teach AArch64 target codegen to infer slots from `save_index`,
  `frame_slot_order`, register names, sorted offsets, or object names.
- Do not redesign general frame-slot allocation.
- Do not implement variadic register-save areas, outgoing call stack areas, or
  stack-slot preserved-value extents.
- Do not weaken callee-save tests, mark supported cases unsupported, or claim
  progress through expectation rewrites.

## Working Model

Prepared frame facts already expose saved callee registers and frame slots as
separate lists. This route adds an explicit relationship between them at the
prepared layer. If the relationship cannot be established from prepared/frame
authority, keep the state fail-closed and record the missing fact instead of
moving the decision into target codegen.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Start by inspecting how saved callee registers and frame slots are currently
  produced; do not assume `save_index` and slot order have a stable semantic
  relationship without prepared authority.
- Add the carrier before any target consumer depends on it.
- Preserve existing simple fixed-frame behavior and existing prepared frame
  facts.
- Make diagnostics explicit for missing or unsupported placement states.
- Treat AArch64-side inference, named-case matching, expectation weakening, or
  broad frame allocation rewrites as route drift.
- For code-changing packets, prove with a build plus the supervisor-chosen
  focused prepared frame/callee-save subset. Escalate to broader backend
  validation if shared frame layout, regalloc, or target-visible contracts
  change beyond the new carrier.

## Ordered Steps

### Step 1: Inspect Placement Authority

Goal: identify the prepared owner that can safely connect saved callee
registers to frame slots and stack offsets.

Primary targets:

- `PreparedSavedRegister`
- `PreparedFrameSlot`
- `PreparedFramePlanFunction`
- `populate_frame_plan`
- prepared frame-plan printer output
- focused callee-save/frame-plan tests

Actions:

- Trace how `saved_callee_registers` and `frame_slot_order` are populated.
- Identify any existing slot objects or allocation facts that already
  represent callee-save storage.
- Determine the smallest prepared carrier shape needed for register identity,
  save order, slot id, offset, size, alignment, and placement visibility.
- Record the first implementation packet target and focused proof subset in
  `todo.md`.

Completion check:

- `todo.md` records whether execution can proceed to carrier definition or
  must split a narrower prerequisite with exact missing facts.

### Step 2: Define The Placement Carrier

Goal: add a structured prepared/shared record for saved-register slot
placement.

Actions:

- Define a carrier that ties a saved callee register to a frame slot id and
  stack offset.
- Preserve register bank, register name, contiguous width, occupied register
  names, save index, register placement, slot id, offset, and relevant storage
  extent facts.
- Keep the carrier attached to the prepared frame plan or another prepared
  frame-owned surface that consumers can query without target-local layout
  inference.
- Add fail-closed handling for incomplete placement data.

Completion check:

- Prepared data can represent the saved-register-to-slot relationship without
  relying on list position or target-specific reconstruction.

### Step 3: Populate Placement Facts

Goal: produce placement records from prepared frame/layout authority.

Actions:

- Connect saved callee registers to the prepared frame slots that own their
  save homes.
- Preserve deterministic ordering for save/restore consumers.
- Avoid deriving placement from AArch64-specific instruction requirements.
- Preserve existing frame size, alignment, dynamic-stack, and frame-slot-order
  behavior.
- Add focused tests for one-register and multi-register callee-save cases.

Completion check:

- Prepared frame facts expose a stable mapping from each saved callee register
  to a slot id and offset for representative fixed-frame cases.

### Step 4: Publish And Observe Placement

Goal: make the new placement authority visible to tests and later consumers.

Actions:

- Update prepared dump or frame-plan observation output to print the mapping.
- Add or update focused prepared-frame tests that assert slot id, offset, size,
  alignment, and register identity.
- Verify missing placement states produce explicit diagnostics or absent
  unsupported-state records rather than target-local fallback behavior.

Completion check:

- A test can prove the placement relation directly from prepared output or
  prepared structures, without inspecting target AArch64 lowering.

### Step 5: Validate And Summarize

Goal: prove the prepared callee-save placement contract and leave clear
handoff notes for the later AArch64 consumer route.

Actions:

- Run the supervisor-chosen build and focused prepared frame/callee-save test
  subset.
- Escalate to broader backend validation if frame layout or regalloc behavior
  changed outside the placement carrier.
- Summarize supported placement states, fail-closed cases, and any remaining
  consumer work in `todo.md`.
- Ask the supervisor whether to close this prerequisite or activate the later
  AArch64 callee-save save/restore route.

Completion check:

- Prepared frame facts expose explicit saved-register slot placement, existing
  simple frame behavior remains stable, and later AArch64 lowering no longer
  needs to infer callee-save homes from generic slot ordering.
