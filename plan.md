# AArch64 Inline Assembly Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Activated From: ideas/open/240_aarch64_inline_asm_machine_nodes.md

## Purpose

Convert the archived AArch64 inline-assembly behavior into structured backend
carriers, selected machine records, and final assembler text without recovering
semantics from already-rendered assembly strings.

## Goal

Supported AArch64 inline-asm operands, constraints, clobbers, tied outputs,
immediates, named operands, and modifiers flow through structured BIR/prepared
authority into selected machine records and template substitution, while
unknown or unsupported forms produce explicit diagnostics.

## Core Rule

Inline assembly is a textual external boundary, but codegen authority must be
structured before printing. Do not infer register, stack, clobber, side-effect,
or operand semantics from rendered assembly text.

## Read First

- `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instructions.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

## Current Scope

- Structured inline-asm template, operand, constraint, clobber, side-effect,
  output, tied-operand, immediate, named-operand, and modifier facts.
- Prepared carrier validation that consumes existing register and stack-home
  authority instead of inventing local scratch allocation.
- AArch64 selected machine records for supported inline-asm packets.
- AArch64 template substitution for supported modifiers after operands have
  assigned structured homes.
- Diagnostics for unknown operands, unsupported constraints, malformed ties,
  unsupported modifiers, and missing homes.

## Non-Goals

- Do not build an independent inline-asm register allocator, spill planner, or
  scratch-register convention.
- Do not parse printed AArch64 assembly to recover backend semantics.
- Do not fold atomic exclusive helper selection into inline-asm substitution.
- Do not claim support through one hard-coded inline-asm fixture.
- Do not downgrade unsupported-path expectations or weaken diagnostics without
  explicit supervisor approval.
- Do not reopen completed intrinsic-carrier routes unless a regression is
  found.

## Working Model

- BIR owns the source inline-asm template, dialect metadata, operand list,
  operand names, constraint text, side-effect flag, clobbers, output flags,
  tied operand links, and immediate facts.
- Prepared carriers own completeness plus the concrete register, stack-home,
  immediate, memory, and output authority needed for selection.
- AArch64 selection consumes only complete prepared inline-asm carriers.
- Template substitution happens after selection and reads selected structured
  operands, not original generic call metadata or already-rendered text.
- Unsupported constraints, modifiers, names, ties, or homes are diagnostic
  surfaces, not permission to fabricate registers.

## Execution Rules

- Keep packets narrow and prove each code-changing step with
  `cmake --build --preset default` plus the supervisor-delegated backend subset.
- Add nearby fail-closed tests for every accepted inline-asm representative.
- Start with a minimal representative only if the carrier shape generalizes to
  positional, named, tied, immediate, and clobber cases.
- Prefer structured enums, operand records, and diagnostics over string
  shortcuts.
- Record blockers in `todo.md`; create a separate open idea if execution finds
  a distinct allocator, TLS, atomic-helper, or parser initiative.

## Step 1: Inventory Inline-Asm Authority

Goal: identify the current inline-asm data path and the smallest representative
set needed to avoid single-fixture support.

Primary targets:

- `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Inspect how inline-asm payload text, operands, clobbers, side effects, and
  outputs are currently represented or dropped.
- Inventory existing prepared register and stack-home authority that selection
  can consume.
- Choose a first implementable representative set covering positional,
  immediate, named, tied-output, and clobber facts without requiring a new
  allocator.
- Record unsupported constraints, modifiers, and missing-home cases that must
  remain diagnostic-only.

Completion check:

- `todo.md` contains a representative matrix and identifies the first
  implementation packet.
- No BIR, prepared, dispatch, or printer behavior changes are required in this
  step.

## Step 2: Define Structured Inline-Asm Carriers

Goal: add BIR and prepared carrier authority for supported inline-asm operands
without selecting AArch64 machine records yet.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Represent template text, dialect, side effects, clobbers, operand names,
  constraints, immediates, outputs, and tied-operand links as structured facts.
- Validate prepared carriers against available register and stack-home
  authority.
- Preserve diagnostics for unsupported constraints, unknown names, malformed
  ties, unsupported operand kinds, and missing homes.
- Expose carrier facts through prepared-printer output for review and tests.

Completion check:

- BIR and prepared-printer tests prove supported carrier fields and nearby
  fail-closed diagnostics.
- AArch64 dispatch remains closed for inline-asm carriers until Step 3.

## Step 3: Select AArch64 Inline-Asm Machine Records

Goal: lower complete prepared inline-asm carriers into AArch64 selected machine
records without formatting template operands yet.

Primary targets:

- `src/backend/mir/aarch64/codegen/instructions.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:

- Add a selected record that carries template text, side-effect state,
  clobbers, operands, homes, immediate values, output facts, ties, and names.
- Select only complete prepared carriers with supported constraints and homes.
- Keep malformed, incomplete, target-mismatched, or unsupported carriers
  diagnostic-only or non-selected.
- Avoid adding local scratch-register assignment in the dispatch layer.

Completion check:

- Dispatch tests prove complete carrier selection and fail-closed cases.
- BIR and prepared-printer tests from Step 2 still pass.

## Step 4: Implement Template Substitution

Goal: print supported AArch64 inline-asm templates from selected structured
records.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Substitute positional operands, named operands, raw immediates, tied outputs,
  and supported AArch64 modifiers from selected structured operands.
- Emit clobber and side-effect-sensitive assembly boundaries in the existing
  printer style.
- Diagnose unknown operands, unsupported modifiers, missing ties, and
  unsupported constraints instead of fabricating spelling.
- Keep atomic helper lowering outside this substitution path.

Completion check:

- Machine-printer tests prove positional, named, tied, immediate, and supported
  modifier substitution.
- Fail-closed tests cover unknown names, unsupported constraints/modifiers, and
  missing homes.

## Step 5: Expand Output, Tie, And Clobber Coverage

Goal: harden the selected and printed path so inline-asm side effects,
clobbers, outputs, and tied operands interact correctly with prepared homes.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Add coverage for output homes, tied input/output homes, clobber lists, and
  side-effect flags that were not required by the first selected packet.
- Preserve the invariant that prepared homes, not local string parsing, drive
  selected operands.
- Record any allocator-scale or unsupported-constraint gaps in `todo.md`
  instead of broadening this route.

Completion check:

- Prepared, dispatch, and printer tests prove output/tie/clobber behavior for
  at least one non-trivial representative.
- Unsupported or allocator-scale cases remain explicit diagnostics or
  lifecycle follow-up notes.

## Step 6: Broader Backend Validation And Closure Readiness

Goal: prove the inline-asm machine-node route did not regress adjacent backend
families and decide whether the source idea is complete.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run the supervisor-selected broader backend validation, normally
  `cmake --build --preset default` followed by `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Compare matching before/after logs with the regression guard when closure is
  requested.
- Record remaining unsupported constraints, modifiers, or allocator-scale
  follow-up needs in `todo.md`.
- Ask the plan owner to close only if the source idea is complete; otherwise
  split any remaining durable scope into a new `ideas/open/*.md`.

Completion check:

- Broader backend validation passes.
- Regression guard passes for the chosen close scope.
- `todo.md` clearly states whether closure is valid or what follow-up idea is
  required.
