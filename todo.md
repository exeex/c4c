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

Current active item:
- pivot the active execution lane from "delete more legacy seams" to
  "inventory and fill the remaining BIR parity gaps that still force
  `BackendModule` to exist"

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

BIR parity checklist for this round:
- Group A: identify the x86 emitter's remaining `BackendModule` semantic
  dependencies and translate them into concrete missing BIR shape
- Group B: identify the aarch64 emitter's remaining `BackendModule` semantic
  dependencies and translate them into concrete missing BIR shape
- Group C: identify which `call_decode` / lowering seams still require
  `BackendModule`, and separate "needs BIR expansion" from "needs consumer
  rewrite"

Close blockers intentionally left outside this round:
- `ir.hpp`, `ir_printer.*`, and `ir_validate.*` still exist and still have
  active references
- `lir_to_backend_ir.*` still exists and still has active lowering glue
- LLVM rescue behavior still exists in
  [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
  and [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

Latest completed slice:
- third cleanup wave landed and validated with:
  `cmake -S . -B build`
  `cmake --build build -j8`
  `ctest --test-dir build -R backend -j --output-on-failure`
- [`src/backend/backend.cpp`](/workspaces/c4c/src/backend/backend.cpp) no
  longer includes `lir_to_backend_ir.hpp`
- [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp) no
  longer carries a `BackendModule` forward declaration
- x86 emitter no longer depends on `LirAdapterError`
- aarch64 emitter has shed additional thin `BackendModule` plumbing wrappers
- shared lowering public surface is narrower, but `BackendModule` still
  survives in `call_decode`, emitters, and `lir_to_backend_ir.*`

Next integration steps:
- dispatch the next A/B/C wave against BIR parity inventory and bounded parity
  additions
- reintegrate any small BIR-shape additions alongside consumer notes
- rerun configure/build plus targeted backend validation
- restate the post-parity gap before deciding when `ir.*` deletion becomes safe
