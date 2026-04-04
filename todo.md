Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4, shrink the now-explicit AArch64-only file-output
asm fallback users and prepare the final legacy backend-IR deletion pass.

Iteration target: remove the bounded non-FP direct-call AArch64 file-output
asm rescue caller `00121`, then continue with the next still-blocked family.

Immediate next slice:

- re-probe the remaining float/ABI-heavy AArch64 rescue callers `00174` and
  `00204`
- choose the smaller still-bounded family and batch its emitter gap fix
  together with removing the matching file-output allowlist tag

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
  `00174` and `00204`
- the small scalar/FP audit split cleanly into two buckets:
  `00113`, `00119`, and `00123` all failed because the AArch64 general
  stack-spill emitter rejected float compares and float casts
  (`sitofp` plus ordered `fcmp` materialization), while the now-cleared
  `00121` direct-call family was separate and should not be grouped with the
  float slice
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
- taught the shared direct-call matcher plus the AArch64 emitter to keep the
  zero-arg `void` helper-call plus immediate-`0` return shape on the backend
  asm path, and removed external/backend AArch64 c-testsuite case `00080`
  from the remaining file-output rescue bucket
- taught the minimal single-function return matchers to ignore unrelated
  declarations when there is exactly one real definition, added AArch64/x86
  backend regressions for that seam, and removed external/backend AArch64
  c-testsuite case `00108` from the remaining file-output rescue bucket
- taught the AArch64 backend/native asm path to keep the bounded
  single-argument identity-helper direct-call shape on stdout-native asm and
  removed external/backend AArch64 c-testsuite case `00116` from the remaining
  file-output rescue bucket
- taught the AArch64 general stack-spill emitter to lower bounded float
  cast/compare slices (`sitofp`, ordered `fcmp`, and the matching float/double
  conversion helpers) and removed external/backend AArch64 c-testsuite cases
  `00113`, `00119`, and `00123` from the remaining file-output rescue bucket
- re-probed external/backend AArch64 c-testsuite case `00175`, confirmed it
  already emits backend-native asm on stdout, and removed its stale
  `backend-file-aarch64` allowlist tag so the live file-output rescue bucket
  now shrinks to `00121`, `00174`, and `00204`
- taught the shared AArch64/native-asm path to keep the bounded dual
  identity-helper direct-call subtraction shape (`main = f(1) - g(1)`) on
  stdout-native asm, added a focused backend regression for that slice, and
  removed external/backend AArch64 c-testsuite case `00121` from the
  remaining file-output rescue bucket so the live rescue set now shrinks to
  `00174` and `00204`

Validation baseline:

- blocker: none
- latest focused proving set:
  `build/backend_lir_adapter_aarch64_tests`,
  `build/c4cll --codegen asm --target aarch64-unknown-linux-gnu tests/c/external/c-testsuite/src/00121.c`,
  and
  `ctest --test-dir build -R "c_testsuite_aarch64_backend_src_00121_c" --output-on-failure`
  with native asm emitted on stdout and the regenerated AArch64 backend
  c-testsuite case passing without file-output rescue
- latest backend regression scope:
  `ctest --test-dir build -R backend --output-on-failure`
  with `100% tests passed, 0 tests failed out of 402`
- latest monotonic guard:
  `test_fail_before.log` vs `test_fail_after.log`
  result `PASS`,
  mode `--allow-non-decreasing-passed`,
  `before: passed=2841 failed=0 total=2841`,
  `after: passed=2841 failed=0 total=2841`,
  `new failing tests: 0`
