# AArch64 Module Compatibility Fold-Back Cleanup Runbook

Status: Active
Source Idea: ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md

## Purpose

Fold legacy AArch64 compatibility projection construction back into the module
compile boundary that still needs it, without changing terminal assembly
authority.

## Goal

Make `module_compile.cpp` / `module_compile.hpp` own any remaining legacy
compatibility projection construction, remove obsolete
`compatibility_projection.*` owner files when possible, and prove AArch64
module compilation plus any legacy projection consumers still behave the same.

## Core Rule

This is mechanical ownership cleanup only. Do not make
`FunctionRecord::machine_nodes` a terminal assembly authority, do not route new
behavior through compatibility projections, and do not rewrite final assembly
emission, machine-module printing, or MIR stream ownership.

## Read First

- `ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md`
- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- `src/backend/mir/aarch64/codegen/module_compile.cpp`
- `src/backend/mir/aarch64/codegen/module_compile.hpp`
- AArch64 codegen build metadata that lists compatibility projection sources
- Tests or legacy consumers that read compatibility projection records

## Current Scope

- Fold these helper files into the module compilation owner or a module-private
  compatibility section:
  - `compatibility_projection.cpp`
  - `compatibility_projection.hpp`
- Preserve `module_compile.cpp` / `module_compile.hpp` as the AArch64 module
  build boundary.
- Keep compatibility projection construction limited to legacy consumers that
  still require it.
- Update includes and build metadata only as required by the fold-back.
- Prefer deleting obsolete projection plumbing when caller evidence proves it
  is no longer needed.

## Non-Goals

- Do not make `FunctionRecord::machine_nodes` a new semantic assembly
  authority.
- Do not rewrite final assembly emission, machine-module printing, or MIR stream
  ownership.
- Do not fold calls, dispatch, memory, comparison, prologue, returns, or other
  AArch64 helper families.
- Do not perform broad module representation rewrites unrelated to
  compatibility projection construction.
- Do not weaken or delete compatibility coverage without caller evidence that
  the projection behavior is unused.

## Working Model

- Treat `compatibility_projection.*` as legacy projection construction used at
  the AArch64 module build boundary.
- Treat the MIR stream as the terminal assembly authority.
- If projection records are still consumed, keep construction module-private and
  clearly compatibility-scoped.
- If projection records are no longer consumed, delete the obsolete plumbing
  rather than preserving a renamed standalone owner.

## Execution Rules

- Keep each code-changing step behavior-preserving unless caller evidence proves
  the compatibility projection path is dead.
- Before editing, inventory all declarations, definitions, includes, build
  metadata entries, and projection-record readers.
- Preserve final assembly printing and MIR stream ownership.
- Reject route drift that introduces new behavior through
  `FunctionRecord::machine_nodes` or compatibility projection records.
- Update `todo.md` with packet progress and proof after each executor packet.
- Use focused AArch64 module/projection proof first, then escalate to a backend
  bucket before closure if include/linkage or legacy consumer blast radius
  justifies it.

## Ordered Steps

### Step 1: Inventory Compatibility Projection Ownership

Goal: identify all declarations, definitions, includes, build entries, and
legacy readers affected by the compatibility projection fold-back.

Primary target: `compatibility_projection.*`, `module_compile.*`, AArch64
codegen build metadata, and tests/consumers that inspect projection records.

Actions:
- Inspect `compatibility_projection.*` and classify each declaration/definition
  as module-owner private, externally consumed, or removable if unused.
- Find all includes of `compatibility_projection.hpp`.
- Locate direct callers and any readers of the resulting projection records.
- Locate build metadata entries for `compatibility_projection.cpp`.
- Record whether projection construction must remain in `module_compile.hpp`,
  can be namespace-local in `module_compile.cpp`, or can be deleted.

Completion check:
- `todo.md` records helper groups, include sites, call sites, legacy consumers,
  build metadata entries, any declarations that must remain public, and a
  recommended first mechanical fold-back packet with no implementation edits.

### Step 2: Fold Or Delete Compatibility Projection Construction

Goal: remove the standalone compatibility projection owner while preserving any
needed legacy projection behavior under module compilation.

Primary target: `src/backend/mir/aarch64/codegen/module_compile.cpp` and
`src/backend/mir/aarch64/codegen/module_compile.hpp`.

Actions:
- Move needed declarations from `compatibility_projection.hpp` into the module
  owner API only when external callers still need them.
- Move implementation from `compatibility_projection.cpp` into
  `module_compile.cpp`, preferably namespace-local when only module compilation
  uses it.
- If caller inventory proves the projection path is unused, delete the dead
  projection construction instead of preserving it.
- Update include sites and build metadata after callers compile through the
  module owner.
- Do not change final assembly emission, MIR stream ownership, or module
  printing behavior.

Completion check:
- No live source file includes `compatibility_projection.hpp`, the obsolete
  translation unit is removed from build metadata, obsolete projection helper
  files are deleted, and focused module/projection validation still passes.

### Step 3: Clean Stale Compatibility Ownership References

Goal: remove stale live-owner wording after compatibility projection ownership
has moved or been deleted.

Primary target: AArch64 codegen includes, build metadata, and comments/header
labels affected by the fold-back.

Actions:
- Search live source, test, and build paths for `compatibility_projection`.
- Remove stale includes, comments, and forward declarations that name obsolete
  owners.
- Keep historical or source-intent references untouched unless lifecycle work
  explicitly owns them.

Completion check:
- Live source/build/test paths no longer reference the removed helper files
  except where deliberate compatibility wording remains under the module owner.

### Step 4: Validate Module Compatibility Fold-Back

Goal: prove module compilation and any legacy compatibility projection consumers
still behave correctly after the fold-back.

Primary target: focused AArch64 module compilation/projection tests plus an
appropriate build or backend bucket.

Actions:
- Run a fresh build proof after build metadata updates.
- Run focused AArch64 module compilation tests and any tests that inspect
  compatibility projection records.
- Escalate to a backend bucket when projection consumers or include/linkage
  blast radius extends beyond one focused test.
- Store canonical proof in `test_after.log` when delegated by the supervisor.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  diff is limited to module compatibility fold-back scope plus required
  build/include fallout.
