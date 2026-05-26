# AArch64 Dispatch Publication Fold-Back Cleanup Runbook

Status: Active
Source Idea: ideas/open/36_aarch64_dispatch_publication_foldback_cleanup.md

## Purpose

Fold small target-local AArch64 dispatch diagnostics and publication-common
helper surfaces back into their reference-style owners while preserving prepared
publication authority.

## Goal

Make `dispatch.cpp` / `dispatch.hpp` own dispatch diagnostics, make
`dispatch_publication.cpp` / `dispatch_publication.hpp` own private publication
helper declarations, remove obsolete helper files from build metadata, and
prove behavior with focused AArch64 dispatch/publication validation plus a
credible build or backend bucket.

## Core Rule

This is mechanical ownership cleanup only. Do not change prepared
edge-publication authority, value-home lookup semantics, diagnostic meaning,
fail-closed unsupported paths, or dispatch orchestration behavior.

## Read First

- `ideas/open/36_aarch64_dispatch_publication_foldback_cleanup.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_diagnostics.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_diagnostics.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication_common.hpp`
- AArch64 codegen build metadata that lists dispatch helper translation units

## Current Scope

- Fold these target-local helper families into their owners:
  - `dispatch_diagnostics.cpp`
  - `dispatch_diagnostics.hpp`
  - `dispatch_publication_common.hpp`
- Use `dispatch.cpp` / `dispatch.hpp` for dispatch diagnostics ownership.
- Use `dispatch_publication.cpp` / `dispatch_publication.hpp` for private
  publication helper declarations and helpers.
- Update includes and build metadata only as required by the fold-back.
- Preserve prepared edge-publication and value-home lookup as the semantic
  authority consumed by AArch64.

## Non-Goals

- Do not rework `dispatch_edge_copies.*`, `dispatch_lookup.*`,
  `dispatch_producers.*`, `dispatch_publication.*`, or
  `dispatch_value_materialization.*` beyond include/API fallout.
- Do not move edge-publication, value-flow, or prepared-home authority into
  AArch64 dispatch.
- Do not perform broad dispatch slimming or orchestration rewrites.
- Do not change diagnostic meaning or weaken unsupported-path contracts.
- Do not fold unrelated calls, memory, comparison, branch, prologue, or return
  helpers in this plan.

## Working Model

- Treat `dispatch_diagnostics.*` as target-local diagnostic plumbing that
  belongs with dispatch routing.
- Treat `dispatch_publication_common.hpp` as private publication-owner surface
  unless another AArch64 owner still legitimately needs a declaration.
- Prefer namespace-local helpers after fold-back when external visibility is
  not required.
- Remove helper files only after declarations, call sites, includes, and build
  entries have been folded into the owner surface.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Before editing a helper family, inspect callers and included headers so API
  visibility remains minimal.
- Preserve diagnostic strings, status kinds, and fail-closed unsupported paths.
- Reject route drift that rediscoveres prepared edge-copy, publication, or
  value-home facts in AArch64 instead of consuming prepared records.
- Update `todo.md` with packet progress and proof after each executor packet.
- Use focused proof first, then escalate to a backend bucket or broader
  validation before closure if multiple fold-back packets land.

## Ordered Steps

### Step 1: Inventory Dispatch Helper Ownership

Goal: identify all declarations, definitions, includes, build entries, and
call sites affected by the dispatch/publication fold-back.

Primary target: listed dispatch diagnostics and publication-common helper
files plus AArch64 codegen build metadata.

Actions:
- Inspect `dispatch_diagnostics.*` and classify every declaration/definition as
  dispatch-owner private or externally consumed by another AArch64 owner.
- Inspect `dispatch_publication_common.hpp` and classify each declaration as
  publication-owner private or externally consumed.
- Find all includes of `dispatch_diagnostics.hpp` and
  `dispatch_publication_common.hpp`.
- Locate build metadata entries for folded implementation files.
- Record any helper that cannot be made private without a narrow owner API
  addition.

Completion check:
- `todo.md` records helper groups, external call sites, build entries, and a
  recommended first fold-back packet with no implementation edits.

### Step 2: Fold Dispatch Diagnostics Into Dispatch Owner

Goal: move dispatch diagnostics plumbing into `dispatch.cpp` / `dispatch.hpp`
without changing diagnostic behavior.

Primary target: `src/backend/mir/aarch64/codegen/dispatch.cpp` and
`src/backend/mir/aarch64/codegen/dispatch.hpp`.

Actions:
- Move needed declarations from `dispatch_diagnostics.hpp` into the dispatch
  owner API only when external callers still need them.
- Move implementation from `dispatch_diagnostics.cpp` into `dispatch.cpp`.
- Keep implementation-private helpers namespace-local where possible.
- Update includes and build metadata after call sites compile through the
  dispatch owner.

Completion check:
- No source file includes `dispatch_diagnostics.hpp`, the obsolete translation
  unit is removed from build metadata, and focused dispatch diagnostics behavior
  still passes.

### Step 3: Fold Publication-Common Declarations Into Publication Owner

Goal: move private publication-common declarations into
`dispatch_publication.cpp` / `dispatch_publication.hpp` without changing
publication semantics.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` and
`src/backend/mir/aarch64/codegen/dispatch_publication.hpp`.

Actions:
- Move declarations from `dispatch_publication_common.hpp` into
  `dispatch_publication.hpp` only when another AArch64 owner legitimately
  needs them.
- Prefer namespace-local declarations or definitions in
  `dispatch_publication.cpp` for publication-private helpers.
- Update include sites to use the publication owner header or no header.
- Do not change prepared publication, value-home lookup, or edge-copy
  semantics.

Completion check:
- No source file includes `dispatch_publication_common.hpp`, required
  publication behavior remains reachable through the publication owner, and no
  unrelated dispatch subsystem is rewritten.

### Step 4: Remove Folded Helper Files From Build Metadata

Goal: remove obsolete helper files after ownership has been folded into the
dispatch/publication owners.

Primary target: AArch64 codegen build metadata and the listed helper files.

Actions:
- Delete folded helper source/header files after their contents are owned by
  the target owner files.
- Remove folded implementation files from build metadata.
- Clean stale includes, comments, and forward declarations.
- Verify repository search finds no stale path references outside historical
  artifacts.

Completion check:
- Build metadata no longer references removed helper translation units or
  headers, and stale live-owner references are gone.

### Step 5: Validate Dispatch Publication Fold-Back

Goal: prove the fold-back preserved dispatch/publication behavior and linkage.

Primary target: focused AArch64 dispatch/publication tests plus build or
backend bucket.

Actions:
- Run a fresh build proof after build metadata updates.
- Run focused AArch64 dispatch/publication tests that cover diagnostics,
  publication/value-home updates, producer lookup, and edge-copy interaction
  where available.
- Escalate to a backend bucket when multiple narrow packets have landed or
  include/linkage blast radius extends beyond one focused test.
- Store canonical proof in `test_after.log` when delegated by the supervisor.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  diff is limited to dispatch/publication fold-back scope plus required
  build/include fallout.
