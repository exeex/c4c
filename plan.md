# AArch64 Codegen Layout Classification Runbook

Status: Active
Source Idea: ideas/open/20_aarch64_codegen_layout_classification.md

## Purpose

Classify extra AArch64 codegen translation units against the reference ARM
codegen layout, then create coherent follow-up ideas for semantic authority
migration and mechanical target-local consolidation.

## Goal

Produce a durable classification map and follow-up idea set without moving,
merging, or rewriting implementation files.

## Core Rule

This is classification work only. Do not edit AArch64 codegen implementation,
build metadata, tests, or reference files while executing this runbook.

## Read First

- `ideas/open/20_aarch64_codegen_layout_classification.md`
- `ideas/closed/16_bir_edge_value_flow_authority.md`
- `ideas/closed/17_aarch64_absent_selection_fallback_retirement.md`
- `ideas/closed/18_aarch64_cts_00181_runtime_regression_reopen.md`
- `ideas/closed/19_x86_riscv_prepared_edge_publication_handoff.md`
- `src/backend/mir/aarch64/codegen/`
- `ref/claudes-c-compiler/src/backend/arm/codegen/`

## Current Targets

- Extra AArch64 codegen files or helper families without a clear reference ARM
  owner counterpart.
- Known families called out by the source idea: `dispatch*`, `calls*`,
  `memory*`, publication, materialization, and bridge files.
- Closure-note evidence from ideas 16 through 19 that points to move-forward
  semantic authority debt or target-local cleanup debt.

## Non-Goals

- Do not move or merge implementation files.
- Do not rewrite AArch64 dispatch, calls, memory, printer, or build behavior.
- Do not treat file-count or line-count reduction as success.
- Do not create a single broad cleanup idea that mixes shared semantic
  migration with mechanical AArch64 file consolidation.
- Do not change tests, expectations, or root proof logs.

## Working Model

Classify every extra file or coherent helper family into exactly one bucket:

- `move-forward`: semantic authority belongs in BIR or shared prepare.
- `fold-back`: target-local helper logic should merge into a matching
  reference-style owner file such as `calls`, `memory`, `comparison`,
  `prologue`, or `returns`.
- `keep-local`: AArch64-specific emission or ABI detail justifies a separate
  local owner.
- `needs-more-evidence`: a small audit is required before cleanup can be
  planned safely.

## Execution Rules

- Keep routine audit notes in `todo.md` until a durable artifact or follow-up
  idea is ready.
- When creating follow-up ideas under `ideas/open/`, keep each idea coherent
  and scoped to one cleanup family.
- Every follow-up idea must include concrete owned file families and reviewer
  reject signals.
- Separate semantic authority migration from mechanical file consolidation.
- Preserve the source idea unless durable lifecycle metadata is required.
- Use repo-native file inventory commands; do not rely on stale file lists.

## Steps

### Step 1: Inventory Extra AArch64 Codegen Files

Goal: Build the current file-family inventory and identify files without a
clear reference ARM owner counterpart.

Primary targets:

- `src/backend/mir/aarch64/codegen/`
- `ref/claudes-c-compiler/src/backend/arm/codegen/`

Actions:

- List AArch64 codegen `.cpp` and `.hpp` files and group them by owner family.
- List reference ARM codegen files and derive the reference owner names.
- Mark direct counterparts first, then isolate extra AArch64-only files and
  helper families.
- Record the inventory table in `todo.md` or a temporary working note until it
  is ready to become durable follow-up content.

Completion check:

- Every AArch64 codegen file is either mapped to a reference owner family or
  listed as extra / AArch64-only for classification.

### Step 2: Classify Dispatch And Publication Families

Goal: Classify dispatch, edge-copy, publication, producer, lookup, diagnostic,
and prepared-value materialization files using closure evidence from ideas 16
through 19.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch*`
- `src/backend/mir/aarch64/codegen/prepared_value_home_materialization.*`
- `src/backend/mir/aarch64/codegen/compatibility_projection.*`
- `ideas/closed/16_bir_edge_value_flow_authority.md`
- `ideas/closed/18_aarch64_cts_00181_runtime_regression_reopen.md`
- `ideas/closed/19_x86_riscv_prepared_edge_publication_handoff.md`

Actions:

- Separate target-local emission and diagnostics from semantic edge/publication
  authority.
- Mark remaining semantic producer lookup, edge value-flow, or prepared fact
  authority as `move-forward` only when the closure evidence supports it.
- Mark pure AArch64 emission, hazard handling, diagnostics, or instruction
  spelling as `keep-local` or `fold-back` into `dispatch` as appropriate.
- Record unresolved areas as `needs-more-evidence` with the exact evidence
  needed.

Completion check:

- Every extra dispatch/publication/materialization helper family has a bucket,
  an owner rationale, and a proposed follow-up direction.

### Step 3: Classify Calls, Memory, Comparison, Prologue, And Return Helpers

Goal: Classify non-dispatch helper families against reference-style owners.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls*`
- `src/backend/mir/aarch64/codegen/memory*`
- `src/backend/mir/aarch64/codegen/comparison_branch_fusion.*`
- `src/backend/mir/aarch64/codegen/prologue_entry_formals.cpp`
- `src/backend/mir/aarch64/codegen/returns.*`
- `ideas/closed/17_aarch64_absent_selection_fallback_retirement.md`

Actions:

- Decide whether each extra helper is target-local ABI/emission logic that
  should fold back into a reference-style owner or stay as a justified local
  owner.
- Use idea 17 closure notes to avoid confusing retired source-selection
  fallback debt with mechanical calls-file consolidation.
- Identify any semantic authority that still belongs in shared prepare instead
  of AArch64 calls or memory codegen.

Completion check:

- Calls, memory, comparison, prologue, and returns helper families have bucket
  assignments with concrete owner-file recommendations.

### Step 4: Build The Durable Classification Table

Goal: Convert the audit into a stable classification artifact suitable for
closing the source idea.

Primary target:

- `todo.md` first; source idea only if the supervisor asks for durable idea
  updates before close.

Actions:

- Normalize the classification into a table with columns for file/family,
  bucket, proposed owner, rationale, and follow-up idea target.
- Check that every extra `dispatch*`, `calls*`, `memory*`, publication,
  materialization, or bridge file appears in the table.
- Keep the table narrow and factual; do not describe implementation changes as
  already performed.

Completion check:

- The classification table is complete and can be reviewed without rereading
  all implementation files.

### Step 5: Create Numbered Follow-Up Ideas

Goal: Split the classified cleanup work into coherent open ideas.

Primary target:

- `ideas/open/`

Actions:

- Create one or more numbered follow-up ideas for semantic authority migration
  work marked `move-forward`.
- Create separate numbered follow-up ideas for mechanical AArch64 fold-back
  cleanup work.
- Keep `keep-local` decisions in the classification table rather than turning
  them into cleanup ideas.
- Turn `needs-more-evidence` entries into small audit ideas only when the
  missing evidence cannot be resolved inside this runbook.
- Include reviewer reject signals in every new idea.

Completion check:

- Follow-up ideas exist for each coherent cleanup slice, and no follow-up idea
  mixes semantic authority migration with mechanical file consolidation.

### Step 6: Close-Readiness Review

Goal: Prepare the active source idea for supervisor review and possible close.

Actions:

- Verify no implementation files, tests, build metadata, or root proof logs
  were changed.
- Verify `todo.md` records the final classification summary and the follow-up
  idea paths.
- Verify the final note explains which extra files are expected to disappear,
  which should merge into reference-style owners, and which are intentionally
  retained.

Completion check:

- The source idea's acceptance criteria are satisfied or the remaining blocker
  is explicit enough for lifecycle review.
