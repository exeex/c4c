Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

# Active Item

- Step 2: switch one shared helper seam off `BackendModule`
- Current slice: inventory the next remaining rich x86 direct-call helper family
  after moving the minimal two-argument helper/main route onto
  `try_lower_to_bir(...)` and the shared BIR direct-call emitter path

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
- Added a bounded `try_lower_to_bir(...)` path for the remaining minimal LIR
  declared-direct-call module shape, including string-pool decoding into BIR
  string constants and synthesized BIR declaration/main functions
- Deleted the redundant x86
  `parse_backend_minimal_declared_direct_call_lir_module(...)` dispatch and
  private LIR-specific emitter overload from `src/backend/x86/codegen/emit.cpp`
- Added focused BIR lowering coverage for the minimal LIR
  declared-direct-call slice plus an x86 end-to-end pipeline test proving LIR
  input now stays on the BIR-owned extern-call route
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully with `2834/2834` tests passing in `test_after.log`
- Added an order-independent `try_lower_to_bir(...)` path for the zero-arg
  minimal LIR direct-call helper/main family so it now synthesizes the shared
  two-function BIR direct-call shape instead of depending on x86's private
  legacy parser dispatch
- Removed the now-dead x86-only
  `parse_backend_minimal_direct_call_lir_module(...)` direct-emission dispatch
  and matching private emitter overload from `src/backend/x86/codegen/emit.cpp`
- Added focused BIR lowering coverage for the zero-arg minimal direct-call LIR
  slice plus an x86 end-to-end pipeline test proving the LIR route now stays
  on the shared BIR direct-call emitter path
- Rebuilt `backend_bir_tests`, reran the binary directly, and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully with
  `2834/2834` tests passing in `test_after.log`
- Added focused shared-util coverage for the new BIR
  two-argument direct-call parser view, plus BIR lowering coverage for the
  matching minimal LIR helper/main slice
- Added focused x86 BIR pipeline coverage for both direct-BIR input and
  LIR-through-BIR input for the minimal two-argument direct-call family
- Added a bounded `try_lower_to_bir(...)` path for the minimal two-argument
  direct-call LIR helper/main shape so it synthesizes a shared BIR helper and
  caller instead of relying on the x86-only direct LIR fast path
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_two_arg_direct_call_module(...)` view for direct BIR
  emission and removed the earlier x86-only direct-LIR dispatch for that
  family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully with `2834/2834` tests passing in `test_after.log`

# Next

- Inventory the remaining x86-only rich direct-call helper families in
  `src/backend/x86/codegen/emit.cpp` and `src/backend/lowering/call_decode.hpp`
  to choose the next smallest seam after the minimal two-argument helper/main
  route
- Continue shrinking stale direct-call helper families one seam at a time now
  that the zero-arg, declared-direct-call, and minimal two-argument helper/main
  paths no longer need private x86 LIR dispatch
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
- The old declared-direct-call helper family is now fully off
  `BackendModule`; the shared parser/emitter route is BIR-owned, and the x86
  LIR-only compatibility seam has also been deleted
- `test_before.log` was not present in the workspace at the start of this run;
  this slice rebuilt the touched backend targets, reran targeted backend
  coverage, and then recorded full-suite success in `test_after.log` at
  `2834/2834`
- This slice removes the x86 emitter's private zero-arg direct-call LIR
  dispatch, but the shared LIR shape recognizer still exists as lowering glue
  for `try_lower_to_bir(...)`
- `test_before.log` was not present in the workspace at the start of this
  slice; the rerun full-suite result in `test_after.log` is `2834/2834`
- This slice removes the x86 emitter's private minimal two-argument direct-call
  LIR dispatch, while keeping the existing LIR recognizer as bounded lowering
  glue for `try_lower_to_bir(...)`
