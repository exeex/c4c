# AArch64 Calls Fold-Back Cleanup Runbook

Status: Active
Source Idea: ideas/open/35_aarch64_calls_foldback_cleanup.md

## Purpose

Fold target-local AArch64 calls helper files back into the `calls.cpp` owner
family while preserving existing call ABI behavior and shared call-preparation
authority.

## Goal

Make the AArch64 calls owner family own the listed call-boundary helper
implementation, remove the extra helper translation units from build metadata,
and prove behavior with focused call-lowering validation plus a backend build or
test bucket.

## Core Rule

This is mechanical ownership cleanup only. Do not change call ABI semantics,
prepared call planning, byval source selection, preservation publication,
return publication, register assignment, stack layout, or diagnostics.

## Read First

- `ideas/open/35_aarch64_calls_foldback_cleanup.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- AArch64 codegen build metadata that lists the helper translation units

## Current Scope

- Fold these target-local helper families into the AArch64 calls owner:
  - `calls_byval_aggregates.cpp`
  - `calls_common.cpp`
  - `calls_dispatch_bridge.cpp`
  - `calls_dispatch_bridge.hpp`
  - `calls_moves.cpp`
- Preserve `calls.cpp` and `calls.hpp` as the AArch64 calls and ABI lowering
  entry points.
- Update includes and build metadata only as required by the fold-back.
- Keep existing call-boundary moves, byval aggregate handling, dynamic-stack
  helper calls, call result recording, and diagnostics behavior intact.

## Non-Goals

- Do not change call ABI semantics, register assignment, stack-argument layout,
  or prepared call planning.
- Do not move shared call preparation into AArch64 codegen.
- Do not reopen byval source-selection, prepared preservation, or return
  publication authority.
- Do not fold unrelated `returns`, `prologue`, `memory`, comparison, branch, or
  dispatch cleanup into this plan.
- Do not replace capability work with expectation downgrades, unsupported
  contract weakening, or named-fixture special cases.

## Working Model

- Treat the helper files as target-local consumers of already-prepared facts.
- Move declarations into the calls owner API only when another AArch64 owner
  legitimately needs to call them after the fold-back.
- Prefer `namespace`-local helpers inside `calls.cpp` for implementation that
  does not need external visibility.
- Remove helper files only after their declarations, call sites, includes, and
  build entries have been folded into the calls owner.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Before editing a helper family, inspect its callers and included headers so
  visibility changes stay minimal.
- Update `todo.md` with packet progress and proof results after each executor
  packet.
- Use focused proof first, then escalate to a backend build or test bucket
  before accepting the fold-back as complete.
- Reject any route that special-cases a function name, value id, aggregate
  shape, ABI register, stack offset, or one narrow fixture.

## Ordered Steps

### Step 1: Inventory Calls Helper Ownership

Goal: identify all helper declarations, definitions, build entries, includes,
and call sites affected by the fold-back.

Primary target: the listed `calls_*` helper files and AArch64 codegen build
metadata.

Actions:
- Inspect each listed helper file and classify its definitions as private to
  calls lowering or externally consumed by another AArch64 owner.
- Find all includes of `calls_dispatch_bridge.hpp` and any declarations coming
  from `calls_common.cpp`, `calls_byval_aggregates.cpp`, or `calls_moves.cpp`.
- Locate build metadata entries for each helper translation unit.
- Record any helper that cannot be made private without a narrow calls-owner API
  addition.

Completion check:
- `todo.md` names the helper groups, external call sites, build entries, and a
  recommended first fold-back packet with no implementation edits yet.

### Step 2: Fold Private Calls Helpers Into `calls.cpp`

Goal: move private implementation from target-local helper files into the calls
owner without changing behavior.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`.

Actions:
- Move calls-only helper definitions from `calls_common.cpp`,
  `calls_byval_aggregates.cpp`, and `calls_moves.cpp` into `calls.cpp`.
- Keep implementation that does not require external linkage in an anonymous
  namespace or existing private calls-owner structure.
- Preserve diagnostic strings, fail-closed unsupported paths, and operand/move
  ordering.
- Remove obsolete internal declarations and includes after call sites compile
  through the calls owner.

Completion check:
- The folded implementation compiles through `calls.cpp`, and no removed helper
  file is still required for private calls lowering behavior.

### Step 3: Fold Dispatch Bridge Surface Into Calls Owner API

Goal: eliminate the separate calls dispatch bridge owner while preserving any
legitimate AArch64 dispatch-to-calls entry point.

Primary target: `src/backend/mir/aarch64/codegen/calls.hpp` and
`src/backend/mir/aarch64/codegen/calls.cpp`.

Actions:
- Move necessary bridge declarations from `calls_dispatch_bridge.hpp` into the
  narrow calls owner API.
- Move bridge implementation from `calls_dispatch_bridge.cpp` into `calls.cpp`
  or another calls-family location justified by existing ownership.
- Update dispatch or other AArch64 call sites to include the calls owner header
  instead of the bridge header.
- Do not use this step to rewrite dispatch publication or prepared edge-copy
  semantics.

Completion check:
- No source file includes `calls_dispatch_bridge.hpp`, the required bridge
  behavior is reachable through the calls owner API, and AArch64 dispatch/calls
  behavior remains unchanged.

### Step 4: Remove Folded Helper Files From Build Metadata

Goal: remove obsolete helper translation units and headers after ownership has
been folded into the calls family.

Primary target: AArch64 codegen build metadata and the listed helper files.

Actions:
- Delete the folded helper implementation/header files after their contents are
  owned by `calls.cpp` / `calls.hpp`.
- Remove their build metadata entries.
- Clean up stale includes or forward declarations.
- Verify no references remain to deleted helper paths.

Completion check:
- Build metadata no longer references the removed helper translation units or
  header, and repository search finds no stale includes or path references
  outside historical artifacts.

### Step 5: Validate Calls Fold-Back

Goal: prove the fold-back preserved behavior and linkage.

Primary target: focused AArch64 call-lowering tests plus a sufficient backend
build or test bucket.

Actions:
- Run a fresh compile or build proof after the final build metadata update.
- Run focused AArch64 call lowering tests that cover call-boundary moves, byval
  aggregate handling, dynamic-stack helper calls, call result recording, and
  diagnostics where available.
- Run a broader backend bucket when include/linkage blast radius extends beyond
  the focused tests or when multiple narrow packets have landed.
- Store canonical proof in `test_after.log` when delegated by the supervisor.

Completion check:
- Validation is green, `todo.md` records the exact commands and results, and
  the diff is limited to the calls fold-back scope plus required build/include
  fallout.
