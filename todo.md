Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4 x86 emitter tightening. Remove the bounded
return-immediate/add/sub dependency on `lower_lir_to_backend_module(...)`
inside [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
so the explicit x86 LIR emit surface can stay on BIR/native parsing for those
shapes instead of using legacy backend IR as the recognizer.

Completed in this slice:

- added a shared LIR-native structured parser for the x86 call-crossing
  direct-call seam in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- switched the x86 explicit LIR emit surface to try direct LIR parsers and
  `try_lower_to_bir(...)` before falling back to legacy backend IR
- removed the old x86 emitter-local post-adaptation return-immediate/add/sub
  recognition branches and replaced them with the BIR-first direct-LIR path
- proved the shared parser with a new regression test in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
- routed the direct x86 LIR emit entrypoint through
  `try_lower_to_bir(...)` before
  `lower_lir_to_backend_module(...)` so BIR-lowerable explicit-LIR slices no
  longer depend on legacy backend IR as their first fallback
- proved the new BIR-first direct-LIR seam with a cast-return regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)

Next slice:

- continue shrinking the remaining x86 emitter-local `lower_lir_to_backend_module(...)`
  fallback for direct-call/global families that are still not covered by the
  direct-LIR parser set or the direct-BIR emit path
- prefer the next family where one explicit x86 emitter-local
  `lower_lir_to_backend_module(...)` dependency can be deleted entirely rather
  than only bypassed for BIR-lowerable slices
- prove the deletion with focused x86 backend tests and
  `ctest --test-dir build -R backend --output-on-failure`

Step 4 remaining surface:

- live legacy lowering:
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp),
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- live legacy backend IR:
  [`src/backend/ir.hpp`](/workspaces/c4c/src/backend/ir.hpp),
  [`src/backend/ir.cpp`](/workspaces/c4c/src/backend/ir.cpp),
  [`src/backend/ir_validate.cpp`](/workspaces/c4c/src/backend/ir_validate.cpp),
  [`src/backend/ir_printer.cpp`](/workspaces/c4c/src/backend/ir_printer.cpp)
- live test/build references:
  `CMakeLists.txt`,
  [`tests/backend/backend_bir_pipeline_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp),
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp),
  [`tests/backend/backend_bir_pipeline_riscv64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_riscv64_tests.cpp),
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)

Known current state from the latest audit:

- there are no remaining live `--codegen asm -o <file>.s` LLVM-rescue callers
- `lir_to_backend_ir.cpp` is still a large live file at roughly `3669` lines
- the next bottleneck is not adding more stdout coverage; it is deleting live
  emitter/test ownership of legacy lowering

Acceptance bar for the next Step 4 commit:

- must delete or tighten one live production legacy path
- must not be test-only or probe-only
- if tests change, they should prove the production deletion in the same batch

Recent baseline:

- blocker: none
- backend regression scope is currently green at `402` passed / `0` failed
- this slice kept the backend scope monotonic at `402` passed / `0` failed
- latest Step 4 follow-through tightened the x86 explicit LIR surface so
  bounded return families no longer need the x86 post-adaptation legacy
  recognition branches
- latest Step 4 follow-through also makes the direct x86 LIR entrypoint try
  BIR emission before legacy backend-IR lowering, which removes one live
  production dependency on `lower_lir_to_backend_module(...)` for
  BIR-lowerable cast/return slices
