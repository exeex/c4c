# AArch64 Instruction And Printer Surface Contraction Runbook

Status: Active
Source Idea: ideas/open/82_aarch64_instruction_printer_surface_contraction.md

## Purpose

Reduce AArch64-local instruction-record and printer-surface duplication in
`instruction.*` and `machine_printer.*` while preserving the current record
contract, assembly spelling, and diagnostic behavior.

## Goal

Consolidate same-shaped target-local naming, status, and printer-validation
helpers into clearer local tables or utilities without moving AArch64 printer
policy into shared BIR.

## Core Rule

Keep AArch64 record-schema policy, mnemonic spelling, register spelling, memory
operand spelling, and printer validation target-local unless the source idea is
explicitly replaced by a new lifecycle decision.

## Read First

- `ideas/open/82_aarch64_instruction_printer_surface_contraction.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Targets

- Record-kind name tables.
- Opcode mnemonic tables.
- Prepared-record error name tables.
- Status builders.
- Printer validation helpers.
- Repeated instruction/printer helper shapes that can remain AArch64-local.

## Non-Goals

- Do not move AArch64 concrete spelling or record-schema policy into BIR,
  prealloc, or shared lowering.
- Do not change textual assembly output except for intentional, reviewed
  formatting fixes with proof.
- Do not combine this route with dispatch relocation, call lowering relocation,
  or broader backend cleanup.
- Do not delete printer validation to reduce code size.

## Working Model

- `instruction.*` owns AArch64 machine-record structure, status construction,
  and record-name surfaces.
- `machine_printer.*` owns AArch64 concrete assembly spelling and printer-side
  validation.
- Shared local helpers are acceptable only when they reduce duplicate
  target-local shapes and keep ownership clearer than the duplicated code.

## Execution Rules

- Start each code-changing step with a small inventory of the exact duplicate
  shape being contracted.
- Preserve existing diagnostic text and failure paths unless the supervisor
  explicitly accepts a reviewed diagnostic change.
- Keep each implementation packet owner-focused: instruction naming/status,
  printer validation, or printer tables.
- Run build proof after each contraction packet.
- Use focused backend or dump tests that exercise the touched surface before
  marking a step complete.
- Escalate to broader validation before closure if changes span both
  instruction records and printer validation.

## Step 1: Inventory Instruction And Printer Surfaces

Goal: produce a precise contraction map before editing implementation files.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- List record-kind name tables, opcode mnemonic tables, prepared-record error
  name tables, status builders, and printer validation helpers.
- Classify each candidate as keep duplicated, fold into an existing local
  helper, or table-drive locally.
- Identify focused proof commands for representative branch, scalar, memory,
  frame, atomic, call-boundary, aggregate-lane, and printer error paths.
- Record any candidate that would require shared BIR authority as out of scope
  unless a separate idea is created.

Completion check:

- `todo.md` records the selected first implementation packet, exact files to
  touch, validation command, and any no-touch candidates discovered during the
  inventory.

## Step 2: Contract Instruction Record Naming And Status Helpers

Goal: remove same-shaped record-name/status duplication inside
`instruction.*` while preserving the record contract.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`

Actions:

- Convert repeated record-kind, opcode, or prepared-record error naming logic
  into local tables or local helper functions where the mapping stays explicit.
- Consolidate status-builder wrappers only when the resulting diagnostics are
  equivalent and easier to audit.
- Avoid renaming public record surfaces unless every dependent printer/test
  surface is updated with explicit proof.

Completion check:

- Build proof passes.
- Focused backend/dump tests cover changed record names and representative
  prepared-record error diagnostics.

## Step 3: Contract Printer Validation Helpers

Goal: consolidate repeated validation helper shapes in `machine_printer.*`
without weakening printer checks.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- Fold repeated printer validation helpers into narrower local helpers when
  the call sites still state the expected record or operand contract clearly.
- Keep validation close to concrete AArch64 spelling decisions.
- Preserve printer error messages and unsupported-path behavior.

Completion check:

- Build proof passes.
- Focused printer/dump tests cover success and error paths for touched branch,
  scalar, memory, frame, atomic, call-boundary, and aggregate-lane surfaces as
  applicable.

## Step 4: Table-Drive Local Printer Mnemonics Where Clear

Goal: reduce repeated local mnemonic or spelling dispatch only where a table is
more auditable than branch duplication.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`

Actions:

- Replace same-shaped local switch/string logic with target-local tables only
  when entries remain explicit and reviewable.
- Keep register spelling, memory operand spelling, frame spelling, and
  call-boundary spelling in AArch64 printer code.
- Do not introduce shared mnemonic authority.

Completion check:

- Build proof passes.
- Focused dumps show unchanged textual assembly for representative touched
  instruction classes.

## Step 5: Closure Validation And Review Readiness

Goal: prove the contraction did not weaken diagnostics or alter assembly output
outside intentional reviewed fixes.

Actions:

- Run the supervisor-selected broader backend validation set after all
  contraction packets are complete.
- Ensure proof covers representative branch, scalar, memory, frame, atomic,
  call-boundary, aggregate-lane, and printer error paths.
- Check the final diff for source-idea reject signals: shared BIR printer
  authority, deleted validation, unproved record-name changes, or mixed
  dispatch/call cleanup.

Completion check:

- Canonical proof logs are ready for supervisor review.
- `todo.md` identifies any residual candidates that should become separate
  ideas instead of being folded into this runbook.
