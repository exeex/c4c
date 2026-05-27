# AArch64 Duplicate Prepared Authority Audit Runbook

Status: Active
Source Idea: ideas/open/46_aarch64_duplicate_prepared_authority_audit.md

## Purpose

Audit seven large AArch64 codegen files for duplicated BIR/prealloc/shared
prepared authority, then split the findings into one durable follow-up idea per
file.

## Goal

Produce a file-by-file authority audit that distinguishes shared prepared facts
from target-local emission logic, then create seven numbered repair ideas with
owned scope and reject signals.

## Core Rule

This plan is audit-only. Do not edit implementation files, tests, build
metadata, or expectations while executing this runbook.

## Read First

- `ideas/open/46_aarch64_duplicate_prepared_authority_audit.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `ref/claudes-c-compiler/src/backend/arm/codegen/emit.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/calls.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/memory.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/alu.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/comparison.rs`

## Current Scope

- Audit exactly these AArch64 files:
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/alu.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/comparison.cpp`
- Compare each file against existing BIR, prealloc, and shared prepared facts.
- Classify suspicious helpers as `consume-shared`, `missing-shared-field`,
  `target-emission`, `legacy-fallback`, or `needs-more-evidence`.
- Create exactly seven follow-up ideas under `ideas/open/`, one per audited
  file, after the audit table has enough evidence.

## Non-Goals

- Do not directly repair the audited implementation files.
- Do not add or weaken tests.
- Do not use line-count reduction as evidence of progress.
- Do not reopen closed fold-back cleanup without a concrete duplicate-authority
  finding.
- Do not claim that all producer lookups are wrong without separating semantic
  authority from target-local hazard or emission logic.

## Working Model

- Shared BIR/prealloc/prepared records own semantic source, home, publication,
  edge-copy, block-entry, call-plan, and producer-index authority.
- AArch64 owns concrete register/scratch/addressing choices, instruction
  spelling, hazard handling, and emission sequencing.
- Duplicate prepared authority exists when AArch64 rederives a semantic fact
  that is already available through shared records or should be added there.
- Target-emission residue is valid when the file only performs AArch64-specific
  materialization, scratch management, addressing, or instruction selection
  after shared authority has chosen the semantic contract.

## Execution Rules

- Keep audit evidence concrete: name helpers, local scans, fallback paths,
  shared records, and missing shared queries.
- Prefer direct code references over broad descriptions such as "move to BIR".
- Compare nearby ARM reference responsibilities before classifying an AArch64
  helper as duplicate authority.
- Record uncertainty as `needs-more-evidence` with the exact proof needed
  rather than forcing a repair route.
- When creating follow-up ideas, include `## Reviewer Reject Signals` and make
  each idea own exactly one audited file family.
- Lifecycle edits to `ideas/open/` are allowed only for the durable audit table
  and the seven follow-up idea files required by the source idea.

## Steps

### Step 1: Inventory Shared Authority And Reference Responsibilities

Goal: build the authority baseline used by every file audit.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `ref/claudes-c-compiler/src/backend/arm/codegen/emit.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/calls.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/memory.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/alu.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/comparison.rs`

Actions:

- List the shared prepared facts and lookup helpers relevant to edge
  publication, source/home recovery, block-entry publication, call plans, value
  homes, and producer indexes.
- Summarize what the ARM reference keeps target-local for emit, calls, memory,
  alu, and comparison.
- Define the evidence columns for the seven-file audit table.

Completion check:

- `todo.md` records the authority baseline and audit table column contract,
  with no implementation files changed.

### Step 2: Audit Edge Copy And Publication Files

Goal: classify duplicate authority candidates in the AArch64 edge-copy and
publication dispatch files.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

Actions:

- Identify local producer scans, source/home recovery, publication planning,
  prepared block-entry publication handling, and edge-copy fact recovery.
- For each suspicious helper or fallback, name the existing shared fact to
  consume, the missing shared field/query, or why it remains target emission.
- Compare responsibilities against the ARM reference `emit.rs` shape.

Completion check:

- The audit table has evidence-backed rows for both dispatch files and assigns
  each candidate one of the required classifications.

### Step 3: Audit Value Materialization And Memory

Goal: classify duplicate authority candidates in value materialization and
memory lowering.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Identify local recovery of value homes, store-source plans, load/store source
  decisions, same-block fallbacks, and address/source derivation.
- Separate target-local scratch/addressing decisions from shared authority
  that should select semantic source and destination contracts.
- Compare memory responsibilities against ARM `memory.rs`.

Completion check:

- The audit table has evidence-backed rows for value materialization and
  memory, including any `needs-more-evidence` proof questions.

### Step 4: Audit ALU, Calls, And Comparison

Goal: classify duplicate authority candidates in arithmetic, call, and compare
lowering.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Identify call source decisions, producer-index recovery, value-home fallback,
  same-block scans, and comparison/ALU source reconstruction.
- Separate target opcode/register spelling from semantic authority that belongs
  in BIR/prealloc/shared prepared facts.
- Compare responsibilities against ARM `alu.rs`, `calls.rs`, and
  `comparison.rs`.

Completion check:

- The audit table has evidence-backed rows for all seven audited files.

### Step 5: Write Durable Audit Findings

Goal: preserve the audit in the source idea before splitting follow-up repair
work.

Primary target:

- `ideas/open/46_aarch64_duplicate_prepared_authority_audit.md`

Actions:

- Add a compact audit table covering all seven files.
- For each file, state which duplicated authority appears present, which facts
  are already shared, which shared authority is missing, and which residue is
  mostly target emission.
- Keep the source idea update factual and compact; do not rewrite its original
  intent.

Completion check:

- The source idea contains a durable audit table that can be reviewed without
  reading transient `todo.md` notes.

### Step 6: Create Seven Follow-Up Ideas

Goal: split repair work into one numbered open idea per audited file.

Primary target:

- `ideas/open/`

Actions:

- Create exactly seven follow-up idea files, one for each audited AArch64 file
  family.
- In each idea, name the owned file, the duplicated helper or fallback path,
  the shared facts to consume or add, the out-of-scope target-emission
  behavior, acceptance criteria, and concrete reviewer reject signals.
- Keep each idea repair-scoped; do not combine multiple audited files into one
  follow-up.

Completion check:

- Seven new numbered ideas exist under `ideas/open/`, each with one-file
  ownership and a concrete `## Reviewer Reject Signals` section.

### Step 7: Close Or Hand Off The Audit

Goal: decide whether this audit idea is complete after the durable findings and
follow-up ideas exist.

Primary targets:

- `ideas/open/46_aarch64_duplicate_prepared_authority_audit.md`
- `plan.md`
- `todo.md`

Actions:

- Verify that the audit table covers all seven files and that seven follow-up
  ideas exist.
- Confirm no implementation, test, build metadata, or expectation files were
  changed.
- Ask the plan owner close flow to decide closure only after lifecycle
  acceptance criteria are satisfied.

Completion check:

- The supervisor has enough evidence to delegate close review, with `todo.md`
  recording the audit-only proof and any remaining ambiguity.
