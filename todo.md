Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4, cut the now-explicit AArch64-only file-output asm
fallback users and prepare the final legacy backend-IR deletion pass.

Iteration target: keep external c-testsuite backend coverage on stdout-native
asm by default, preserve file-output rescue only for the known blocked AArch64
varargs-heavy cases, and tighten `c4cll` so non-AArch64 targets no longer get
the file-output LLVM asm fallback.

Immediate next slice:

- use the now-explicit tagged fallback set in
  [`tests/c/external/c-testsuite/allowlist.txt`](/workspaces/c4c/tests/c/external/c-testsuite/allowlist.txt)
  as the only remaining app-layer file-output asm rescue family
- verify whether any of those tagged AArch64 c-testsuite cases have become
  backend-native and can be removed from the fallback bucket
- inspect the production references that still keep legacy lowering live:
  [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp),
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp),
  [`src/backend/lir_adapter.hpp`](/workspaces/c4c/src/backend/lir_adapter.hpp),
  and
  [`src/backend/lowering/extern_lowering.hpp`](/workspaces/c4c/src/backend/lowering/extern_lowering.hpp)
- land the next Step 4 slice only if it removes one of those remaining tagged
  fallback callers or cuts a live legacy lowering dependency in the same batch

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
- internal runtime, stdout-contract, and unblocked external c-testsuite
  backend families now run through stdout-native asm by default
- the remaining live file-output asm rescue users are now explicit allowlist
  tags for external/backend AArch64 c-testsuite coverage:
  `00080`, `00108`, `00113`, `00116`, `00119`, `00121`, `00123`,
  `00174`, `00175`, and `00204`
- `c4cll` now rejects file-output LLVM asm fallback on non-AArch64 targets;
  the app-layer rescue path is retained only for the tagged AArch64 family

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
- switched external c-testsuite backend registration to stdout-native asm by
  default and made the remaining AArch64 file-output rescue callers explicit in
  the allowlist metadata plus runner wiring
- tightened `c4cll` so non-AArch64 `--codegen asm -o <file>.s` requests no
  longer get LLVM-IR-to-asm rescue when the backend fails to emit native asm

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
  `before: passed=2841 failed=0 total=2841`,
  `after: passed=2841 failed=0 total=2841`,
  `new failing tests: 0`
