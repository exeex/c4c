# LIR Struct Type Printer Authority Readiness Audit Runbook

Status: Active
Source Idea: ideas/open/110_lir_struct_type_printer_authority_readiness_audit.md

## Purpose

Audit whether the `StructNameId` structured path is ready to become printer
authority for LLVM struct type declarations and selected struct type
references.

## Goal

Produce a focused readiness report under `review/` that classifies remaining
rendered struct/type string surfaces and recommends whether printer authority
can move from `type_decls` to `struct_decls`.

## Core Rule

This is an audit checkpoint only. Do not remove legacy rendered type text, do
not switch printer authority, and do not broaden the work into implementation
cleanup.

## Read First

- `ideas/open/110_lir_struct_type_printer_authority_readiness_audit.md`
- Recent review artifacts for ideas 108 and 109, if available under `review/`
- LIR module, verifier, printer, and HIR-to-LIR lowering surfaces that mention
  struct declarations, type declarations, or `StructNameId`

## Current Targets

- `LirModule::type_decls`
- `LirModule::struct_decls`
- `LirTypeRef` struct-name mirrors
- global, function, and extern signature text fields
- HIR-to-LIR layout lookup paths still using `TypeSpec::tag`
- LIR verifier shadow-render and parity checks
- emitted LLVM diffs from focused struct, template, and global-init coverage

## Non-Goals

- Do not perform the printer authority switch.
- Do not delete compatibility fields or verifier shadow checks.
- Do not downgrade tests or expectations to make the audit look cleaner.
- Do not add testcase-shaped shortcuts.
- Do not edit source ideas unless the supervisor explicitly requests a
  lifecycle change after the audit.

## Working Model

Classify every relevant rendered struct/type string surface as one of:

- `printer-authority-ready`: structured render has exact parity and enough
  proof.
- `legacy-proof-only`: retained only for mismatch observation.
- `bridge-required`: downstream still lacks structured identity.
- `printer-only`: final text output only.
- `blocked-by-type-ref`: waiting for more structured `LirTypeRef` coverage.
- `blocked-by-layout-lookup`: waiting for more `StructNameId` layout lookup.

## Execution Rules

- Prefer code inspection and focused proof over broad rewrites.
- Tie blockers to concrete files, functions, and tests.
- Separate printer authority candidates from bridge-required text surfaces.
- Treat expectation rewrites or unsupported downgrades as testcase-overfit.
- Keep the final audit report concise enough for the supervisor to route the
  next idea.

## Ordered Steps

### Step 1: Inventory Struct Type Surfaces

Goal: Find every active rendered-string and structured-ID surface relevant to
LLVM struct type printing.

Primary targets:
- LIR module declarations and type reference definitions
- LIR printer and verifier code
- HIR-to-LIR lowering code that builds declarations, signatures, globals, and
  layout lookups

Actions:
- Inspect definitions and call sites for `LirModule::type_decls`,
  `LirModule::struct_decls`, `LirTypeRef`, `StructNameId`, and
  `TypeSpec::tag`.
- Identify where rendered type strings are still authoritative, where they are
  shadow proof, and where structured identity is already carried.
- Record concrete file/function notes for each surface in `todo.md`.

Completion check:
- The executor can name the complete set of audited surfaces and has not made
  implementation edits.

### Step 2: Classify Readiness And Blockers

Goal: Assign the source idea classification labels to each audited surface.

Actions:
- Mark each surface as `printer-authority-ready`, `legacy-proof-only`,
  `bridge-required`, `printer-only`, `blocked-by-type-ref`, or
  `blocked-by-layout-lookup`.
- Explain each blocker in terms of missing structured identity, missing layout
  lookup authority, or final-output-only behavior.
- Identify any surfaces where previous ideas 107 through 109 left dual-path
  support incomplete.

Completion check:
- The classification separates switch candidates from surfaces that still need
  bridge work or additional structured coverage.

### Step 3: Prove Parity With Focused Coverage

Goal: Check whether current behavior gives enough evidence for or against a
printer authority switch.

Actions:
- Run supervisor-delegated focused tests that exercise struct declarations,
  templates, signatures, globals, externs, and global initialization.
- Inspect emitted LLVM diffs or verifier output where available.
- Do not change expectations as part of this audit.

Completion check:
- `todo.md` records the exact proof commands and result summary for the audit
  packet.

### Step 4: Write The Readiness Report

Goal: Produce the required review artifact for supervisor routing.

Primary target:
- `review/110_lir_struct_type_printer_authority_readiness_audit.md`

Actions:
- Summarize audited surfaces and classification results.
- Tie remaining blockers to concrete files and tests.
- Recommend either a follow-up idea to switch printer authority from
  `type_decls` to `struct_decls`, or more dual-path coverage before that
  switch.
- State explicitly that no legacy removal was performed.

Completion check:
- The report exists under `review/`, covers the acceptance criteria from the
  source idea, and gives the supervisor a clear next-route recommendation.
