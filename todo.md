Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Lock x86 legacy `BackendModule` emitter entry
- [x] Lock aarch64 legacy `BackendModule` emitter entry
- [x] Remove shared backend `BackendModule` public emission entry
- [x] Remove legacy backend-IR-centric test targets from active CMake / ctest
- [x] Reintegrate three cleanup waves with green backend validation
- [x] Remove direct `lir_to_backend_ir.hpp` dependency from
  [`src/backend/backend.cpp`](/workspaces/c4c/src/backend/backend.cpp)
- [x] Remove the stale `BackendModule` forward declaration from
  [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp)
- [x] Land the first bounded BIR contract expansion slice and keep backend
  validation green
- [x] Land the declared-direct-call BIR seam and keep targeted backend
  validation green

Current active item:
- continue the BIR contract expansion lane, with Group C focused on the next
  shared contract after the initial direct-call/globals/string/local-slot
  slice and mainline preparing the first consumer migrations

Mainline rules:
- mainline owns planning files, cross-group conflict resolution, and final
  validation
- worker lanes should not edit other groups' todo files
- worker lanes should compile only their own representative `.o` target before
  handoff
- only mainline performs the final broad build/test pass after reintegration
- this round does not touch LLVM rescue removal in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  or [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- Group D stays parked unless a new bounded test-only follow-up appears

BIR contract expansion checklist:
- Group A: prepare the first x86 helper switches once bounded BIR shape lands
- Group B: prepare the first aarch64 helper switches once bounded BIR shape
  lands
- Group C: keep expanding `bir.hpp` with the next bounded contract additions
  beyond the initial direct-call/globals/string/local-slot slice
- Mainline: keep shrinking active legacy IR utility consumers while Group C
  extends bounded BIR contract shape

Close blockers intentionally left outside this round:
- `ir.hpp`, `ir_printer.*`, and `ir_validate.*` still exist and still have
  active references
- `lir_to_backend_ir.*` still exists and still has active lowering glue
- LLVM rescue behavior still exists in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  and [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

Latest completed slice:
- declared-direct-call BIR seam landed in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  together with active test consumer cleanup in
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp)
  and
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp)
- validated with:
  `cmake --build build -j8`
  `./build/backend_bir_tests`
  `./build/backend_shared_util_tests`
  `ctest --test-dir build -R '^backend_runtime_(call_helper|extern_global_array)$' --output-on-failure`
- note: a broad `ctest --test-dir build -R backend -j --output-on-failure`
  run still shows widespread external aarch64 `c-testsuite` `[FRONTEND_FAIL]`
  noise, so mainline is currently relying on targeted backend validation for
  this slice
- the earlier initial BIR contract expansion added:
  direct-call shape, `globals`, `string_constants`, and `local_slots` in
  [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp)
- the first bounded BIR contract slice now exists in
  [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp):
  direct-call shape, `globals`, `string_constants`, and `local_slots`
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  now has BIR-facing direct-call and declared-direct-call parser seams
  alongside the legacy `BackendModule` seam
- [`src/backend/backend.cpp`](/workspaces/c4c/src/backend/backend.cpp) no
  longer includes `lir_to_backend_ir.hpp`
- [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp) no
  longer carries a `BackendModule` forward declaration
- [`tests/backend/backend_header_boundary_tests.cpp`](/workspaces/c4c/tests/backend/backend_header_boundary_tests.cpp)
  has been aligned with the now-removed public `BackendModule` name
- x86 and aarch64 emitter files now carry migration-order notes for the first
  helpers that should switch once additional BIR contracts land
- first active test consumers have started moving off legacy IR utility
  includes:
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp)
  no longer includes `ir_printer.hpp` / `lir_to_backend_ir.hpp`, and
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp)
  no longer includes `ir_printer.hpp` / `ir_validate.hpp`
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
  no longer includes the dead `ir_printer.hpp` dependency
- [`src/backend/lowering/extern_lowering.hpp`](/workspaces/c4c/src/backend/lowering/extern_lowering.hpp)
  no longer exposes `ir.hpp` or `lir_to_backend_ir.hpp`; those legacy
  dependencies are now implementation-local to
  [`src/backend/lowering/extern_lowering.cpp`](/workspaces/c4c/src/backend/lowering/extern_lowering.cpp)

Mainline follow-up map:
- active test-side legacy IR utility includes have been cleared from:
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp)
  and
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp)
- remaining `ir.*` utility references now live primarily in the legacy files
  themselves and not in those active test consumers
- remaining public-ish legacy backend IR exposure is now concentrated in:
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- remaining large `BackendModule` helper surfaces still live in:
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)

Next integration steps:
- let Group C continue with the next bounded BIR contract:
  extern / address-global classification / local-slot read-write shape
- use those new contracts to start switching the first `call_decode` /
  emitter helpers away from `BackendModule`
- restate which emitter helpers should be switched first after the next BIR
  contract lands
