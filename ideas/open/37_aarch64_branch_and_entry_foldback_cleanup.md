# AArch64 Branch And Entry Fold-Back Cleanup

## Goal

Fold target-local comparison branch-fusion and prologue entry-formal helpers
back into their reference-style AArch64 owners.

## Why This Exists

The classification table marked `comparison_branch_fusion.*` and
`prologue_entry_formals.cpp` as target-local mechanical layout debt. Both
families consume prepared facts and emit AArch64 instruction sequences owned
by existing reference-style owners: comparison/branch lowering and prologue
entry setup.

## In Scope

- Fold these helper families into their owner files:
  - `src/backend/mir/aarch64/codegen/comparison_branch_fusion.cpp`
  - `src/backend/mir/aarch64/codegen/comparison_branch_fusion.hpp`
  - `src/backend/mir/aarch64/codegen/prologue_entry_formals.cpp`
- Use `comparison.cpp` / `comparison.hpp` for branch-fusion emission.
- Use `prologue.cpp` / `prologue.hpp` for entry-formal publication and
  prologue setup.
- Update build metadata and includes only as needed for the consolidation.
- Preserve prepared branch-condition facts and prepared formal publication
  plans as inputs consumed by AArch64.

## Out Of Scope

- Changing compare semantics, condition-code policy, branch target selection,
  or prepared branch-condition planning.
- Changing ABI entry formal layout, byval entry-copy semantics, F128 carrier
  handling, or shared formal preparation.
- Folding calls, memory, dispatch, or module compatibility helpers.
- Broad prologue/frame-layout rewrites.

## Acceptance Criteria

- The branch-fusion and entry-formal helper families are owned by the
  comparison and prologue owners rather than standalone extra files.
- Existing branch lowering, formal publication, byval entry copy, and F128
  entry behavior remain intact.
- Build metadata no longer references removed helper translation units.
- Validation covers focused comparison/branch lowering, prologue/formal
  handling, and an appropriate backend build or test bucket.

## Reviewer Reject Signals

- The patch changes prepared branch-condition or formal-publication authority
  while claiming to do mechanical fold-back.
- The patch special-cases one comparison operator, branch label, function
  name, formal index, aggregate shape, or F128 fixture.
- The patch weakens unsupported diagnostics or rewrites expectations to hide a
  branch or prologue regression.
- The patch leaves branch-fusion or entry-formal ownership split behind a new
  helper file outside `comparison` or `prologue`.
- The patch mixes in unrelated calls, memory, dispatch, returns, or module
  compatibility cleanup.
