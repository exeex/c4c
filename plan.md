# AArch64 Inline Assembly Remaining Scope Runbook

Status: Active
Source Idea: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Replaces: exhausted Step 1-6 runbook for the same source idea

## Purpose

Finish the remaining AArch64 inline-assembly machine-node scope without
reopening the completed structured positional-register path or claiming
source-idea completion while named operands, clobbers, memory/address
constraints, or allocator-scale tied-output policy remain unsupported.

## Goal

Named operands, structured clobbers, memory/address constraints, and tied
output/coallocation behavior either lower through structured backend authority
or remain explicitly documented diagnostic surfaces with durable follow-up
ownership.

## Core Rule

Do not recover inline-asm semantics from rendered assembly text. Every accepted
operand, clobber, constraint, tie, and modifier must flow through structured
BIR, prepared, selected-machine, and printer authority.

## Already Implemented

- Structured positional register input support.
- One register output representative.
- Numeric tied-operand facts for diagnostic and simple supported paths.
- Integer immediate substitution.
- `%wN` and `%xN` AArch64 register-width modifiers.
- Side-effect marking through prepared and selected records.
- Explicit diagnostics for unsupported forms covered by the completed runbook.
- Broader backend validation passed with 139/139 backend tests.

## Read First

- `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
- `todo.md`
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

- `%[name]` template references that substitute from retained structured
  operand names.
- Structured clobber ingress, prepared carriage, selected-machine carriage,
  and printer behavior when upstream source/LIR authority exists.
- Memory and address constraints that can consume existing prepared register or
  stack-home authority without creating a local inline-asm allocator.
- Tied output/input coallocation policy that can be proven through existing
  prepared-home authority, plus explicit diagnostics for cases requiring a
  larger allocator decision.
- Durable split notes for any remaining source-idea work that proves larger
  than this backend runbook.

## Non-Goals

- Do not rebuild an independent inline-asm register allocator, spill planner,
  or scratch-register convention.
- Do not weaken unsupported-path tests or diagnostics to claim progress.
- Do not add named-case shortcuts for one inline-asm fixture.
- Do not parse final AArch64 assembly to infer operand, clobber, or constraint
  facts.
- Do not fold atomic exclusive helper selection into inline-asm template
  substitution.
- Do not refactor unrelated backend lowering or allocator code.

## Working Model

- BIR owns source template text, operand names, constraint text, side-effect
  state, clobber facts when available, output facts, tie links, and immediate
  facts.
- Prepared carriers validate whether each operand has enough register,
  stack-home, immediate, memory, or address authority to be selected.
- AArch64 dispatch selects only complete supported carriers.
- Machine printing substitutes from selected structured operands and reports
  unsupported names, clobbers, constraints, modifiers, or homes explicitly.
- Allocator-scale decisions are not hidden in dispatch or printing; if a tie or
  memory/address form needs coallocation policy not present in prepared
  authority, it remains diagnostic-only or becomes a separate open idea.

## Execution Rules

- Keep each packet narrow and prove code changes with `cmake --build --preset
  default` plus the supervisor-delegated backend subset.
- Add a supported-path test and nearby fail-closed test for every newly
  accepted inline-asm surface.
- Prefer structured enums and records over string matching on rendered
  templates.
- Preserve all completed positional-register behavior while extending names,
  clobbers, constraints, and ties.
- Record blockers in `todo.md`; ask the plan owner to split a new
  `ideas/open/*.md` only when the remaining work becomes a distinct parser,
  source/LIR, allocator, or constraint-policy initiative.

## Step 1: Rebaseline Remaining Inline-Asm Gap Matrix

Goal: convert the Step 6 closure notes into a precise execution matrix for the
remaining source-idea scope.

Primary targets:

- `todo.md`
- `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
- existing inline-asm tests under `tests/backend/`

Actions:

- Inspect the completed tests and current diagnostics for named operands,
  clobbers, memory/address constraints, and tied outputs.
- Identify which gaps can be solved inside the current backend carrier path and
  which require source/LIR or allocator-scale follow-up ownership.
- Choose the first code packet and record the proof command the supervisor
  should delegate.
- Do not edit implementation files in this inventory step unless the supervisor
  delegates a separate executor packet.

Completion check:

- `todo.md` contains a concise matrix of remaining supported, diagnostic-only,
  and follow-up-required surfaces.
- The next executor packet has one concrete target surface and proof subset.

## Step 2: Implement Named Operand Substitution

Goal: support `%[name]` template references from retained structured operand
names without falling back to string-derived semantics.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instructions.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Preserve operand-name facts through prepared and selected inline-asm records.
- Map `%[name]` references to selected operands using structured name metadata.
- Keep duplicate, missing, or malformed names diagnostic-only.
- Prove named operands with both register and immediate representatives where
  existing carrier authority supports them.

Completion check:

- Machine-printer tests prove named substitution and unknown-name diagnostics.
- Existing positional and numeric tie substitution tests still pass.

## Step 3: Carry Structured Clobber Authority

Goal: represent clobber facts through the backend path when source/LIR provides
them, and keep absent or unsupported clobber authority explicit.

Primary targets:

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

Actions:

- Define the structured clobber carrier expected by BIR, prepared records, and
  selected AArch64 inline-asm records.
- Consume clobbers only from structured source/LIR authority; do not parse them
  out of template text.
- Print or annotate clobber effects in the existing backend style when the
  target representation supports it.
- If source/LIR cannot provide clobbers in this route, record the exact missing
  upstream authority in `todo.md` and request a split follow-up idea.

Completion check:

- Tests prove carried clobber facts for a structured input, or `todo.md`
  records why clobbers must move to a source/LIR follow-up before backend
  support can continue.
- Unsupported clobber spellings remain explicit diagnostics.

## Step 4: Handle Memory And Address Constraints

Goal: support the memory/address constraints that can be lowered from existing
prepared authority and diagnose the forms that need new allocation policy.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instructions.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Classify supported memory/address constraints against existing prepared
  register and stack-home records.
- Select structured memory/address operands only when the home authority is
  complete and target-valid.
- Keep unsupported constraints, missing homes, and target-invalid address forms
  diagnostic-only.
- Do not invent scratch registers or spills in dispatch or printing.

Completion check:

- Tests prove one supported memory/address representative if available from
  current prepared authority.
- Tests prove unsupported or missing-home memory/address constraints fail
  closed.

## Step 5: Resolve Tied Output Coallocation Boundaries

Goal: make tied output/input behavior honest about what existing prepared-home
authority can guarantee.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Verify that tied outputs and inputs share a concrete prepared home before
  selection accepts them.
- Diagnose mismatched, missing, or allocator-dependent tied homes.
- Keep coallocation policy out of template substitution unless prepared records
  already prove the shared home.
- If real support requires allocator changes, record the boundary in `todo.md`
  and request a separate allocator-policy idea.

Completion check:

- Tests prove accepted tied output/input selection only for shared structured
  homes.
- Tests prove allocator-dependent or mismatched ties fail closed.

## Step 6: Remaining-Scope Validation And Lifecycle Decision

Goal: prove the completed remaining-scope packets and decide whether the source
idea can close or must split durable leftovers.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run the supervisor-selected broader backend validation, normally
  `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Compare matching before/after logs with the regression guard if closure is
  requested.
- Record whether named operands, clobbers, memory/address constraints, and
  tied-output coallocation are complete, diagnostic-only by design, or split
  into durable follow-up ideas.
- Ask the plan owner to close only if the source idea itself is complete after
  this remaining-scope pass.

Completion check:

- Broader backend validation passes.
- Regression guard passes for the chosen close scope if closure is requested.
- `todo.md` clearly states whether source-idea closure is valid, or names the
  follow-up `ideas/open/*.md` files that own any remaining durable scope.
