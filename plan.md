# AArch64 Branch And Entry Fold-Back Cleanup Runbook

Status: Active
Source Idea: ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md

## Purpose

Fold target-local comparison branch-fusion and prologue entry-formal helpers
back into their reference-style AArch64 owners while preserving prepared
branch-condition and formal-publication authority.

## Goal

Make `comparison.cpp` / `comparison.hpp` own branch-fusion emission, make
`prologue.cpp` / `prologue.hpp` own entry-formal publication and prologue setup,
remove obsolete helper files from build metadata, and prove behavior with
focused AArch64 comparison/branch and prologue/formal validation plus an
appropriate backend build or test bucket.

## Core Rule

This is mechanical ownership cleanup only. Do not change compare semantics,
condition-code policy, branch target selection, ABI entry formal layout, byval
entry-copy semantics, F128 carrier handling, or prepared branch/formal planning.

## Read First

- `ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/comparison_branch_fusion.cpp`
- `src/backend/mir/aarch64/codegen/comparison_branch_fusion.hpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/mir/aarch64/codegen/prologue.hpp`
- `src/backend/mir/aarch64/codegen/prologue_entry_formals.cpp`
- AArch64 codegen build metadata that lists branch-fusion or prologue helper
  translation units

## Current Scope

- Fold these target-local helper families into their owners:
  - `comparison_branch_fusion.cpp`
  - `comparison_branch_fusion.hpp`
  - `prologue_entry_formals.cpp`
- Use `comparison.cpp` / `comparison.hpp` for branch-fusion emission.
- Use `prologue.cpp` / `prologue.hpp` for entry-formal publication and prologue
  setup.
- Update includes and build metadata only as required by the fold-back.
- Preserve prepared branch-condition facts and prepared formal publication
  plans as the semantic authority consumed by AArch64.

## Non-Goals

- Do not change compare semantics, condition-code policy, branch target
  selection, or prepared branch-condition planning.
- Do not change ABI entry formal layout, byval entry-copy semantics, F128
  carrier handling, or shared formal preparation.
- Do not fold calls, memory, dispatch, returns, or module compatibility helpers.
- Do not perform broad prologue or frame-layout rewrites.
- Do not weaken unsupported diagnostics or rewrite tests to hide branch or
  prologue regressions.

## Working Model

- Treat `comparison_branch_fusion.*` as target-local branch emission plumbing
  that belongs with comparison/branch lowering.
- Treat `prologue_entry_formals.cpp` as target-local prologue entry setup that
  belongs with the prologue owner.
- Prefer namespace-local helpers after fold-back when external visibility is
  not required.
- Remove helper files only after declarations, call sites, includes, and build
  entries have been folded into the owner surface.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Before editing a helper family, inspect callers and includes so API
  visibility remains minimal.
- Preserve diagnostic strings, unsupported-path contracts, prepared branch
  authority, and prepared formal publication authority.
- Reject route drift that rediscoveres branch-condition or formal-publication
  facts in AArch64 instead of consuming prepared records.
- Update `todo.md` with packet progress and proof after each executor packet.
- Use focused proof first, then escalate to a backend bucket or broader
  validation before closure if multiple fold-back packets land.

## Ordered Steps

### Step 1: Inventory Branch And Entry Helper Ownership

Goal: identify declarations, definitions, includes, build entries, and call
sites affected by the branch-fusion and entry-formal fold-back.

Primary target: listed comparison branch-fusion and prologue entry-formal helper
files plus AArch64 codegen build metadata.

Actions:
- Inspect `comparison_branch_fusion.*` and classify every declaration/definition
  as comparison-owner private or externally consumed by another AArch64 owner.
- Inspect `prologue_entry_formals.cpp` and classify every helper as
  prologue-owner private or externally consumed.
- Find all includes of `comparison_branch_fusion.hpp` and all build metadata
  entries for folded implementation files.
- Locate direct call sites into branch-fusion and entry-formal helper surfaces.
- Record any helper that cannot be made private without a narrow owner API
  addition.

Completion check:
- `todo.md` records helper groups, external call sites, include sites, build
  metadata entries, any declarations that must remain public, and a recommended
  first mechanical fold-back packet with no implementation edits.

### Step 2: Fold Comparison Branch Fusion Into Comparison Owner

Goal: move branch-fusion declarations and implementation into
`comparison.cpp` / `comparison.hpp` without changing branch lowering behavior.

Primary target: `src/backend/mir/aarch64/codegen/comparison.cpp` and
`src/backend/mir/aarch64/codegen/comparison.hpp`.

Actions:
- Move needed declarations from `comparison_branch_fusion.hpp` into the
  comparison owner API only when external callers still need them.
- Move implementation from `comparison_branch_fusion.cpp` into
  `comparison.cpp`.
- Keep implementation-private helpers namespace-local where possible.
- Update includes and build metadata after call sites compile through the
  comparison owner.
- Do not change prepared branch-condition use, condition-code mapping, branch
  target selection, or diagnostics.

Completion check:
- No source file includes `comparison_branch_fusion.hpp`, the obsolete
  translation unit is removed from build metadata, branch-fusion helper files
  are deleted when empty/obsolete, and focused comparison/branch tests still
  pass.

### Step 3: Fold Entry-Formal Publication Into Prologue Owner

Goal: move entry-formal publication and prologue setup helpers into
`prologue.cpp` / `prologue.hpp` without changing ABI or prepared formal
behavior.

Primary target: `src/backend/mir/aarch64/codegen/prologue.cpp` and
`src/backend/mir/aarch64/codegen/prologue.hpp`.

Actions:
- Move implementation from `prologue_entry_formals.cpp` into `prologue.cpp`.
- Add declarations to `prologue.hpp` only if another AArch64 owner still needs
  a public prologue-owner API.
- Keep implementation-private helpers namespace-local where possible.
- Update includes and build metadata after call sites compile through the
  prologue owner.
- Do not change ABI entry formal layout, byval entry-copy semantics, F128
  carrier handling, or shared formal preparation.

Completion check:
- `prologue_entry_formals.cpp` is removed from build metadata and deleted, live
  call sites compile through the prologue owner, and focused prologue/formal,
  byval, and F128 entry behavior still passes.

### Step 4: Clean Stale Ownership References

Goal: remove stale source/build/test references to folded helper owners after
the code has moved.

Primary target: AArch64 codegen includes, build metadata, and owner comments or
header labels affected by the fold-back.

Actions:
- Search live source, test, and build paths for
  `comparison_branch_fusion` and `prologue_entry_formals`.
- Remove stale includes, comments, and forward declarations that name obsolete
  owners.
- Keep historical or source-intent references untouched unless lifecycle work
  explicitly owns them.

Completion check:
- Live source/build/test paths no longer reference the removed helper files
  except where a deliberate historical or lifecycle artifact owns the wording.

### Step 5: Validate Branch And Entry Fold-Back

Goal: prove the fold-back preserved comparison/branch and prologue/formal
behavior and linkage.

Primary target: focused AArch64 comparison/branch and prologue/formal tests plus
build or backend bucket.

Actions:
- Run a fresh build proof after build metadata updates.
- Run focused AArch64 tests covering comparison/branch lowering, branch fusion,
  prologue entry formal handling, byval entry copy, and F128 entry behavior
  where available.
- Escalate to a backend bucket when multiple narrow packets have landed or
  include/linkage blast radius extends beyond one focused test.
- Store canonical proof in `test_after.log` when delegated by the supervisor.

Completion check:
- Validation is green, `todo.md` records exact commands and results, and the
  diff is limited to branch/prologue fold-back scope plus required
  build/include fallout.
