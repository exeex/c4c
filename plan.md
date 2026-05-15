# AArch64 Variadic Prepared Storage And Helper Authority Runbook

Status: Active
Source Idea: ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md
Activated from: ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md
Supersedes active runbook: ideas/open/243_aarch64_variadic_machine_node_consumption.md is parked on a prepared-authority blocker.

## Purpose

Provide the prepared/shared storage, scratch-resource, and helper operand-home
facts required before AArch64 variadic helper machine-node lowering can consume
`va_start`, scalar `va_arg`, aggregate `va_arg`, or `va_copy`.

## Goal

Make the missing variadic authority explicit and structurally available, while
keeping AArch64 helper lowering fail-closed until idea 243 can consume those
facts without local ABI reconstruction.

## Core Rule

This runbook prepares authority only. It must not implement selected
machine-node lowering for variadic helpers or move AAPCS64 frame/layout/scratch
decisions into AArch64 target lowering.

## Read First

- `ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md`
- `ideas/open/243_aarch64_variadic_machine_node_consumption.md`
- `ideas/closed/232_aarch64_variadic_function_entry_carriers.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- focused tests under `tests/backend/mir/`

## Current Targets

- `PreparedVariadicEntryPlanFunction` storage facts.
- Register-save-area slot id and stack offset.
- Overflow-area base slot id and base stack offset.
- Helper scratch register and scratch stack byte facts.
- Helper operand-home facts for `va_list` pointers and helper destinations.
- Missing-fact diagnostics that prevent target-local reconstruction.

## Non-Goals

- Do not emit selected machine nodes for `va_start`, scalar `va_arg`,
  aggregate `va_arg`, or `va_copy`.
- Do not infer register-save slots, overflow bases, stack offsets, named
  register counts, `va_list` layout, or scratch allocation in AArch64 target
  lowering.
- Do not weaken existing fail-closed diagnostics or unsupported expectations.
- Do not broaden into unrelated callee-save, preserved-value, memory,
  aggregate-copy, scalar-cast, printer, or frame-allocation rewrites.

## Working Model

Idea 243 remains the consumer. This runbook supplies the missing authority that
Step 1 identified: storage placement for register-save and overflow areas,
explicit helper scratch resources, and concrete homes for helper pointer/result
operands. Once these facts are present and validated, idea 243 can be
reactivated for machine-node consumption.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Add carriers or shared facts at the producer layer that already owns prepared
  frame and variadic entry metadata.
- Preserve fail-closed behavior whenever a required field cannot be populated.
- Treat expectation-only changes, helper renames, diagnostic-only rewrites, and
  selected-node consumption in this prerequisite as route drift.
- For code-changing packets, prove with a build plus the supervisor-chosen
  focused prepared/AArch64 variadic subset. Escalate to broader backend
  validation after shared prepared carrier changes.

## Ordered Steps

### Step 1: Attach Register-Save And Overflow Storage Authority

Goal: Populate the prepared frame-storage facts that AArch64 variadic helper
consumption needs for register-save-area and overflow-area addressing.

Primary targets:

- `PreparedVariadicEntryPlanFunction`
- prepared frame slot and stack-offset population
- missing-fact completeness checks and prepared dumps

Actions:

- Trace where variadic entry register-save and overflow areas are allocated or
  should be allocated in prepared/prealloc state.
- Populate `register_save_area.slot_id` and
  `register_save_area.stack_offset_bytes`.
- Populate `overflow_area.base_slot_id` and
  `overflow_area.base_stack_offset_bytes`.
- Ensure `overflow_area.base_slot_id` participates in missing-required-fact
  diagnostics when absent.
- Add focused coverage for present and incomplete storage facts.

Completion check:

- Prepared variadic entry dumps and dispatch completeness checks expose both
  slot id and stack offset for register-save and overflow base storage, with
  explicit missing-fact diagnostics when any required field is absent.

### Step 2: Add Helper Scratch Resource Authority

Goal: Make helper scratch requirements explicit prepared/shared facts instead
of target-local policy.

Actions:

- Define the producer-owned meaning of
  `helper_resources.scratch_register_count` and
  `helper_resources.scratch_stack_bytes` for recognized variadic helpers.
- Populate the scratch facts for `va_start`, scalar `va_arg`, aggregate
  `va_arg`, and `va_copy` classification paths.
- Preserve fail-closed diagnostics for missing scratch facts.
- Add focused tests that distinguish populated scratch facts from unsupported
  helper lowering.

Completion check:

- Every recognized variadic helper has explicit scratch register and stack-byte
  facts available to consumers, and missing scratch facts diagnose without
  selected-node lowering.

### Step 3: Add Helper Operand-Home Authority

Goal: Preserve concrete homes for helper pointer and destination operands so
selected machine-node consumers do not re-derive them from raw BIR operands.

Actions:

- Define structured helper operand-home records for `va_start` destination
  `va_list` pointers.
- Define scalar `va_arg` result homes and source `va_list` homes.
- Define aggregate `va_arg` destination payload homes and source `va_list`
  homes.
- Define `va_copy` destination and source `va_list` homes.
- Connect records to existing helper classification without claiming selected
  machine-node support.
- Add focused record/dump coverage for each helper family.

Completion check:

- Helper call records or prepared/shared carriers expose concrete operand-home
  facts for all four helper families, while AArch64 lowering still fails
  closed until idea 243 consumes them.

### Step 4: Validate And Hand Back To Machine-Node Consumption

Goal: Prove the prerequisite authority and make the lifecycle handoff back to
idea 243 explicit.

Actions:

- Run the supervisor-chosen build and focused variadic prepared/backend subset.
- Escalate to broader backend validation if shared prepared or printer-visible
  facts changed beyond one narrow carrier.
- Summarize which fields are now available and any remaining blockers in
  `todo.md`.
- Ask the supervisor to route plan-owner to reactivate idea 243 only if the
  prerequisite authority is complete.

Completion check:

- The prerequisite facts are structurally present, incomplete facts still fail
  closed, and `todo.md` names whether idea 243 is ready to resume.
