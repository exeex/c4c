Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4 x86 emitter tightening. Continue shrinking the
remaining x86 emitter-local `lower_lir_to_backend_module(...)` fallback for the
next bounded local-runtime family after the double-indirect local-pointer
conditional chain.

Completed in this slice:

- taught the direct x86 LIR emit path in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to constant-fold the bounded local-slot arithmetic / two-local return family
  directly to the minimal return asm path so those slices no longer need
  `lower_lir_to_backend_module(...)` on the explicit x86 entrypoint
- taught the direct x86 LIR emit path in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to recognize the bounded countdown do-while slice and fold it directly to
  the final return asm path so that family no longer needs
  `lower_lir_to_backend_module(...)` on the explicit x86 entrypoint
- proved the production deletion with a new parity regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  that keeps the direct x86 LIR emit surface and the explicit lowered seam on
  identical asm for the bounded countdown do-while family
- proved the production deletion with new explicit-LIR parity regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  for the local-slot arithmetic chain and two-local scalar-slot return slices
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
- added a direct-LIR x86 parser/emitter path for the bounded extern scalar
  global-load family in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so that family no longer needs `lower_lir_to_backend_module(...)` on the
  direct x86 entrypoint
- replaced the old explicit-lowered extern scalar global-load proof with a
  direct render test plus direct-vs-lowered parity coverage in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
- added a bounded direct x86 LIR parser/emitter path for the multi-`printf`
  vararg declared-direct-call slice in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so the explicit x86 LIR entrypoint no longer needs
  `lower_lir_to_backend_module(...)` before keeping that slice on the asm path
- proved the production deletion with a new explicit-emit regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  that keeps the bounded multi-`printf` slice on assembly output from the
  direct x86 LIR surface and the normal backend selection path
- added a bounded direct x86 LIR parser/emitter path for the single-local
  single-argument helper-call slice in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so the explicit x86 LIR entrypoint no longer needs
  `lower_lir_to_backend_module(...)` before keeping that slice on the asm path
- rechecked the direct x86 scalar-global load and store-reload families with
  explicit-LIR parity coverage in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so those bounded global families stay pinned to identical direct-vs-lowered
  assembly output while Step 4 keeps shrinking legacy owners
- taught the direct x86 LIR return-immediate parser in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to track bounded local pointer-slot aliases through the local round-trip
  store/load pattern so that slice no longer needs
  `lower_lir_to_backend_module(...)` on the explicit x86 entrypoint
- proved the production deletion with a new explicit-LIR parity regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  for the bounded local pointer round-trip slice
- added a bounded direct x86 LIR parser for the goto-only constant-return
  branch chain in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so that family no longer needs `lower_lir_to_backend_module(...)` on the
  explicit x86 entrypoint just to discover the final immediate return
- proved the production deletion with a new explicit-LIR parity regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  for the bounded goto-only constant-return branch chain
- added a bounded direct x86 LIR constant-fold interpreter for the
  double-indirect local-pointer conditional family in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so that family no longer needs `lower_lir_to_backend_module(...)` on the
  explicit x86 entrypoint — the interpreter tracks local alloca slots, pointer
  aliases, stores, loads, comparisons, and conditional branches to fold the
  entire computation to a constant return value
- proved the production deletion with a new explicit-LIR parity regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  for the bounded double-indirect local-pointer conditional family

Next slice:

- prefer another local-runtime family that still hits
  `lower_lir_to_backend_module(...)` over compatibility-only probe growth
- keep lowered-backend tests scoped to compatibility seams that still exist
  after the production deletion
- prove the next deletion with focused x86 backend tests and
  `ctest --test-dir build -R backend -j1 --output-on-failure`

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
- backend regression scope is currently green at `402` passed / `0` failed via
  `ctest --test-dir build -R backend -j1 --output-on-failure`
- latest Step 4 follow-through also removes the bounded local-slot arithmetic /
  two-local scalar-slot dependency on `lower_lir_to_backend_module(...)` from
  the direct x86 LIR entrypoint
- latest Step 4 follow-through also removes the bounded single-local
  single-argument direct-call dependency on `lower_lir_to_backend_module(...)`
  from the direct x86 LIR entrypoint
- latest Step 4 follow-through also removes the bounded multi-`printf` vararg
  declared-direct-call dependency on `lower_lir_to_backend_module(...)` from
  the direct x86 LIR entrypoint
- plain `ctest --test-dir build -R backend --output-on-failure` was flaky in
  this run and reported transient `c_testsuite_aarch64_backend_*` failures,
  but the serial rerun was fully green
- this slice kept the backend scope monotonic at `402` passed / `0` failed
- latest Step 4 follow-through tightened the x86 explicit LIR surface so
  bounded return families no longer need the x86 post-adaptation legacy
  recognition branches
- latest Step 4 follow-through also makes the direct x86 LIR entrypoint try
  BIR emission before legacy backend-IR lowering, which removes one live
  production dependency on `lower_lir_to_backend_module(...)` for
  BIR-lowerable cast/return slices
- latest Step 4 follow-through also removes the bounded extern scalar
  global-load dependency on `lower_lir_to_backend_module(...)` from the direct
  x86 LIR entrypoint
- latest Step 4 follow-through also removes the bounded countdown do-while
  dependency on `lower_lir_to_backend_module(...)` from the direct x86 LIR
  entrypoint and keeps the direct/lowered x86 seams on identical asm
- latest Step 4 follow-through also removes the bounded local pointer
  round-trip dependency on `lower_lir_to_backend_module(...)` from the direct
  x86 LIR entrypoint and keeps the direct/lowered x86 seams on identical asm
- latest Step 4 follow-through also removes the bounded goto-only
  constant-return branch-chain dependency on
  `lower_lir_to_backend_module(...)` from the direct x86 LIR entrypoint and
  keeps the direct/lowered x86 seams on identical asm
- latest Step 4 prep adds explicit direct-x86 parity coverage for the bounded
  member-array runtime family so the next local-runtime deletion can target a
  concrete green direct-entry seam instead of adding more compatibility-only
  probes
- latest Step 4 follow-through also removes the bounded double-indirect
  local-pointer conditional dependency on `lower_lir_to_backend_module(...)` from
  the direct x86 LIR entrypoint via a mini constant-fold interpreter that tracks
  alloca slots, pointer aliases, and conditional branches
- latest Step 4 follow-through extends the x86 LIR constant-fold interpreter
  to handle CastOp (ZExt, SExt, Trunc), BinOp (Add, Sub, Mul, And, Or, Xor,
  Shl, LShr, AShr), and SelectOp instructions, removing the bounded
  mixed-cast, truncating-binop, and select constant-conditional goto-return
  families from the `lower_lir_to_backend_module(...)` fallback path
- latest Step 4 follow-through extends the x86 LIR constant-fold interpreter
  with PhiOp support (predecessor-label tracking) and unsigned compare
  predicates (ult, ule, ugt, uge), removing the bounded local-slot phi-join
  constant-return family from the LLVM-text fallback path — patterns with
  both alloca slots and phi nodes now fold to constant returns via the
  interpreter instead of falling through to raw LLVM IR output
- latest Step 4 follow-through extends the x86 LIR constant-fold interpreter
  with floating-point support: SIToFP/UIToFP/FPToSI/FPToUI/FPExt/FPTrunc
  casts, float/double alloca store/load tracking, ordered and unordered fcmp
  predicates (oeq, one, olt, ole, ogt, oge, ord, uno, ueq, une, ult, ule,
  ugt, uge), and SDiv/SRem in the multi-block BinOp handler — removing the
  bounded float-local-slot SIToFP equality family from the
  `lower_lir_to_backend_module(...)` fallback path
