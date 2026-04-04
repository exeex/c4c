Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4, finish shrinking the remaining app-layer LLVM asm
rescue surface and prepare the final legacy backend-IR deletion pass.

Iteration target: execute one fuller Step 4 batch instead of another
single-case probe. The next slice should finish a repo-wide audit of the
remaining `--codegen asm -o <file>.s` callers, convert every already-native
runtime/contract family in that audited set to stdout-native coverage, and
then remove the matching bounded `c4cll` fallback branch if the audit shows no
real caller still depends on it.

Immediate next slice:

- inventory all remaining runtime and contract users of file-output asm in
  [`tests/c/internal/InternalTests.cmake`](/workspaces/c4c/tests/c/internal/InternalTests.cmake)
  and group them by shared seam instead of by individual testcase name
- verify whether the remaining `call_helper` / extern-call-adjacent paths are
  truly blocked, or were only left behind by stale test routing
- inspect the current app fallback in
  [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp) and map each
  branch to a still-live caller family
- capture the production references that still keep legacy lowering live:
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp),
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp),
  [`src/backend/lir_adapter.hpp`](/workspaces/c4c/src/backend/lir_adapter.hpp),
  and
  [`src/backend/lowering/extern_lowering.hpp`](/workspaces/c4c/src/backend/lowering/extern_lowering.hpp)
- land the slice only if it removes one bounded fallback/codepath family plus
  the matching test assumptions in the same commit-sized batch

Slice deliverables:

- a checked-in audit summary in this file that enumerates the remaining
  file-output asm users by family and by reason
- conversion of every already-native caller in the chosen family batch from
  file-output coverage to stdout-native coverage
- removal or tightening of the corresponding fallback logic in
  [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- updated focused backend/runtime tests that prove the fallback surface is
  actually smaller after the slice

Slice acceptance criteria:

- no "touch-and-commit" slice that only reprobes one testcase without reducing
  the remaining fallback/codepath surface
- no slice closes with only documentation or naming churn; it must delete or
  convert at least one real caller/fallback seam
- if the audit finds a still-blocked backend matcher/emitter gap, batch that
  fix together with the test conversion for the same family before stopping
- Step 4 commits should include a real production deletion or fallback removal;
  test-only commits do not count as meaningful Step 4 progress
- when a Step 4 commit adds or migrates tests, those test changes should prove
  the production deletion in the same batch instead of being the sole payload

Out of scope for this slice:

- wholesale deletion of `ir.*` and `lir_to_backend_ir.*` before their live
  callers are fully mapped and cut over
- unrelated Step 2 or Step 3 expansion outside the bounded fallback-removal
  batch

Step 4 remaining surface:

- `c4cll` still contains the app-layer LLVM asm fallback for `--codegen asm`
  file output
- legacy lowering is still live through
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
  and
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- legacy backend IR data structures are still present in
  [`src/backend/ir.hpp`](/workspaces/c4c/src/backend/ir.hpp),
  [`src/backend/ir.cpp`](/workspaces/c4c/src/backend/ir.cpp),
  [`src/backend/ir_validate.cpp`](/workspaces/c4c/src/backend/ir_validate.cpp),
  and
  [`src/backend/ir_printer.cpp`](/workspaces/c4c/src/backend/ir_printer.cpp)
- tests and build wiring still reference `lir_to_backend_ir` directly and will
  need cleanup after the production cutover

Known live references from the current audit:

- `CMakeLists.txt` still builds `src/backend/lowering/lir_to_backend_ir.cpp`
- tests still include `lir_to_backend_ir.hpp` directly in
  [`tests/backend/backend_bir_pipeline_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp),
  [`tests/backend/backend_bir_pipeline_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_aarch64_tests.cpp),
  [`tests/backend/backend_bir_pipeline_riscv64_tests.cpp`](/workspaces/c4c/tests/backend/backend_bir_pipeline_riscv64_tests.cpp),
  and
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
- `lir_to_backend_ir.cpp` is still large enough to deserve a planned cutover
  batch on its own at roughly `3669` lines, and the old backend IR support
  files in `ir.*` still represent another large deletion batch after that
- internal runtime and stdout-contract families no longer rely on `-o <file>.s`
  rescue; the remaining live file-output asm rescue users observed in this
  slice are external/backend AArch64 c-testsuite cases that still lower
  through LLVM IR for varargs-heavy coverage, including
  `00080`, `00108`, `00113`, `00116`, `00119`, `00121`, `00123`,
  `00174`, `00175`, and `00204`
- the app-layer rescue branch that regenerated fresh legacy LLVM IR after a
  backend `--codegen asm` request returned no text has now been removed;
  `c4cll` only rescues file-output asm when the backend explicitly returns
  LLVM IR text, and empty-output paths now fail immediately with the existing
  unsupported-asm diagnostic

Recently completed milestones:

- converted `backend_runtime_call_helper` onto the explicit stdout-native asm
  path and removed the stale runtime skip for that case
- re-probed `call_helper` through `build/c4cll --codegen asm` and confirmed
  backend-native stdout asm on both `x86_64-unknown-linux-gnu` and
  `aarch64-unknown-linux-gnu`
- converted the bounded stdout-native runtime families for `extern_global_array`,
  `string_literal_char`, `global_*pointer*_diff`, `branch_if_*`, `local_temp`,
  `local_array`, local call/parameter slices, and simple returns away from the
  `-o <file>.s` rescue route
- converted bounded stdout-native contract families for scalar/global-load,
  zero-init global-load, `extern_global_array`, and x86/aarch64 extern-call
  object seams away from file-output rescue assumptions
- removed the stale aarch64 declared-direct-call type-decl gate that was
  blocking one extern-call stdout path
- changed unsupported stdout-only `--codegen asm` requests to fail explicitly
  instead of silently printing LLVM-derived fallback text
- removed `llvm_codegen.cpp` production routing and kept fresh backend-target
  route selection on the BIR-by-default path
- removed the dead `c4cll` fallback leg that regenerated legacy LLVM IR after a
  backend-native asm request produced no text, while preserving the still-live
  file-output LLVM-IR rescue used by external AArch64 varargs coverage

Validation baseline:

- blocker: none
- latest focused proving set:
  `backend_contract_aarch64_extern_call_stdout_object`,
  `backend_contract_x86_64_extern_call_stdout_object`,
  `backend_runtime_call_helper`
  with `100% tests passed, 0 tests failed out of 3`
- latest backend regression scope:
  `ctest --test-dir build -R backend --output-on-failure`
  with `100% tests passed, 0 tests failed out of 402`
- latest monotonic guard:
  `test_fail_before.log` vs `test_fail_after.log`
  result `PASS`,
  `before: passed=2840 failed=0 total=2840`,
  `after: passed=2841 failed=0 total=2841`,
  `new failing tests: 0`
