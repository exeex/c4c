# AArch64 Memory Fold-Back After Store Source Planning Runbook

Status: Active
Source Idea: ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md

## Purpose

Fold remaining AArch64-only memory store-source emission residue back into the
reference-style `memory` owner after the semantic store-source planning work has
moved selected source facts forward.

## Goal

Make `memory.cpp` / `memory.hpp` own the remaining target-local store-source
emission helpers, remove obsolete `memory_store_sources.*` helper files when
possible, and prove AArch64 memory/store-source behavior still consumes shared
prepared facts and fails closed when those facts are absent.

## Prerequisite

`ideas/closed/34_aarch64_store_source_publication_planning.md` is closed. That
closure moved the selected store-local cast producer source decision into shared
prepared store-source publication planning and left only AArch64 memory
emission residue in `memory_store_sources.*`.

## Core Rule

This is mechanical ownership cleanup only. Do not move target-neutral
store-source facts, rediscover semantic source choices in AArch64, weaken
fail-closed behavior for missing shared facts, or change broad memory semantics.

## Read First

- `ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md`
- `ideas/closed/34_aarch64_store_source_publication_planning.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/memory_store_sources.cpp`
- `src/backend/mir/aarch64/codegen/memory_store_sources.hpp`
- `src/backend/mir/aarch64/codegen/memory_dynamic_stack.cpp`
- AArch64 codegen build metadata that lists memory store-source helper sources
- Focused tests covering AArch64 memory/store-source publication and
  fail-closed missing prepared facts

## Current Scope

- Fold remaining target-local residue from:
  - `memory_store_sources.cpp`
  - `memory_store_sources.hpp`
- Use `memory.cpp` / `memory.hpp` as the AArch64 memory lowering owner.
- Keep only AArch64 instruction spelling, addressing, scratch, and store/load
  emission logic in this mechanical cleanup.
- Preserve consumption of shared prepared store-source facts.
- Update includes and build metadata only as required by the fold-back.
- Preserve fail-closed behavior for missing shared store-source facts.

## Non-Goals

- Do not move target-neutral store-source facts; that belonged to the closed
  semantic store-source publication planning idea.
- Do not add new AArch64-local source selection, same-block producer scanning,
  or semantic rediscovery.
- Do not rework `memory_dynamic_stack.*` unless narrow include fallout requires
  it.
- Do not change frame layout, stack slot allocation, broad memory operation
  semantics, or final assembly printing.
- Do not fold calls, dispatch, comparison, prologue, module compatibility, or
  return helpers.
- Do not weaken unsupported diagnostics, downgrade tests, or remove negative
  missing-fact coverage.

## Working Model

- Treat `memory_store_sources.*` as a post-semantic-planning residue file:
  remaining code should be AArch64 emission mechanics around prepared facts.
- Treat shared prepared store-source publication facts as the source authority.
- If a helper still performs semantic source choice locally, stop and report the
  dependency rather than folding it as mechanical cleanup.
- Prefer namespace-local helpers after fold-back when external visibility is
  not required.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Before moving code, inventory all declarations, definitions, includes, build
  metadata entries, call sites, and any helper that still appears semantic.
- Do not create a new memory helper family as a replacement owner.
- Preserve existing diagnostics and fail-closed behavior for absent or
  incomplete prepared store-source facts.
- Update `todo.md` with packet progress and proof after each executor packet.
- Use focused AArch64 memory/store-source proof first, then escalate to a
  backend bucket before closure when multiple include/build changes land.

## Ordered Steps

### Step 1: Inventory Memory Store-Source Residue Ownership

Goal: identify all declarations, definitions, includes, build entries, call
sites, and any semantic blockers in `memory_store_sources.*`.

Primary target: `memory_store_sources.*`, `memory.*`, `memory_dynamic_stack.*`,
AArch64 codegen build metadata, and focused memory/store-source tests.

Actions:
- Inspect `memory_store_sources.*` and classify each declaration/definition as
  memory-owner private, externally consumed, or blocked because it still owns
  semantic source choice.
- Find all includes of `memory_store_sources.hpp`.
- Locate direct callers of the store-source helper surface.
- Locate build metadata entries for `memory_store_sources.cpp`.
- Identify which declarations must remain public through `memory.hpp` and which
  can become namespace-local in `memory.cpp`.
- Record focused tests that prove prepared store-source consumption and
  fail-closed missing-fact behavior.

Completion check:
- `todo.md` records helper groups, include sites, call sites, build metadata
  entries, declarations that must remain public, semantic blockers if any, test
  handles, and a recommended first mechanical fold-back packet with no
  implementation edits.

### Step 2: Fold Store-Source Emission Into Memory Owner

Goal: move AArch64-only store-source emission helpers into
`memory.cpp` / `memory.hpp` without changing memory behavior.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp` and
`src/backend/mir/aarch64/codegen/memory.hpp`.

Actions:
- Move needed declarations from `memory_store_sources.hpp` into the memory owner
  API only when external callers still need them.
- Move implementation from `memory_store_sources.cpp` into `memory.cpp`,
  preferably namespace-local when only memory lowering uses it.
- Update include sites and build metadata after callers compile through the
  memory owner.
- Delete obsolete `memory_store_sources.cpp` / `.hpp` only after declarations,
  call sites, includes, and build metadata have been folded.
- Do not change prepared store-source fact consumption or missing-fact
  diagnostics.

Completion check:
- No live source file includes `memory_store_sources.hpp`, the obsolete
  translation unit is removed from build metadata, obsolete helper files are
  deleted when empty/obsolete, and focused memory/store-source tests still pass.

### Step 3: Clean Stale Memory Ownership References

Goal: remove stale live-owner references after store-source emission ownership
has moved.

Primary target: AArch64 codegen includes, build metadata, and owner comments or
header labels affected by the fold-back.

Actions:
- Search live source, test, and build paths for `memory_store_sources`.
- Remove stale includes, comments, and forward declarations that name obsolete
  owners.
- Keep historical or source-intent references untouched unless lifecycle work
  explicitly owns them.

Completion check:
- Live source/build/test paths no longer reference the removed helper files
  except where a deliberate historical or lifecycle artifact owns the wording.

### Step 4: Validate Memory Fold-Back

Goal: prove AArch64 memory/store-source behavior and linkage survived the
fold-back.

Primary target: focused AArch64 memory/store-source tests plus an appropriate
build or backend bucket.

Actions:
- Run a fresh build proof after build metadata updates.
- Run focused AArch64 memory/store-source tests that cover prepared
  store-source consumption and missing-fact fail-closed behavior.
- Include nearby memory operand/store-source tests when include or linkage blast
  radius touches shared memory helpers.
- Escalate to a backend bucket before closure when multiple narrow packets have
  landed or when the supervisor requests acceptance-grade confidence.
- Store canonical proof in `test_after.log` when delegated by the supervisor.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  diff is limited to memory store-source fold-back scope plus required
  build/include fallout.
