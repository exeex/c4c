# AArch64 Call-Boundary Record And Printer Surface Audit Runbook

Status: Active
Source Idea: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md

## Purpose

Audit call-boundary and aggregate-lane surfaces that cross AArch64 call
lowering, instruction records, and machine-printer output, then decide whether
any concrete follow-up implementation idea is justified.

## Goal

Trace call-boundary move records and aggregate-lane publication records from
construction through printer output, classify duplicated validation or spelling
surfaces, and create narrow follow-up ideas only when the evidence supports
them.

## Core Rule

This plan is audit-only. Do not edit implementation files while executing this
runbook; create lifecycle follow-up ideas only for concrete, narrow candidates
with traced record paths and proof expectations.

## Read First

- `ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

## Current Targets

- Call-boundary construction:
  `src/backend/mir/aarch64/codegen/calls.cpp`
- Call-boundary public surface:
  `src/backend/mir/aarch64/codegen/calls.hpp`
- Instruction record schemas and helpers:
  `src/backend/mir/aarch64/codegen/instruction.cpp`
  `src/backend/mir/aarch64/codegen/instruction.hpp`
- Machine-printer spelling and validation:
  `src/backend/mir/aarch64/codegen/machine_printer.cpp`
  `src/backend/mir/aarch64/codegen/machine_printer.hpp`

## Non-Goals

- Do not continue broad instruction/printer table-driving from idea 82.
- Do not move AArch64 ABI-specific call-boundary construction into shared BIR,
  prealloc, or target-neutral authority.
- Do not refactor all of `calls.cpp`.
- Do not change assembly output, diagnostics, or unsupported-path contracts.
- Do not create an implementation idea without a traced record path and a
  focused proof set.

## Working Model

Classify each audited surface into one of:

- `target-local-abi-printer-ownership`
- `local-table-or-helper-contraction-candidate`
- `call-boundary-record-schema-cleanup-candidate`
- `aggregate-lane-record-surface`
- `missing-evidence`

Repeated validation or spelling should remain local when it protects AArch64
ABI decisions, diagnostics, scratch choices, or printer-owned spelling.
Candidate cleanup must name the exact record path and owner boundary.

## Execution Rules

- Keep audit notes in `todo.md` until they are durable enough to become a
  follow-up source idea.
- Prefer traced record paths over line-count or helper-name arguments.
- Treat uncertainty as `missing-evidence`; do not convert it into
  implementation scope.
- Reject any route that deletes validation or moves ABI ownership just to
  reduce duplication.
- If implementation files are accidentally touched, stop and return the dirty
  state to the supervisor; audit-only proof no longer applies.

## Steps

### Step 1: Inventory Call-Boundary Record Surfaces

Goal: Map how call-boundary move records are built, represented, and printed.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Actions:

- Trace call argument, return, scratch, and ABI boundary move construction in
  `calls.cpp`.
- Identify the instruction record fields and helper APIs that carry those
  moves through `instruction.*`.
- Identify the printer functions that spell or validate those records.
- Record each surface with a one-line responsibility summary.

Completion check:

- `todo.md` contains a call-boundary record inventory from construction to
  printer output, including the owning file and responsibility of each surface.

### Step 2: Inventory Aggregate-Lane Publication Surfaces

Goal: Map aggregate-lane publication records and printer output without
assuming they share the same cleanup boundary as call-boundary moves.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- Trace aggregate-lane publication construction and record emission.
- Identify instruction schema fields used by aggregate-lane records.
- Identify printer spelling and validation paths for aggregate-lane records.
- Record where aggregate-lane behavior is genuinely parallel to
  call-boundary moves and where it has distinct ownership.

Completion check:

- `todo.md` contains an aggregate-lane inventory with explicit notes on shared
  versus distinct call-boundary ownership.

### Step 3: Classify Duplicate Validation And Spelling Surfaces

Goal: Decide which repeated surfaces are justified ownership and which are
  concrete cleanup candidates.

Actions:

- Classify each repeated validation, spelling, schema, or helper surface using
  the working-model categories.
- Preserve surfaces that own AArch64 ABI, diagnostics, scratch choices, record
  construction, or machine-printer spelling.
- Mark only concrete contraction or schema-cleanup candidates that have a
  stable owner boundary and no semantics change.
- Mark unclear surfaces as `missing-evidence`.

Completion check:

- `todo.md` names each candidate or non-candidate, its classification, and the
  evidence supporting that classification.

### Step 4: Create Follow-Up Ideas Only For Concrete Candidates

Goal: Convert proven audit findings into narrow source ideas, not broad
calls/printer rewrites.

Actions:

- Create a new `ideas/open/*.md` implementation idea only when a candidate has
  a traced record path, bounded owner boundary, and focused proof route.
- Include goal, why, scope, non-goals, acceptance or proof expectations, and
  concrete reviewer reject signals in each generated idea.
- Do not create follow-up ideas for justified target-local ownership or vague
  missing-evidence areas.

Completion check:

- New follow-up ideas exist only for concrete candidates, and `todo.md` names
  every created idea or states why none were justified.

### Step 5: Prepare Audit Close Summary

Goal: Make the audit outcome clear enough for the supervisor to close,
deactivate, or route follow-up work.

Actions:

- Summarize call-boundary record surfaces and their ownership.
- Summarize aggregate-lane record surfaces and their ownership.
- Summarize concrete follow-up ideas created, if any.
- Summarize rejected candidates and the evidence that blocked them.
- State that no backend tests were required if no implementation files changed.

Completion check:

- `todo.md` contains a close-ready audit summary satisfying the source idea's
  proof expectations.
