# AArch64 Variadic Va Start Helper Lowering Runbook

Status: Active
Source Idea: ideas/open/317_aarch64_variadic_va_start_helper_lowering.md
Activated from: idea 315 closure after Step 4 residual classification

## Purpose

Repair AArch64 `va_start` helper printing so prepared helper payloads become
legal selected assembly instead of raw note-like text in generated `.s` output.

## Goal

Make AArch64 `va_start` helper lowering initialize `va_list` fields through
legal machine instructions while preserving prepared metadata guardrails.

## Core Rule

Progress must be a general AArch64 `va_start` helper lowering capability. Do
not reopen large frame setup/teardown materialization, stack-slot memory
spelling, scalar ALU immediate materialization, frame-layout consistency,
semantic admission, expectations, unsupported classifications, runners, timeout
policy, proof-log contents, or CTest registration.

## Read First

- `ideas/open/317_aarch64_variadic_va_start_helper_lowering.md`
- `ideas/closed/315_aarch64_large_frame_adjustment_materialization.md`
- `ideas/closed/314_aarch64_large_stack_offset_addressing.md`
- `todo.md`
- generated AArch64 artifacts and prepared dumps for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_before.log` / `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current residual fact:
  - `00204.c`: generated assembly contains raw `va.start`,
    `va.start.rsa`, `va.start.initial_offsets`, and `va.start.field` lines in
    `myprintf`.
- Suspected owner surface:
  - `src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call`
  - `PreparedVariadicEntryHelperKind::VaStart`
- Prior-owner guardrails:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
  - `backend_cli_dump_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_block_entry_00204`
  - `backend_aarch64_machine_printer`
  - `backend_aarch64_instruction_dispatch`
  - `backend_aarch64_target_instruction_records`

## Non-Goals

- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication owner.
- Do not repair idea 316's frame-slot/frame-layout consistency residual.
- Do not repair scalar ALU immediate materialization such as
  `mov w9, #503808`; that belongs to idea 318.
- Do not change semantic admission, prepared-handoff contracts, runners,
  timeout policy, expectations, unsupported classifications, or CTest
  registration.
- Do not rely on filenames, function names, helper ids, stack-slot ids,
  diagnostic strings, or c-testsuite numbers.

## Working Model

Idea 315 moved `00204.c` past the large frame adjustment printer diagnostic.
The first remaining assembler blocker is raw variadic helper payload text in
the generated assembly. The owner should translate `VaStart` helper metadata
into selected AArch64 stores and address materialization for the destination
`va_list` fields, while leaving the upstream metadata facts observable in
prepared dumps and note tests.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the exact helper payload shape before editing code.
- Prefer local backend coverage for helper lowering before relying on the
  external c-testsuite representative.
- Preserve prepared metadata guardrails; do not erase handoff facts to avoid
  printing them.
- If the representative advances to scalar ALU immediate materialization,
  frame-layout consistency, runtime mismatch, linker behavior, or timeout,
  record the new first bad fact and return it to lifecycle classification
  unless generated-code evidence proves this owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared target-machine behavior broadly.

## Ordered Steps

### Step 1: Localize Va Start Helper Payloads

Goal: identify the concrete prepared helper fields and AArch64 printer path
that currently emit raw `va.start` text.

Primary target: AArch64 variadic helper lowering and focused dumps for
`00204.c`.

Actions:

- Inspect the generated `00204.c` assembly around the raw `va.start` lines and
  the prepared dump entries that feed them.
- Inspect `print_variadic_call` and related helper data structures enough to
  identify the fields needed to initialize the target `va_list`.
- Compare current note-level guardrails against machine-printer or backend
  coverage gaps.
- Record in `todo.md` the owning code surface, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the missing `va_start` helper lowering fact, the owning code
  surface, representative tests, and the smallest focused proof command for the
  repair.

### Step 2: Lower Va Start To Legal AArch64 Assembly

Goal: replace raw `va.start` helper payload output with legal selected
instructions that initialize the destination `va_list`.

Primary target: code surface identified by Step 1.

Actions:

- Implement a narrow lowering path for `PreparedVariadicEntryHelperKind::VaStart`.
- Store required `va_list` fields using legal AArch64 address and constant
  materialization sequences.
- Preserve prepared metadata and fail closed for unsupported helper shapes.
- Avoid scalar ALU immediate, frame-layout, frame-adjustment, runner,
  expectation, and timeout changes.

Completion check:

- Focused backend proof shows raw `va.start` helper text no longer reaches
  generated assembly, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Backend Coverage

Goal: make AArch64 `va_start` helper lowering observable in local backend tests.

Primary target: existing backend note, dump, machine-printer, dispatch, or
target-instruction tests that already cover adjacent variadic helper behavior.

Actions:

- Add or extend coverage that proves generated helper output uses legal
  selected instructions rather than raw helper text.
- Preserve prepared metadata coverage so semantic handoff facts remain visible.
- Assert behavior at the helper contract level, not one incidental line number
  or stack slot id.

Completion check:

- Local backend coverage exercises the `va_start` lowering contract and passes
  without weakening existing prepared handoff guardrails.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails from ideas 312, 314, and 315, and the `00204.c`
  c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if raw `va.start` helper text still reaches
  generated assembly.

Completion check:

- `todo.md` records fresh proof. The current raw `va.start` assembler
  diagnostic is gone, or the remaining blocker is explicitly localized for the
  next packet.
