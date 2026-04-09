# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2 architecture reset, now in `Phase 2: Compile recovery`:
  use the split shared lowering seams as the owning implementation surface and
  recover compile/test health one seam-local family at a time
- current exact slice:
  start narrow compile recovery around the memory-owned global/string address
  seam now that the duplicate typed-width and pointer-value legalization
  predicates have been normalized into `lir_to_bir/types.cpp`

## Next Slice

- run narrow seam-local compile recovery on the memory-owned lowering burst,
  starting with the focused backend utility/bir executables that exercise:
  scalar global load/store, extern array loads, global pointer-diff, and
  string-literal compare lowering
- classify the first post-split failure by owning boundary and add the
  narrowest regression before touching broader backend routing

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

## Blockers

- no active blocker on ownership wiring
- later compile/test recovery still has known unrelated backend test noise, so
  not every existing backend test executable is currently a clean regression
  signal for this lane

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
- when this lane switches to compile recovery, start narrow and seam-local
  before restoring broader regression discipline
