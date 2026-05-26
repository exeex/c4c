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

## Closure Note

Closed on 2026-05-26.

The scoped fold-back is complete:

- `comparison_branch_fusion.cpp` / `comparison_branch_fusion.hpp` were deleted
  after their declarations and implementation moved into
  `comparison.hpp` / `comparison.cpp`.
- `prologue_entry_formals.cpp` was deleted after
  `lower_entry_formal_publications` and its private entry-formal helpers moved
  into `prologue.cpp`; the existing `prologue.hpp` API remains available for
  `dispatch.cpp`.
- Build metadata no longer references the removed helper translation units.
- Live source, test, and build paths no longer reference
  `comparison_branch_fusion` or `prologue_entry_formals`.
- The implementation stayed within the source idea scope and did not change
  prepared branch-condition authority, condition-code mapping, branch target
  selection, ABI entry formal layout, byval entry-copy semantics, F128 carrier
  handling, diagnostics, or test expectations.

Closure evidence:

- Focused comparison/branch proof passed 6/6:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' | tee test_after.log`
- Focused prologue/formal proof passed 4/4:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_function_traversal|backend_prealloc_formal_publications)$' | tee test_after.log`
- Matching backend before/after regression guard passed with 163/163 tests
  before and 163/163 tests after.
- Accepted full-suite baseline at `2fb6f338f` passed 3411/3411 tests.
- `git diff --check` passed for the implementation slices.
