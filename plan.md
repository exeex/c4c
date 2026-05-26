# AArch64 Memory Store-Source Fold-Back Runbook

Status: Active
Source Idea: ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md

## Purpose

Fold the remaining AArch64-only store-source emission helpers back into the
memory owner now that semantic store-source residue has been removed or
target-neutralized by the closed 39a prerequisite.

## Goal

Remove the standalone `memory_store_sources.*` helper family through a
mechanical ownership cleanup while preserving AArch64 memory behavior,
diagnostics, fail-closed behavior, and build/test expectations.

## Core Rule

This is a mechanical fold-back. Do not reintroduce AArch64-local semantic
source rediscovery. AArch64 memory lowering should continue consuming shared
prepared store-source facts or fail closed when authority is absent.

## Read First

- `ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md`
- `ideas/closed/39a_aarch64_store_source_semantic_residue_prerequisite.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/memory_store_sources.cpp`
- `src/backend/mir/aarch64/codegen/memory_store_sources.hpp`
- `src/backend/CMakeLists.txt`

## Current Scope

- Fold remaining target-local helpers from:
  - `src/backend/mir/aarch64/codegen/memory_store_sources.cpp`
  - `src/backend/mir/aarch64/codegen/memory_store_sources.hpp`
- Make `memory.cpp` / `memory.hpp` the AArch64 memory lowering owner for this
  helper surface.
- Update include sites and build metadata only as required by deleting the
  standalone helper files.
- Preserve behavior for prepared store-source facts, pointer-base publication,
  store-local publication, store-global publication, and stack writeback.

## Non-Goals

- Do not move target-neutral store-source facts; 39a completed that
  prerequisite.
- Do not edit `memory_dynamic_stack.*` unless compile-only include fallout
  requires it.
- Do not change frame layout, stack slot allocation, broad memory semantics,
  final assembly printing, or diagnostics.
- Do not fold calls, dispatch, comparison, prologue, module compatibility, or
  return helpers.
- Do not weaken tests, diagnostics, or unsupported behavior.

## Working Model

- Treat `memory_store_sources.*` as target-local emission residue.
- Keep shared prepared planning as the source of semantic store-source facts.
- Move helpers into `memory.cpp` without changing signatures unless the
  deletion requires declarations to live in `memory.hpp`.
- Keep each packet mechanical and prove it with focused AArch64 memory and
  backend store-source tests.

## Execution Rules

- Start with a refreshed inventory after 39a closure because the blocker state
  changed.
- Prefer moving a coherent helper group at a time instead of rewriting memory
  lowering.
- Delete `memory_store_sources.cpp/.hpp` only after all live declarations and
  include sites have moved.
- Remove the translation unit from `src/backend/CMakeLists.txt` only when the
  implementation has fully moved.
- Run focused proof after code packets and a backend bucket before closure.

## Ordered Steps

### Step 1: Refresh Mechanical Store-Source Inventory

Goal: confirm the post-39a `memory_store_sources.*` surface is mechanical and
identify exact declarations, include sites, and build metadata to fold.

Primary target: `memory_store_sources.*`, `memory.cpp/.hpp`, include sites, and
`src/backend/CMakeLists.txt`.

Actions:
- Inventory remaining functions and declarations in `memory_store_sources.*`.
- Identify external include/call sites that must move to `memory.hpp` or become
  namespace-local in `memory.cpp`.
- Confirm no AArch64-local semantic source-choice recovery remains.
- Record a recommended first mechanical fold-back packet and focused proof.

Completion check:
- `todo.md` records helper groups, include/call sites, build metadata entries,
  declarations that must remain public, and the first mechanical fold-back
  packet.

### Step 2: Fold Private Store-Source Helpers Into Memory Owner

Goal: move helpers used only by memory lowering into `memory.cpp`.

Primary target: private helper groups from `memory_store_sources.cpp`.

Actions:
- Move private helpers into namespace-local scope in `memory.cpp`.
- Keep target instruction emission behavior unchanged.
- Leave public declarations only for helpers still consumed by other AArch64
  codegen files.
- Update includes only for compile fallout.

Completion check:
- Private store-source helper definitions are owned by `memory.cpp`; focused
  AArch64 memory/store-source proof passes.

### Step 3: Fold Remaining Public Store-Source Surface

Goal: eliminate the standalone `memory_store_sources.hpp` public surface.

Primary target: declarations still included outside `memory.cpp`.

Actions:
- Move required declarations to `memory.hpp` if they remain externally needed.
- Replace includes of `memory_store_sources.hpp` with `memory.hpp` where needed.
- Keep helper signatures stable unless a declaration is no longer externally
  required.

Completion check:
- No live source includes `memory_store_sources.hpp`; focused AArch64
  memory/dispatch proof passes.

### Step 4: Delete Obsolete Store-Source Translation Unit

Goal: remove obsolete files and build metadata once all definitions are folded.

Primary target: `memory_store_sources.cpp/.hpp` and `src/backend/CMakeLists.txt`.

Actions:
- Delete `memory_store_sources.cpp` and `memory_store_sources.hpp`.
- Remove `memory_store_sources.cpp` from `src/backend/CMakeLists.txt`.
- Verify no source/build/test path references the deleted helper files except
  lifecycle/source-intent text.

Completion check:
- Build metadata no longer references the deleted translation unit; focused
  AArch64 memory/store-source proof passes.

### Step 5: Validate Mechanical Fold-Back Completion

Goal: prove the memory fold-back is complete and closure-ready.

Primary target: focused AArch64 memory/store-source tests plus backend bucket.

Actions:
- Run a fresh build.
- Run focused AArch64 memory/store-source and dispatch tests after the final
  fold.
- Run a backend bucket before lifecycle closure.
- Search live source/build/test paths for `memory_store_sources` references.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  source idea can close without remaining standalone store-source helper files.
