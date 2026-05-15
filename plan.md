# Inline Assembly Structured Clobber Authority Runbook

Status: Active
Source Idea: ideas/open/241_inline_asm_clobber_authority.md
Activated from split closure of ideas/closed/240_aarch64_inline_asm_machine_nodes.md

## Purpose

Add structured inline-asm clobber authority before backend clobber support is
claimed.

## Goal

Clobber lists flow from source/LIR into BIR, prepared records, selected AArch64
inline-asm machine records, and diagnostics without parsing rendered assembly
text.

## Core Rule

Do not recover clobber semantics from template text, final assembly, or
diagnostic strings. Accepted clobbers must come from structured upstream facts.

## Read First

- `ideas/open/241_inline_asm_clobber_authority.md`
- `ideas/closed/240_aarch64_inline_asm_machine_nodes.md`
- `todo.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

## Current Scope

- Source/LIR representation for inline-asm clobber lists.
- BIR, prepared, and selected-machine clobber carriers.
- AArch64 target validation for supported clobber spellings.
- Explicit diagnostics for unsupported or target-invalid clobbers.
- Regression proof that existing inline-asm operand, name, immediate, modifier,
  side-effect, output, and tied-home behavior remains intact.

## Non-Goals

- Do not parse clobbers from template text or final assembler output.
- Do not introduce scratch-register allocation, spill policy, or allocator
  coallocation changes.
- Do not weaken unsupported-path diagnostics or tests.
- Do not fold memory/address constraints or tied-home allocation policy into
  this clobber route.
- Do not refactor unrelated backend lowering or printer code.

## Working Model

- Source and LIR own the raw clobber list as structured facts.
- BIR carries clobbers alongside inline-asm operands, constraints, side effects,
  outputs, ties, names, immediates, and modifiers.
- Prepared records preserve clobbers and reject forms that lack enough target
  authority.
- AArch64 selection accepts only complete structured clobbers.
- Printing emits selected inline assembly from structured records and leaves
  unsupported clobbers diagnostic-only.

## Execution Rules

- Keep each packet narrow and update `todo.md` with packet progress and proof.
- Add a supported-path test and nearby fail-closed test for every newly accepted
  clobber surface.
- Prefer structured records and enums over string matching.
- Preserve all completed inline-asm behavior from the closed 240 idea.
- If a needed parser/source representation is absent, record the blocker in
  `todo.md` before widening the route.

## Step 1: Inventory Clobber Ingress

Goal: identify where inline-asm clobber facts should enter source/LIR and how
far current records can carry them.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- existing inline-asm tests under `tests/backend/`

Actions:

- Inspect current inline-asm source, LIR, and BIR shapes for clobber fields or
  missing fields.
- Identify the narrowest structured clobber carrier that can reach BIR without
  parsing printed assembly.
- Record the first implementation packet and proof subset in `todo.md`.

Completion check:

- `todo.md` states the ingress point, missing upstream authority if any, and
  the first code packet.

## Step 2: Carry Clobbers Through BIR And Prepared Records

Goal: preserve structured clobber facts through BIR and prepared inline-asm
records.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Add or thread clobber fields through BIR inline-asm records.
- Preserve clobbers in prepared inline-asm records.
- Print or dump prepared clobber facts in the existing backend diagnostic style.
- Keep unsupported clobber spellings fail-closed.

Completion check:

- BIR/prepared tests prove structured clobber carriage and unsupported clobber
  diagnostics.

## Step 3: Select AArch64 Clobber Facts

Goal: select AArch64 inline-asm machine records only when structured clobbers
are complete and target-valid.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Add selected-machine clobber carriage.
- Validate supported AArch64 clobber spellings from structured facts.
- Keep unknown, malformed, or target-invalid clobbers diagnostic-only.
- Do not infer clobbers from the inline-asm template string.

Completion check:

- AArch64 dispatch and printer tests prove selected clobber carriage and
  fail-closed diagnostics.

## Step 4: Regression And Lifecycle Check

Goal: prove clobber work did not regress the completed inline-asm surfaces and
decide whether the clobber idea can close.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run the supervisor-selected backend proof, normally `cmake --build --preset
  default` followed by `ctest --test-dir build -j --output-on-failure -R
  '^backend_'`.
- Compare matching before/after logs with the regression guard when closure is
  requested.
- Record whether clobber authority is complete or whether any upstream parser
  ownership must split again.

Completion check:

- Backend validation passes.
- Regression guard passes for the chosen close scope if closure is requested.
- `todo.md` clearly states whether source-idea closure is valid or names the
  follow-up idea that owns remaining scope.
