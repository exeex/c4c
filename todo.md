Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Lock x86 legacy `BackendModule` emitter entry
- [x] Lock aarch64 legacy `BackendModule` emitter entry
- [x] Remove shared backend `BackendModule` public emission entry
- [x] Remove legacy backend-IR-centric test targets from active CMake / ctest
- [x] Coordinate A/B/C/D parallel cleanup lanes
- [x] Reintegrate worker commits without overlap regressions
- [x] Run broad post-merge backend validation

Current active item:
- start the next parallel round from the updated A/B/C/D ownership files after
  this reintegration checkpoint, with mainline continuing to own conflict
  resolution and final validation

Mainline rules:
- mainline owns planning files and cross-group conflict resolution
- worker lanes should not edit other groups' todo files
- worker lanes should compile only their own representative `.o` target before handoff
- only mainline performs the final broad build/test pass after reintegration

Latest completed slice:
- reintegrated the first A/B/C/D worker wave into the mainline tree
- kept the x86 and aarch64 public emitter headers aligned with the already
  locked implementations by removing the legacy `BackendModule` entry
  declarations from
  [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp)
  and
  [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp)
- kept the aarch64 emitter tail explicit by restoring the local
  `fail_legacy_backend_ir_not_implemented()` helper in
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  after the header-side boundary cleanup
- shrank the public surface of
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
  by removing the `lower_to_backend_ir(...)` compatibility alias and moving the
  `call_decode.hpp` dependency into the implementation file
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- deleted the pure parked legacy test scaffolding files
  [`tests/backend/backend_module_tests.cpp`](/workspaces/c4c/tests/backend/backend_module_tests.cpp)
  and
  [`tests/backend/backend_x86_64_extracted_tests.cpp`](/workspaces/c4c/tests/backend/backend_x86_64_extracted_tests.cpp)
- removed the two remaining internal `run_backend_module_check_case.cmake`
  registrations from
  [`tests/c/internal/InternalTests.cmake`](/workspaces/c4c/tests/c/internal/InternalTests.cmake)
  that still required the now-removed legacy backend-module emission path
- revalidated the integrated state with:
  `cmake -S . -B build`
  `cmake --build build -j8`
  `ctest --test-dir build -R backend -j --output-on-failure`

Next integration steps:
- update `todoA.md` through `todoD.md` for the second worker wave if we keep
  parallelizing
- continue removing emitter-local `BackendModule` helper ownership in A/B
- continue shrinking `lir_to_backend_ir.*` / `call_decode.*` in C
- retire or migrate the remaining parked `backend_lir_adapter*` files in D
