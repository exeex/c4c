# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2 architecture reset, now in `Phase 2: Compile recovery`:
  use the split shared lowering seams as the owning implementation surface and
  recover compile/test health one seam-local family at a time
- current exact slice:
  continue the bounded x86-native variadic runtime lane by moving from the
  now-green integer-only `backend_runtime_variadic_sum2` prepared-LIR slice to
  the still-unsupported floating `backend_runtime_variadic_double_bytes` probe

## Next Slice

- keep the work seam-local in `src/backend/x86/codegen/emit.cpp` and move next
  to the floating-point variadic `backend_runtime_variadic_double_bytes` probe
- avoid broad `try_lower_to_bir_with_options(...)` ordering changes; if another
  phi-bearing seam needs pre-phi ownership, add it as an explicit narrow
  pre-phi lowering hook instead of reopening generic pipeline flow

## Recently Completed

- expanded `src/backend/bir.hpp` so BIR can carry backend-owned type, memory,
  and call metadata needed by the split lowering seam
- added `src/backend/lowering/lir_to_bir/passes.hpp` and wired the translated
  split lowering scaffolds into the tree and build lists
- wrapped the old `try_lower_to_bir(...)` path as a legacy fallback behind
  `try_lower_to_bir_with_options(...)`
- moved type/legalization helper ownership into
  `src/backend/lowering/lir_to_bir/types.cpp`
- moved first memory/materialization and repeated GEP matcher ownership into
  `src/backend/lowering/lir_to_bir/memory.cpp`
- moved first direct-call construction, argument lowering, and basic call
  metadata ownership into `src/backend/lowering/lir_to_bir/calls.cpp`
- split direct-call metadata further so the split call seam now owns:
  `arg_types`, placeholder `arg_abi/result_abi`,
  target-derived `calling_convention`, and declared-function `is_variadic`
- moved the minimal declared direct-call matcher/module lowering path out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/calls.cpp`, including declared/extern
  return-type validation, typed-call surface reconciliation, string-pool
  materialization, and direct-call BIR module construction
- moved the remaining minimal direct-call helper cluster out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/calls.cpp`, covering the zero-arg direct
  call, void direct-call with immediate return, two-arg direct call, single-arg
  add/immediate and identity helpers, folded two-arg direct call, dual identity
  call/sub, and call-crossing direct-call matchers
- exported the split call-owned direct-call entry points through
  `src/backend/lowering/lir_to_bir/passes.hpp` so the monolith now dispatches
  to `calls.cpp` for that helper family instead of owning the definitions
- rewired `src/backend/lowering/lir_to_bir.cpp` dispatch through the exported
  split call-seam entry point and removed the now-dead monolith helper
  wrappers for that path
- moved the last shared minimal-signature/direct-call helper cluster out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/calls.cpp`, exporting the call-owned
  signature/return-width helpers through `passes.hpp` so the monolith no
  longer owns duplicate copies
- verified the tree rebuilds cleanly after the declared direct-call ownership
  move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the remaining minimal
  direct-call helper extraction:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after centralizing the residual
  call-owned signature helpers in `calls.cpp`:
  `cmake --build build -j8` succeeds
- re-ran seam-local backend validation after the declared direct-call move and
  confirmed the same pre-existing unrelated failure buckets remain:
  `./build/backend_bir_tests` still fails across existing shared-BIR lowering
  acceptance/support cases, and `./build/backend_shared_util_tests` still
  aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the direct-call helper
  extraction and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- moved the minimal global char-pointer-diff and global int-pointer-diff
  matcher/module lowerers out of `src/backend/lowering/lir_to_bir.cpp` and
  into `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry points through `passes.hpp` so the monolith now only
  dispatches those address-decoding seams
- moved the minimal extern/global array-load matcher/module lowerer out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry point through `passes.hpp` so the monolith now only
  dispatches that GEP/address-decoding seam
- moved the minimal plain and extern scalar global-load matcher/module lowerers
  out of `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry points through `passes.hpp` so the monolith now only
  dispatches those scalar global read seams
- moved the remaining scalar global store+reload matcher/module lowerer out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry point through `passes.hpp` so the monolith now only
  dispatches the residual scalar global write+reload seam
- verified the tree still rebuilds cleanly after the global pointer-diff
  ownership move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the extern/global array-load
  ownership move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the plain/extern scalar
  global-load ownership move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the scalar global
  store+reload ownership move:
  `cmake --build build -j8` succeeds
- re-ran the same seam-local backend executables after the global pointer-diff
  ownership move and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the extern/global
  array-load ownership move and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the plain/extern scalar
  global-load ownership move and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the scalar global
  store+reload ownership move and observed the same pre-existing failure
  shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- moved the minimal string-literal compare phi-return matcher/module lowerer
  out of `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry point through `passes.hpp` so the monolith now only
  dispatches that residual string/global byte-address seam
- verified the tree still rebuilds cleanly after the string-literal
  compare phi-return ownership move:
  `cmake --build build -j8` succeeds
- re-ran the same seam-local backend executables after the string-literal
  compare phi-return ownership move and observed the same pre-existing
  failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- normalized the duplicate integer-width and memory-value legalization
  predicates out of `src/backend/lowering/lir_to_bir/memory.cpp` and the
  monolith into shared `src/backend/lowering/lir_to_bir/types.cpp` helpers so
  the split memory seam no longer owns its own ad hoc typed-text parsing
- verified the tree rebuilds cleanly after that predicate normalization:
  `cmake --build build -j8` succeeds
- switched the split memory-owned typed global/global-array load and
  store-reload matchers to semantic type legalization instead of stale
  text-only checks, so the seam now accepts:
  `test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_global_array_load_lir_slice_with_stale_text`,
  and
  `test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text`
- re-ran the nearby non-stale memory-owned global lowering checks after that
  change and confirmed they still pass:
  `test_bir_lowering_accepts_minimal_scalar_global_load_lir_module`,
  `test_bir_lowering_accepts_minimal_extern_scalar_global_load_lir_module`,
  `test_bir_lowering_accepts_minimal_extern_global_array_load_lir_module`, and
  `test_bir_lowering_accepts_minimal_scalar_global_store_reload_lir_module`
- restored the memory-owned minimal string-literal compare phi-return fold by
  letting `try_lower_to_bir_with_options(...)` give that exact seam one
  pre-phi lowering attempt after CFG normalization but before the scaffold
  phi-erasure pass destroys the join phi shape
- verified the focused shared-BIR string-literal seam is green again:
  `test_bir_lowering_accepts_minimal_string_literal_compare_phi_return_lir_module`
  and
  `test_backend_bir_pipeline_drives_x86_lir_minimal_string_literal_compare_phi_return_through_bir_end_to_end`
  now pass
- added the first x86-native explicit support probes in
  `src/backend/x86/codegen/emit.cpp` for:
  direct-BIR single-block immediate returns and prepared-LIR minimal
  string-literal byte loads
- verified the paired x86 native support probes are green:
  `test_x86_try_emit_module_reports_direct_bir_support_explicitly` and
  `test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
  now pass
- re-ran the nearby memory-owned stale-text/global seam checks after the
  string-literal fix and confirmed they still pass:
  `test_bir_lowering_accepts_minimal_scalar_global_load_lir_module`,
  `test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_global_array_load_lir_slice_with_stale_text`,
  and
  `test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text`
- added a seam-local x86 prepared-LIR matcher in
  `src/backend/x86/codegen/emit.cpp` for the narrowed SysV integer variadic
  `sum2` runtime module after entry-alloca preparation coalesces the temporary
  va-arg spill into `%lv.second`
- verified the narrowed x86 variadic integer runtime slice is green again:
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/variadic_sum2.c`
  now emits the expected bounded native assembly, and
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_variadic_sum2$'`
  now passes

## Blockers

- no active blocker on ownership wiring
- later compile/test recovery still has known unrelated backend test noise, so
  not every existing backend test executable is currently a clean regression
  signal for this lane
- the adjacent floating-point x86 variadic runtime slice
  `backend_runtime_variadic_double_bytes` still fails at the unsupported
  direct-LIR boundary and remains the next bounded probe
- full-suite monotonic validation against the checked-in
  `test_fail_before.log` baseline is still red, but the failure bucket is the
  same broad pre-existing `backend_bir_tests` / route-test phi-select surface
  already visible before this slice rather than a new seam-local
  string-literal/x86-probe regression

## Notes To Resume

- do not add any new testcase-shaped x86 direct-LIR matcher while this reset
  is active
- the current goal is structural ownership, not behavioral completion
- the call seam now owns the minimal signature/return-width helper family;
  next slices should avoid reintroducing those helpers into the monolith
- the memory seam now owns the two minimal global pointer-diff matchers; keep
  new GEP/address-form decoding work out of the monolith when extending this
  lane
- the memory seam now also owns the minimal extern/global array-load matcher;
  keep residual global-address and GEP decoding moves on the split seam side
- the memory seam now also owns the minimal plain and extern scalar
  global-load matchers plus the remaining scalar global store/reload seam;
  keep follow-on scalar global load/store ownership on the split seam side
- the memory seam now also owns the minimal string-literal compare
  phi-return seam; use that split implementation as the home for any
  remaining string/global byte-address cleanup instead of rebuilding those
  matchers in the monolith
- the string-literal seam depends on seeing the join phi before scaffold
  phi-erasure; keep that exception explicit and narrow instead of changing the
  global pre-phi lowering order
- when this lane switches to compile recovery, start narrow and seam-local
  before restoring broader regression discipline
- the typed stale-text regressions for scalar global load/store and extern
  global-array load plus the bounded string-literal compare fold are now green,
  so the next slice should move forward from the x86 probe/runtime surface
  rather than reopening those recovered memory checks
