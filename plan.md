# AArch64 Aggregate-Lane Helper/Table Contraction Runbook

Status: Active
Source Idea: ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md

## Purpose

Contract the local AArch64 aggregate-lane helper and table surface around byval
lane publication while preserving construction behavior, printer assembly
output, diagnostics, scratch choices, and chunk-width selection.

## Goal

Reduce duplicated local helper/table spelling for aggregate register-lane
publication without moving byval ABI authority, selected-lane validation, or
printer diagnostics out of their current owners.

## Core Rule

This is a behavior-preserving local contraction. Do not change assembly output,
unsupported-path contracts, diagnostics, scratch selection, chunk-width
selection, or the ownership split between `calls.cpp` construction and
machine-printer emission.

## Read First

- `ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Targets And Scope

- Local aggregate-lane load mnemonic and printable chunk helper/table surfaces
  in `src/backend/mir/aarch64/codegen/instruction.cpp`.
- Existing public declarations in `instruction.hpp` only where current
  construction or printer paths require them.
- Aggregate register-lane publication paths consumed from `calls.cpp`.
- Printer paths that spell aggregate register-lane publication from
  `machine_printer.cpp`.

## Non-Goals

- Do not move AArch64 byval lane classification, selected-lane extent,
  prepared-source validation, lane/chunk coverage, or size-limit authority out
  of `calls.cpp`.
- Do not fold stack-lane inline-asm publication into this register-lane helper
  contraction.
- Do not delete or weaken printer rejection checks for missing scratch, missing
  destination lane, missing printable source chunk, or missing chunk load
  mnemonic.
- Do not perform a broad call-boundary, aggregate-lane, instruction-record, or
  machine-printer rewrite.
- Do not change assembly output, unsupported-path contracts, diagnostics, or
  scratch selection.

## Working Model

Classify each aggregate-lane helper/table surface before changing code:

- `construction-owned`: decides byval lane classification, selected-lane
  extent, prepared-source validity, lane/chunk coverage, or size limits and
  must stay in `calls.cpp`.
- `printer-owned`: owns final assembly spelling, scratch use, OR assembly,
  byte-shifted lane placement, or printer rejection diagnostics.
- `local-helper-table`: selects load mnemonics, printable chunk widths,
  destination lanes, aggregate-lane memory recognition, or scratch exclusions
  and may be contracted if behavior stays equivalent.
- `not-this-idea`: belongs to stack-lane inline assembly, broad call-boundary
  records, or generic machine-printer entry points.

## Execution Rules

- Start by auditing the helper/table surface and choosing the smallest
  contraction boundary.
- Keep `calls.cpp` ABI construction decisions and `machine_printer.cpp`
  printable assembly constraints in their current owners.
- Contract duplicated spelling only after mapping every existing construction
  and printer consumer.
- Preserve public declarations unless a helper can be made private without
  changing current consumers.
- For every code-changing step, run build proof plus focused AArch64 backend
  tests covering aggregate register-lane publication.
- Use matching `test_before.log` and `test_after.log` when the helper/table
  surface or public declarations move.

## Steps

### Step 1: Inventory Aggregate-Lane Helper And Table Surfaces

Goal: map the current helper/table responsibilities and consumers before any
implementation changes.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- Locate aggregate-lane load mnemonic selection, printable chunk selection,
  chunk printability checks, destination-lane derivation, aggregate lane memory
  helpers, and scratch exclusion helpers/tables.
- Record each helper or table with its current consumers and classify it using
  the working model.
- Identify the smallest local contraction candidate that does not move
  construction or printer authority.
- Choose the focused proof commands for register-sourced and
  frame-slot-sourced aggregate register-lane publication.

Completion check:

- `todo.md` records the helper/table inventory, selected first contraction
  boundary, retained owner boundaries, and focused proof commands.

### Step 2: Contract Private Helper/Table Spelling

Goal: reduce duplicated local spelling for private helper/table surfaces while
preserving all consumers.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.cpp`

Actions:

- Contract only helpers or tables classified as `local-helper-table` in Step 1.
- Preserve load mnemonic selection, printable chunk selection, destination-lane
  derivation, aggregate-lane memory recognition, and scratch exclusion
  behavior.
- Keep construction-owned validation in `calls.cpp` and final printer-owned
  checks in `machine_printer.cpp`.
- Avoid touching public declarations unless a private contraction requires no
  external consumer changes.

Completion check:

- Build proof and focused aggregate-lane backend tests pass.
- `todo.md` names the contracted private helpers/tables, unchanged consumers,
  and proof log location.

### Step 3: Tighten Public Aggregate-Lane Helper Surface If Justified

Goal: narrow existing public declarations only when Step 1 proves they are
unnecessarily broad for current construction and printer consumers.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- Narrow call-site adjustments in `calls.cpp` or `machine_printer.cpp` only if
  required by the helper-surface contraction.

Actions:

- Remove, privatize, or combine public declarations only when all current
  consumers remain covered.
- Preserve the accepted and rejected record shapes for aggregate register-lane
  publication.
- Keep printer rejection checks for missing scratch, missing destination lane,
  missing printable source chunk, and missing chunk load mnemonic.
- Do not import stack-lane inline-asm publication or broad call-boundary record
  work into this step.

Completion check:

- Build proof and focused aggregate-lane backend tests pass.
- If public declarations move, matching `test_before.log` and `test_after.log`
  cover the affected tests.
- `todo.md` records the narrowed public surface and any declarations that
  intentionally remained public.

### Step 4: Prove Equivalence And Prepare Closure Evidence

Goal: make behavior equivalence explicit enough for supervisor review and
source-idea closure judgment.

Actions:

- Compare generated assembly output for register-sourced and
  frame-slot-sourced byval lane publication before and after the contraction
  when matching logs are required.
- Cover chunk-width load selection, destination-lane derivation, scratch
  exclusion, and printer rejection paths.
- Summarize any helper/table surfaces left unchanged and why they remain
  construction-owned, printer-owned, or out of scope.
- State whether broader AArch64 backend validation is needed before closure.

Completion check:

- `todo.md` contains a close-ready evidence summary with proof commands and
  log paths.
- The active diff does not change assembly output, diagnostics,
  unsupported-path contracts, or scratch selection.
