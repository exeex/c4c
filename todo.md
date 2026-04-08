Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Lock x86 legacy `BackendModule` emitter entry
- [x] Lock aarch64 legacy `BackendModule` emitter entry
- [x] Remove shared backend `BackendModule` public emission entry
- [x] Remove legacy backend-IR-centric test targets from active CMake / ctest
- [x] Coordinate first A/B/C/D parallel cleanup wave
- [x] Reintegrate first worker wave without overlap regressions
- [x] Run broad post-merge backend validation
- [x] Rewrite `plan.md` for the second parallel round
- [x] Refresh per-group ownership notes for A/B/C/D

Current active item:
- restate the post-third-wave close gap and decide whether to open a fourth
  A/B/C worker round or switch to a smaller mainline-only cleanup slice

Mainline rules:
- mainline owns planning files, cross-group conflict resolution, and final
  validation
- worker lanes should not edit other groups' todo files
- worker lanes should compile only their own representative `.o` target before
  handoff
- only mainline performs the final broad build/test pass after reintegration
- this round does not touch LLVM rescue removal in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  or [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp); those remain
  explicit close blockers for idea 41, not part of the current worker wave
- Group D is parked for this round because the remaining
  `backend_lir_adapter*` files are already gone

Legacy seam checklist for third-wave dispatch:
- Group A: reduce x86 emitter-local `BackendModule` and `LirAdapterError`
  ownership in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  now that the dead shim/includes are already gone
- Group B: reduce aarch64 emitter-local `BackendModule` ownership in
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  by collapsing or better isolating the remaining private helper surface
- Group C: shrink the public/shared legacy lowering seam in
  [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp),
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp),
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp),
  and [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)

Close blockers intentionally left outside this wave:
- `ir.hpp`, `ir_printer.*`, and `ir_validate.*` still exist and still have
  active references
- `lir_to_backend_ir.*` still exists and still exports
  `lower_lir_to_backend_module(...)`
- LLVM rescue behavior still exists in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  and [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

Latest completed slice:
- first-wave integration landed and was validated with:
  `cmake -S . -B build`
  `cmake --build build -j8`
  `ctest --test-dir build -R backend -j --output-on-failure`
- public emitter headers for x86/aarch64 no longer expose legacy
  `BackendModule` entry points
- shared backend public emission no longer accepts raw `BackendModule`
- the first parked legacy test scaffolds were deleted and the remaining legacy
  test family is now narrowed to the three `backend_lir_adapter*` files
- second-wave worker reintegration also passed with:
  `cmake -S . -B build`
  `cmake --build build -j8`
  `ctest --test-dir build -R backend -j --output-on-failure`
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  no longer carries the dead internal
  `emit_module(const BackendModule&, ...)` shim and no longer includes
  `ir_printer.hpp` / `ir_validate.hpp`
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  no longer includes `lir_to_backend_ir.hpp`, `ir_printer.hpp`, or
  `ir_validate.hpp`, and now uses local invalid-argument handling instead of
  `LirAdapterError` in its direct-LIR fallback
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  and [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp)
  no longer expose the legacy alias names `BackendTypedCallArgView` and
  `OwnedBackendTypedCallArg`
- the remaining parked legacy adapter tests were deleted:
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  [`tests/backend/backend_lir_adapter_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_aarch64_tests.cpp)
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)

Next integration steps:
- inventory the smallest remaining private `BackendModule` helper surface in
  the x86 and aarch64 emitters after the third-wave collapse
- inventory the remaining external consumers of `LirAdapterError` after the
  shared-header narrowing
- decide whether the next wave should target emitter-internal helper collapse,
  `ir.*` retirement preparation, or `lir_to_backend_ir.*` internalization

Post-second-wave blocker map:
- x86 emitter still contains a large private `BackendModule` helper surface in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp),
  plus many remaining `LirAdapterError` throws/catches tied to that legacy path
- aarch64 emitter still contains a large private `BackendModule` helper surface
  in [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp),
  even though the legacy lowering/printer/validator includes are now gone
- shared backend public headers still expose legacy lowering through
  [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp), which
  forward-declares `BackendModule` and declares
  `lower_lir_to_backend_module(...)`
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
  still exports `LirAdapterError`, `lower_lir_to_backend_module(...)`, and
  `render_module(...)`
- active test/code references to legacy IR utilities still remain, especially
  in [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp),
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp),
  and [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- physical legacy files still present:
  [`src/backend/ir.hpp`](/workspaces/c4c/src/backend/ir.hpp)
  [`src/backend/ir.cpp`](/workspaces/c4c/src/backend/ir.cpp)
  [`src/backend/ir_printer.hpp`](/workspaces/c4c/src/backend/ir_printer.hpp)
  [`src/backend/ir_printer.cpp`](/workspaces/c4c/src/backend/ir_printer.cpp)
  [`src/backend/ir_validate.hpp`](/workspaces/c4c/src/backend/ir_validate.hpp)
  [`src/backend/ir_validate.cpp`](/workspaces/c4c/src/backend/ir_validate.cpp)
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)

Latest completed slice, third wave:
- third-wave worker reintegration passed with:
  `cmake -S . -B build`
  `cmake --build build -j8`
  `ctest --test-dir build -R backend -j --output-on-failure`
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  no longer depends on `LirAdapterError` or
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp);
  the remaining x86 legacy seam is now the private `BackendModule` helper
  surface itself
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  collapsed the thin `BackendModule` pass-through wrappers for symbol/prelude
  plumbing, leaving the remaining ownership concentrated in the semantic
  direct-call helpers
- [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp) no
  longer publicly declares `lower_lir_to_backend_module(...)`
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
  no longer exports `render_module(...)`, and its error surface is narrowed to
  `LirAdapterError::unsupported(...)`, `LirAdapterError::malformed(...)`, and
  `is_unsupported()`
