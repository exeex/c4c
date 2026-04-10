# HIR Dense ID Maps And Storage Cleanup

Status: Active
Source Idea: ideas/open/04_hir_dense_id_maps_and_storage_cleanup.md

## Purpose

Reduce boilerplate and sharpen invariants in HIR and frontend lowering code by
introducing dense-ID storage helpers for allocator-produced IDs.

## Goal

Land a small frontend-owned dense ID map abstraction and migrate the narrowest
high-value HIR/lowering call sites away from obvious `unordered_map<uint32_t, T>`
patterns.

## Core Rule

Keep this work scoped to frontend / lowering code and preserve behavior first;
do not turn the runbook into a repo-wide container rewrite.

## Read First

- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/inline_expand.*`
- HIR/lowering helpers that map `LocalId`, `BlockId`, `ExprId`, `GlobalId`, or
  `FunctionId` to metadata
- `prompts/EXECUTE_PLAN.md`

## Scope

- introduce `DenseIdMap<Id, T>` and `OptionalDenseIdMap<Id, T>` as frontend-
  owned helpers
- start with HIR and frontend lowering call sites where IDs are minted densely
  and consumed locally
- prefer wrappers first; representation changes can stay simple if that keeps
  the slice safe

## Non-Goals

- no repo-wide replacement of all `unordered_map`
- no backend allocator or register-allocation storage redesign
- no string-keyed or genuinely sparse data migration just for consistency
- no custom allocator work unless a migrated helper demonstrably requires it

## Working Model

- dense-ID helpers should encode the intended access pattern at the type level
- checked access should distinguish "must exist" from optional occupancy
- the first slice may remain STL-backed internally if that preserves safety and
  keeps the migration narrow
- each migrated caller should become simpler or more explicit than the raw-map
  version it replaces

## Execution Rules

- prefer one dense-ID helper slice at a time over broad mechanical rewrites
- add or update focused tests before widening migration scope
- keep helper APIs small and local to frontend/HIR needs
- if a newly discovered use site is not required for the current slice, record
  it in `todo.md` instead of absorbing it silently

## Ordered Steps

### Step 1: Inventory dense-ID lookup surfaces and choose the first migration

Goal: identify the narrowest high-value `unordered_map<uint32_t, T>`-style HIR
or lowering surfaces that already behave like dense ID tables.

Primary targets:

- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/inline_expand.*`
- nearby lowering helpers discovered by search

Actions:

- list the typed-ID keyed maps currently used in HIR and lowering code
- separate clearly dense allocator-produced surfaces from genuinely sparse ones
- pick the first coherent migration slice with good validation coverage

Completion check:

- the first helper landing and migration surface are explicitly bounded

### Step 2: Add frontend-owned dense ID map helpers

Goal: introduce minimal dense-ID helper types that encode checked and optional
occupancy access patterns without forcing a broad representation change.

Primary targets:

- `src/frontend/hir/hir_ir.hpp` or a nearby frontend-owned helper header

Actions:

- add `DenseIdMap<Id, T>` and `OptionalDenseIdMap<Id, T>` with small,
  intention-revealing APIs
- keep the implementation simple and behavior preserving
- document or encode the expected ID-to-index conversion pattern

Completion check:

- the project builds with the new helper types available to migrated call sites

### Step 3: Migrate the first dense-ID call-site slice

Goal: replace the chosen raw typed-ID map usage with the new helper APIs.

Primary targets:

- `src/frontend/hir/inline_expand.*`
- the specific lowering helper files selected in Step 1

Actions:

- update only the chosen call sites onto dense-ID helpers
- preserve semantics and simplify existence checks where the helper clarifies
  intent
- avoid widening into unrelated backend or sparse-map cleanup

Completion check:

- the targeted slice no longer depends on ad hoc raw typed-ID hash maps

### Step 4: Validate and record follow-on migrations

Goal: prove the helper-backed slice is behavior preserving and leave the next
candidate migrations clearly recorded.

Actions:

- add or update narrow HIR/lowering regressions for the migrated slice
- run targeted validation first, then broader regression checks as required by
  `prompts/EXECUTE_PLAN.md`
- record deferred dense-ID map candidates in `todo.md`

Completion check:

- targeted validation passes and the next dense-ID migration surface is named
