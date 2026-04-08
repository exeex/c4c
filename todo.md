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
- Mainline: line up the first active tests/consumers that should move off
  `ir_printer.*` / `ir_validate.*`

Close blockers intentionally left outside this round:
- `ir.hpp`, `ir_printer.*`, and `ir_validate.*` still exist and still have
  active references
- `lir_to_backend_ir.*` still exists and still has active lowering glue
- LLVM rescue behavior still exists in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  and [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

Latest completed slice:
- initial BIR contract expansion landed and validated with:
  `cmake -S . -B build`
  `cmake --build build -j8`
  `ctest --test-dir build -R backend -j --output-on-failure`
- the first bounded BIR contract slice now exists in
  [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp):
  direct-call shape, `globals`, `string_constants`, and `local_slots`
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  now has a BIR-facing direct-call parser seam alongside the legacy
  `BackendModule` seam
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

Mainline follow-up map:
- first active test consumers to migrate off legacy IR utilities:
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp)
  and
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp)
- remaining `ir.*` utility references still live in:
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp)
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp)
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- remaining large `BackendModule` helper surfaces still live in:
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)

Next integration steps:
- let Group C continue with the next bounded BIR contract:
  extern / address-global classification / local-slot read-write shape
- prepare the first active test consumers to move off legacy IR utilities
- restate which emitter helpers should be switched first after the next BIR
  contract lands
