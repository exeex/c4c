# Backend StructDecl Layout Table Dual Path Runbook

Status: Active
Source Idea: ideas/open/113_backend_struct_decl_layout_table_dual_path.md

## Purpose

Introduce a BIR-facing structured aggregate layout table derived from
`LirModule::struct_decls`, while keeping legacy `module.type_decls` parsing as
a fallback and parity source.

## Goal

Make backend layout consumers prefer structured `StructNameId` /
`LirStructDecl` facts where parity can be checked, without changing emitted
backend output.

## Core Rule

Do not remove `module.type_decls` or downgrade behavior. Structured layout is
allowed to become the preferred path only when the legacy path remains
available and parity can be observed for the touched backend path.

## Read First

- `ideas/open/113_backend_struct_decl_layout_table_dual_path.md`
- Current LIR module declarations and `LirStructDecl` definitions.
- BIR/backend aggregate layout helpers that currently parse
  `module.type_decls`.

## Current Targets

- BIR aggregate addressing and memory layout helpers.
- Helpers that resolve type declarations from `module.type_decls`.
- Aggregate size/alignment calculation.
- HFA, direct-GP, and sret aggregate classification.
- Global initializer and aggregate store/load lowering where backend-owned
  layout is required.

## Non-Goals

- Do not migrate or clean up `src/backend/mir/`.
- Do not rework MIR/aarch64 aggregate layout helpers before the planned MIR
  rewrite.
- Do not preserve MIR `.cpp` files in the compile target when they are the only
  blocker for this route; treat that as a compile-target exclusion problem.
- Do not remove rendered type text or legacy type declarations as part of this
  runbook.

## Working Model

- Build a backend-owned structured layout table from `LirModule::struct_decls`.
- Keep legacy parsing from `module.type_decls` as fallback and as the parity
  oracle.
- Use structured facts first only inside backend paths where fallback and
  mismatch reporting are wired.
- Preserve exact emitted LLVM/backend text while changing internal lookup
  authority.

## Execution Rules

- Prefer small code slices that can be compiled and narrowly proven.
- Add or reuse a shared parity check instead of writing one-off testcase-shaped
  checks.
- Record structured-vs-legacy layout mismatches for touched paths.
- Keep fallback behavior explicit when structured data is missing or incomplete.
- Escalate validation when a step touches classification or aggregate memory
  lowering shared by multiple features.

## Ordered Steps

### Step 1: Inventory Backend Layout Authority

Goal: identify the BIR/backend consumers that still rely on
`module.type_decls` parsing for aggregate layout decisions.

Primary target: backend layout lookup, size/alignment, aggregate addressing,
classification, global initializer, and aggregate load/store surfaces.

Actions:
- Inspect backend code paths that resolve aggregate declarations from rendered
  type text.
- Group consumers by behavior: lookup-only, size/alignment, ABI
  classification, initializer lowering, memory addressing, and load/store.
- Confirm which paths are BIR/backend-owned and which are MIR-only.
- Record any MIR-only compile blockers as target-exclusion candidates rather
  than migration work.

Completion check:
- The executor can name the first concrete backend-owned consumer to convert
  and the fallback/parity surface it will use.
- `todo.md` records the inventory result and the chosen first conversion
  packet.

### Step 2: Introduce Structured Backend Layout Table

Goal: add the shared BIR-facing table that maps structured struct identifiers
to aggregate layout facts sourced from `LirModule::struct_decls`.

Primary target: a backend-owned helper/module that can be reused by aggregate
layout consumers.

Actions:
- Derive structured layout entries from `LirStructDecl` data available in the
  LIR module.
- Keep legacy `module.type_decls` parsing available in the same lookup flow or
  as an adjacent fallback.
- Define a mismatch-reporting path for structured-vs-legacy layout facts.
- Keep construction behavior-preserving when structured declarations are
  absent.

Completion check:
- The project builds for the touched backend target.
- A narrow proof shows the table can be constructed without changing emitted
  output.
- `todo.md` records the exact proof command and any fallback gaps.

### Step 3: Convert Aggregate Size And Addressing Consumers

Goal: make backend aggregate size/alignment and memory-addressing helpers use
structured layout facts first where parity exists.

Primary target: BIR aggregate layout helpers and aggregate addressing paths.

Actions:
- Route size/alignment lookup through the structured table.
- Keep legacy parsing as fallback and parity source.
- Record parity mismatches without changing successful legacy behavior.
- Prove nearby aggregate addressing cases, not only one narrow testcase.

Completion check:
- Focused aggregate layout and memory-addressing proof passes without output
  drift.
- Fallback behavior remains covered for missing structured facts.

### Step 4: Convert ABI Classification Consumers

Goal: make HFA, direct-GP, sret, and variadic aggregate classification use
structured layout facts first where safe.

Primary target: backend aggregate classification helpers.

Actions:
- Identify classification decisions that require aggregate field layout.
- Route those decisions through the structured table with legacy fallback.
- Preserve ABI-visible emitted output.
- Add or update parity proof for classification paths touched by the change.

Completion check:
- Focused HFA, sret, direct-GP, and variadic aggregate proof passes without
  expectation downgrades.
- The executor records any consumers that remain legacy-only and why.

### Step 5: Convert Initializer And Aggregate Load/Store Consumers

Goal: make backend-owned global initializer and aggregate load/store lowering
prefer structured layout facts where parity is stable.

Primary target: global initializer lowering and aggregate store/load lowering
that require backend-owned layout.

Actions:
- Route backend-owned initializer and aggregate memory lowering through the
  structured table.
- Keep legacy fallback for incomplete coverage.
- Record mismatches and unresolved fallback-only paths.
- Avoid changing HIR-to-LIR layout authority in this step; that belongs to a
  later idea.

Completion check:
- Focused global-init and aggregate load/store proof passes without output
  drift.
- Remaining legacy-only paths are documented in `todo.md`.

### Step 6: Broader Parity And Regression Checkpoint

Goal: prove the dual-path structured backend layout route is stable enough for
later type-text demotion ideas.

Primary target: aggregate, HFA, sret, variadic, global-init, and memory
addressing proof set selected by the supervisor.

Actions:
- Run the supervisor-selected broader proof after the converted backend paths
  are complete.
- Compare output-sensitive tests for drift.
- Confirm no change downgraded expectations or added testcase-shaped matching.
- Summarize remaining fallback-only backend paths in `todo.md`.

Completion check:
- Fresh build and broader proof pass.
- `todo.md` clearly states whether idea 113 acceptance criteria are satisfied
  or what blocker remains.
