Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

# Active Item

- Step 2: switch one shared helper seam off `BackendModule`
- Current slice: continue the declared-direct-call cleanup into the remaining
  shared legacy helper seam by deleting the dead
  `parse_backend_minimal_declared_direct_call_module(...)` family after both
  native emitters switched to the BIR parser view

# Completed

- Read `plan.md` and the linked source idea
- Confirmed `todo.md` was missing and initialized execution state for this run
- Inspected `call_decode.hpp` and found existing BIR helpers for minimal direct
  call and declared direct call module parsing
- Recorded the full-suite baseline in `test_before.log`
- Fixed `parse_bir_minimal_direct_call_module(...)` so it matches a helper
  definition returning an immediate instead of a declaration-only placeholder
- Extended `parse_bir_minimal_declared_direct_call_module(...)` to preserve
  whether the caller returns the call result or a fixed immediate
- Added focused shared-util coverage for both BIR direct-call parser helpers
- Rebuilt from scratch and confirmed full-suite parity:
  `2834/2834` tests passed before and after
- Added focused x86 BIR pipeline coverage for a direct BIR helper/main
  zero-argument direct-call module
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_direct_call_module(...)` before the affine-return BIR
  subset so direct BIR helper/main call pairs no longer require a legacy
  `BackendModule` parser on that path
- Rebuilt `backend_bir_tests` and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully
- Switched `src/backend/aarch64/codegen/emit.cpp` to consume
  `parse_bir_minimal_direct_call_module(...)` before the affine-return BIR
  subset so direct BIR helper/main call pairs now avoid the legacy
  `BackendModule` route on the native aarch64 entry too
- Added focused aarch64 BIR pipeline coverage for a direct BIR helper/main
  zero-argument direct-call module
- Rebuilt `backend_bir_tests` and confirmed full-suite parity again:
  `2834/2834` tests passed in `test_after.log`
- Extended `parse_bir_minimal_declared_direct_call_module(...)` to decode
  bounded BIR extern-call arguments into concrete immediate vs pointer-style
  metadata using declaration param types plus module symbol tables
- Added shared-util coverage proving the BIR declared-direct-call parser
  preserves pointer-style string-constant operands as concrete extern-call args
- Switched the native aarch64 direct-BIR emitter to consume the new BIR
  declared-direct-call view so that declared extern-call modules no longer need
  the legacy `BackendModule` parser on that path
- Added focused aarch64 direct-BIR pipeline coverage for a declared direct-call
  module that passes a string-constant pointer to an extern declaration and
  then returns a fixed immediate
- Rebuilt the affected backend test binaries and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully
- Confirmed full-suite parity for this slice: `2834/2834` tests passed in
  `test_after.log`
- Added focused x86 direct-BIR pipeline coverage for a declared direct-call
  module that passes a string-constant pointer to an extern declaration and
  then returns a fixed immediate
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_declared_direct_call_module(...)` so direct BIR declared
  extern-call modules no longer require the legacy `BackendModule` parser on
  the native x86 entry
- Rebuilt `backend_bir_tests` and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully
- Confirmed full-suite parity for the x86 declared-direct-call slice:
  `2834/2834` tests passed in `test_after.log`
- Removed the now-dead
  `ParsedBackendMinimalDeclaredDirectCallModuleView` /
  `parse_backend_minimal_declared_direct_call_module(...)` family from
  `src/backend/lowering/call_decode.hpp` once both native emitters had already
  switched to the BIR declared-direct-call view
- Deleted the matching dead `BackendModule`-only declared-direct-call emitter
  overloads from `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp`
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, then reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully
- Reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully and confirmed parity again: `2834/2834` tests passed in
  `test_after.log`

# Next

- Decide whether the next highest-value Step 2 cut is the remaining
  LIR-specific declared-direct-call helper seam or a different still-live
  `BackendModule` parser family in `call_decode.hpp`
- Continue shrinking stale direct-call helper families one seam at a time now
  that the dead backend declared-direct-call route is gone
- Use the next slice to make `lir_to_backend_ir.*` more obviously adapter-only
  before Step 4 collapse work starts

# Notes

- Updated the stale `call_decode.hpp` comment to reflect that BIR already owns
  part of the direct-call surface, while legacy helpers still carry richer
  typed-call / extern-arg detail
- `test_before.log` was not present in the workspace at the start of this
  iteration; the last recorded full-suite baseline in `todo.md` was
  `2834/2834`, and this slice reran the full suite successfully in
  `test_after.log`
- This slice removes one real x86 direct-call consumer from the legacy
  `BackendModule` parser family, but aarch64 and declared-direct-call seams
  still remain
- The aarch64 direct BIR entry now matches x86 for the minimal helper/main
  zero-argument direct-call family; the next live shared seam is the richer
  declared-direct-call surface
- This slice removes the native aarch64 direct-BIR declared-direct-call path
  from the legacy `BackendModule` parser family, and x86 now matches it; the
  remaining live seam is the shared legacy declared-direct-call helper route
- That remaining declared-direct-call helper route is now narrowed to the
  LIR-specific compatibility path; the `BackendModule` variant has been
  deleted from `call_decode.hpp` and both native emitters
