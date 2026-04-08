Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

# Active Item

- Step 2: switch one shared helper seam off `BackendModule`
- Current slice: remove the now-dead legacy `BackendModule` call-crossing
  parser/emitter route after the shared BIR call-crossing view and LIR-through-
  BIR path are in place

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
- Reran the full `ctest --test-dir build -j --output-on-failure` suite after
  the dual-identity subtraction migration and confirmed parity again:
  `2834/2834` tests passed in `test_after.log`
- Added focused shared-util coverage for the new BIR dual-identity direct-call
  subtraction parser view
- Added a bounded `try_lower_to_bir(...)` path for the dual-identity
  subtraction LIR family so it synthesizes shared BIR identity helpers and a
  BIR subtraction caller instead of depending on x86's private direct LIR
  emitter seam
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_dual_identity_direct_call_sub_module(...)` view for
  direct BIR emission and removed the earlier x86-only direct-LIR fast path
  for that family
- Added focused BIR lowering coverage plus direct-BIR and LIR-through-BIR x86
  pipeline coverage for the dual-identity direct-call subtraction family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, then reran the
  focused `dual_identity` cases in both test binaries successfully
- Added a shared BIR parser for the minimal single-argument identity-return
  direct-call helper/main module shape
- Added a bounded `try_lower_to_bir(...)` path for the matching minimal LIR
  identity-return direct-call family so it synthesizes a shared BIR helper and
  caller instead of depending on the x86-only direct LIR emitter seam
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_direct_call_identity_arg_module(...)` view for direct BIR
  emission and the shared identity-return slice abstraction for LIR input
- Added focused shared-util coverage, BIR lowering coverage, and x86
  end-to-end pipeline coverage for the identity-return direct-call family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran both test
  binaries directly, and reran the full `ctest --test-dir build -j
  --output-on-failure` suite successfully with `2834/2834` tests passing in
  `test_after.log`
- Added a bounded `try_lower_to_bir(...)` path for the folded two-argument
  direct-call LIR family so it collapses the helper/main slice to a shared BIR
  caller return-immediate shape instead of depending on x86's private legacy
  parser dispatch
- Removed the now-dead x86-only folded two-argument direct-call direct-emission
  dispatch from `src/backend/x86/codegen/emit.cpp`
- Added focused BIR lowering coverage for the folded two-argument direct-call
  LIR slice plus an x86 end-to-end pipeline test proving the LIR route now
  stays on the shared BIR-owned emitter path
- Rebuilt `backend_bir_tests`, reran the binary directly, and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully with
  `2834/2834` tests passing in `test_after.log`
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
- Added a shared BIR parser for the minimal single-argument add-immediate
  direct-call helper/main module shape
- Added a bounded `try_lower_to_bir(...)` path for the matching minimal LIR
  add-immediate direct-call family so it synthesizes a shared BIR helper and
  caller instead of depending on x86-only direct LIR emission
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_direct_call_add_imm_module(...)` view for direct BIR
  emission and the shared add-immediate slice abstraction for LIR input
- Added focused shared-util coverage, BIR lowering coverage, and x86
  end-to-end pipeline coverage for the add-immediate direct-call family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully with `2834/2834` tests passing in `test_after.log`
- Added a shared BIR parser for the minimal call-crossing direct-call
  helper/main family, preserving the helper identity, recovered source add
  value name, source immediate, and helper add immediate
- Added a bounded `try_lower_to_bir(...)` path for the minimal call-crossing
  LIR family so x86 call-crossing input now routes through shared BIR instead
  of depending on the direct-LIR emission shortcut
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_call_crossing_direct_call_module(...)` for direct BIR
  emission and removed the earlier direct-LIR call-crossing fast path from
  `try_emit_direct_lir(...)`
- Added focused shared-util coverage, BIR lowering coverage, and x86 pipeline
  coverage for both direct-BIR and LIR-through-BIR minimal call-crossing
  direct-call modules
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  against the repo's current dirty baseline with the same 13 known failures in
  `test_before.log` and `test_after.log`

# Next

- Inventory the remaining x86-only rich direct-call helper families in
  `src/backend/x86/codegen/emit.cpp` and `src/backend/lowering/call_decode.hpp`
  to choose the next smallest seam after the dual-identity subtraction route
- Continue shrinking stale direct-call helper families one seam at a time now
  that the zero-arg, declared-direct-call, minimal two-argument helper/main,
  single-argument add-immediate, single-argument identity-return, folded
  two-argument helper/main, dual-identity subtraction, and call-crossing paths
  no longer need private x86 LIR dispatch
- Confirm the legacy `BackendModule` call-crossing parser family is now dead,
  then delete the dead helper view and any remaining adapter-only references
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
- This slice removes another x86-only direct-call seam from the legacy route:
  the minimal single-argument add-immediate helper/main family now lowers
  through shared BIR and has a direct-BIR emitter path on x86
- This slice removes another x86-only direct-call seam from the legacy route:
  the minimal single-argument identity-return helper/main family now lowers
  through shared BIR and has a direct-BIR emitter path on x86
- This slice removes the x86 emitter's private folded two-argument direct-call
  LIR dispatch; the helper/main family now lowers through shared BIR as a
  collapsed caller return-immediate shape
- This slice removes the x86 emitter's private dual-identity subtraction
  direct-call LIR fast path; the helper/main family now lowers through shared
  BIR and has a direct-BIR emitter path on x86
- `test_before.log` and `test_after.log` both recorded the same current
  full-suite baseline for this run: `99%` passed with 13 known failures
  (`backend_runtime_*` plus two aarch64 `c_testsuite` cases), so this slice
  was monotonic against the repo's dirty state
