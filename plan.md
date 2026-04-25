# LIR Struct Declaration Printer Authority Switch Runbook

Status: Active
Source Idea: ideas/open/111_lir_struct_decl_printer_authority_switch.md

## Purpose

Switch LLVM struct declaration printing from legacy rendered declaration text
to structured `LirModule::struct_decls` authority.

## Goal

Make emitted LLVM struct declaration lines come from
`render_struct_decl_llvm()` over `struct_decls`, while keeping
`type_decls` available as verifier and backcompat proof.

## Core Rule

Only switch struct declaration printer authority. Do not remove legacy
`type_decls`, broaden type-reference authority, rewrite expectations, or move
backend/layout consumers away from their current sources.

## Read First

- `ideas/open/111_lir_struct_decl_printer_authority_switch.md`
- `review/110_lir_struct_type_printer_authority_readiness_audit.md`, if the
  supervisor explicitly makes it available as context
- LIR module, verifier, and LLVM printer code around `LirModule::type_decls`,
  `LirModule::struct_decls`, `render_struct_decl_llvm()`, and
  `verify_struct_decl_shadows()`

## Current Targets

- LLVM struct declaration emission order and text
- `LirModule::struct_decls`
- `LirModule::type_decls`
- `render_struct_decl_llvm()`
- `verify_struct_decl_shadows()` and any related parity checks
- focused tests that cover struct declarations, templates, signatures,
  globals, externs, ABI, variadics, and aggregate layout

## Non-Goals

- Do not remove `type_decls`.
- Do not demote global, function, extern, call, or raw `LirTypeRef` text
  fields.
- Do not switch backend, BIR, or MIR aggregate layout consumers away from
  `type_decls`.
- Do not broaden HIR-to-LIR layout lookup authority.
- Do not rewrite expectations or downgrade tests to make the switch pass.
- Do not add testcase-shaped matching for a named failing case.

## Working Model

`struct_decls` becomes the printer authority only for LLVM struct declaration
lines. `type_decls` remains populated as the legacy rendered mirror and
continues to participate in verifier shadow/parity checks until a later
removal audit proves it can be deleted.

## Execution Rules

- Keep each implementation step behavior-preserving except for the intended
  authority source of struct declaration emission.
- Preserve emitted LLVM parity for existing focused coverage.
- Treat verifier parity failures as signal to fix the structured declaration
  path, not as permission to weaken checks.
- Use focused proof first, then broader validation when the printer switch has
  passed the narrow set.
- Any test change must prove semantic authority movement, not merely update a
  narrow expected string.

## Ordered Steps

### Step 1: Inspect Declaration Emission And Parity Paths

Goal: Identify the exact LLVM printer loop and verifier parity checks that
currently make `type_decls` authoritative for struct declaration lines.

Primary targets:
- LIR module declaration storage
- LLVM printer struct declaration emission
- `render_struct_decl_llvm()`
- `verify_struct_decl_shadows()`

Actions:
- Inspect where emitted LLVM struct declaration lines are collected and
  ordered.
- Confirm `struct_decls` carries all data needed to render the same
  declaration text for focused coverage.
- Confirm `type_decls` remains populated before and after the printer emits
  declarations.
- Record any ordering or parity risks in `todo.md` before editing code.

Completion check:
- The executor can name the printer edit point, the verifier checks that must
  remain, and the narrow proof command for the first implementation step.

### Step 2: Switch Struct Declaration Printing To `struct_decls`

Goal: Make LLVM struct declaration emission source declaration lines from
structured `struct_decls`.

Primary targets:
- LLVM printer declaration emission code
- `render_struct_decl_llvm()`

Actions:
- Replace only the struct declaration emission source with iteration over
  `LirModule::struct_decls`.
- Preserve existing output order and exact rendered text.
- Keep `LirModule::type_decls` population unchanged.
- Keep verifier shadow/parity checks active.

Completion check:
- Focused struct declaration and mirror tests pass without expectation
  downgrades.

### Step 3: Prove Legacy Shadow And Backcompat Behavior

Goal: Demonstrate that `type_decls` is still present and catches mismatches
  after the authority switch.

Primary targets:
- verifier parity tests
- any existing tests around declaration shadow mismatch behavior

Actions:
- Run or add focused proof that structured declarations and legacy text still
  agree.
- If a new test is needed, make it exercise general verifier parity behavior
  rather than a single named testcase.
- Do not weaken or remove `verify_struct_decl_shadows()`.

Completion check:
- Proof shows mismatches between `struct_decls` rendering and `type_decls`
  are still rejected.

### Step 4: Run Focused LLVM Parity Coverage

Goal: Preserve emitted LLVM parity across the coverage called out by the
source idea.

Actions:
- Run supervisor-delegated focused tests for struct declarations, templates,
  signatures, globals, externs, ABI, variadics, and aggregate layout.
- Inspect failures for real authority-switch regressions before changing
  tests.
- Record exact commands and results in `todo.md`.

Completion check:
- Focused coverage passes, and any remaining failure is classified as a real
  blocker rather than hidden by expectation rewrites.

### Step 5: Broader Validation Checkpoint

Goal: Confirm the printer authority switch did not regress adjacent LIR,
frontend, or runtime coverage.

Actions:
- Run the broader validation selected by the supervisor after focused proof is
  green.
- Preserve canonical `test_before.log` and `test_after.log` policy if this is
  accepted as a regression-guard slice.
- Do not include unrelated cleanup in the validation slice.

Completion check:
- The broader validation result is recorded in `todo.md`, and the active idea
  can be judged for closure or follow-up by the supervisor.
