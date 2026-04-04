Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4 follow-through after deleting the generic
backend-entry legacy-prelowered route. The app-layer LLVM asm fallback is
gone, the caller-owned `BackendModuleInput(BackendModule, ...)` seam is gone,
and the next bounded Step 4 slice should keep shrinking live
`lir_to_backend_ir.*` ownership inside target emitters and tests.

This iteration target: delete one live production legacy-lowering seam instead
of stopping at audit-only work, then leave the next iteration aimed at the
remaining emitter-local and test-local `lir_to_backend_ir.*` ownership.

Current exact target for this iteration:

- delete the redundant production `src/backend/lir_adapter.hpp` shim so
  emitter code and backend tests stop depending on a header whose only purpose
  is to re-export `lower_lir_to_backend_module(...)` from
  `lir_to_backend_ir.hpp`
- remove the no-op backend test includes of that shim and switch the remaining
  direct callsite over to `lower_lir_to_backend_module(...)`
- keep the next slice aimed at the still-live direct
  `lir_to_backend_ir.hpp` includes in the BIR-pipeline/adapter tests plus the
  emitter-local lowering ownership that still cannot be removed yet

Current bounded deletion slice: keep Step 4 focused on production deletion.
- delete the generic backend-entry `LegacyPreloweredModule` route and the stale
  `BackendPipeline` toggle that no longer changed runtime behavior
- move explicit lowered-backend coverage onto a direct
  `emit_module(const BackendModule&, ...)` helper instead of routing lowered
  backend IR back through `BackendModuleInput`
- keep the next batch centered on deleting another live legacy lowering seam
  rather than widening unrelated backend support

Immediate next slice:

- cut one remaining direct `lir_to_backend_ir.hpp` test/include family
  (`backend_bir_pipeline*` or `backend_lir_adapter_tests`) or one
  emitter-local `lower_lir_to_backend_module(...)` ownership seam
- keep the next Step 4 commit deleting a live legacy lowering or backend-IR
  seam rather than doing more preparatory probing
- leave unrelated Step 2/3 expansion and broader `ir.*` removal outside that
  bounded deletion slice

Slice deliverables:

- a checked-in audit summary in this file that enumerates the remaining
  file-output asm users by family and by reason
- conversion of the final remaining caller from file-output coverage to
  stdout-native coverage once the native backend can carry the full `00204`
  larger-aggregate and long-double/HFA family
- removal or tightening of the corresponding fallback logic only when the same
  batch deletes the final `00204` rescue path in production and test wiring
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
- the generic backend entry no longer accepts caller-owned prelowered legacy
  modules; explicit lowered backend-module emission now uses
  `emit_module(const BackendModule&, ...)` directly, so the remaining live
  legacy ownership is narrower and easier to cut in a later Step 4 batch
- `lir_to_backend_ir.cpp` is still large enough to deserve a planned cutover
  batch on its own at roughly `3669` lines, and the old backend IR support
  files in `ir.*` still represent another large deletion batch after that
- internal runtime, stdout-contract, and unblocked external c-testsuite
  backend families now run through stdout-native asm by default
- there are no remaining live `--codegen asm -o <file>.s` LLVM-rescue callers;
  backend asm requests now require native backend assembly on both stdout and
  file output
- the small scalar/FP audit split cleanly into two buckets:
  `00113`, `00119`, and `00123` all failed because the AArch64 general
  stack-spill emitter rejected float compares and float casts
  (`sitofp` plus ordered `fcmp` materialization), while the now-cleared
  `00121` direct-call family was separate and should not be grouped with the
  float slice
- `00174` was blocked by the AArch64 direct LIR emitter rejecting FP binops
  (`fadd`, `fsub`, `fmul`, `fdiv`, `fneg`) plus LLVM `une` float predicates,
  and by losing `double` libcall returns when spilling back to stack; those
  seams now emit native stdout asm and the `backend-file-aarch64` allowlist tag
  is gone
- `00204` is no longer a live Step 4 blocker; it now runs through the regular
  stdout-native AArch64 backend coverage path and no longer needs dedicated
  file-output rescue metadata

Recently completed milestones:

- deleted the redundant production `src/backend/lir_adapter.hpp` shim, updated
  the x86/aarch64 emitters and the no-op backend test includes to depend on
  direct legacy-lowering headers instead of the re-export layer, and switched
  the last shim callsite over to `lower_lir_to_backend_module(...)`
- re-ran `backend_lir_adapter_tests`, then re-ran
  `ctest --test-dir build -R backend --output-on-failure`, and refreshed the
  monotonic backend guard with
  `test_backend_before.log` vs `test_backend_after.log` staying flat at
  `402` passed / `0` failed with no new failures
- deleted the generic backend-entry legacy-prelowered route by removing
  `BackendModuleInput(const BackendModule&, ...)`, removing
  `BackendLoweringRoute::LegacyPreloweredModule`, and collapsing the stale
  `BackendPipeline` option that no longer changed LIR-entry behavior
- moved explicit lowered-backend coverage onto
  `emit_module(const BackendModule&, ...)` so backend-owned IR tests no longer
  route lowered legacy modules back through the generic BIR-vs-legacy entry
  selection surface
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
- taught the AArch64 direct LIR emitter to keep bounded FP arithmetic
  (`fadd`, `fsub`, `fmul`, `fdiv`, `fneg`) plus `double`-return libcalls on
  the native asm path, taught the float compare mapper to keep LLVM `une` on
  stdout-native asm, added focused backend regressions for both seams, and
  removed external/backend AArch64 c-testsuite case `00174` from the
  remaining file-output rescue bucket so the live rescue set now shrinks to
  `00204`
- taught the AArch64 general emitter to keep the low-coupling variadic
  intrinsic slice (`va_start`, `va_end`, `va_copy`) plus direct scalar
  `va_arg` on the native asm path, added focused backend regressions for those
  helpers, and kept aggregate/helper-style variadic lowering explicitly on the
  fallback path so `00204` remains a bounded Step 4 blocker instead of
  silently switching to bad native asm
- taught the AArch64 native asm path to keep helper-only aggregate `va_arg`
  lowering on stdout-native assembly when the variadic callee only walks
  `__va_list_tag_` and copies aggregate payloads, while still forcing fallback
  for the remaining `00204` class where a variadic callee also makes nested
  calls
- converted the bounded internal AArch64 variadic regression set
  (`variadic_double_bytes`, `variadic_pair_second`, `variadic_sum2`, and the
  mixed aggregate helper family) from LLVM-text fallback expectations to
  stdout-native asm contracts/runtime coverage, and updated the focused backend
  adapter tests to assert native aggregate helper asm

Validation baseline:

- blocker:
  `build/c4cll --codegen asm --target aarch64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
  still exits with:
  `error: --codegen asm requires backend-native assembly output on stdout.`
- latest focused repro set:
  `ctest --test-dir build -R "backend_lir_adapter_aarch64_tests|c_testsuite_aarch64_backend_src_00204_c" --output-on-failure`,
  and
  `ctest --test-dir build -R backend --output-on-failure`
  with the repaired aggregate return/stdarg path passing for `00204`,
  the focused/backend regression scope green, and no remaining native
  correctness blocker in that testcase
- latest backend regression scope:
  `ctest --test-dir build -R backend --output-on-failure`
  with `100% tests passed, 0 tests failed out of 402`
- latest monotonic guard:
  `test_backend_before.log` vs `test_backend_after.log`
  result `PASS`,
  mode `--allow-non-decreasing-passed`,
  `before: passed=402 failed=0 total=402`,
  `after: passed=402 failed=0 total=402`,
  `new failing tests: 0`

Latest bounded progress:

- finished the `00204` return/stdarg half of the AArch64 PCS repair batch on
  the native backend path by fixing three concrete emitter gaps together:
  aggregate `store`/memcpy destination ordering for memory-valued stores,
  whole-argument all-or-stack routing for variadic 9-16-byte GP and FP
  aggregate payloads, and float-sized GEP/load/store handling in the generic
  emitter paths that the HFA scratch walker relied on
- re-ran the end-to-end `00204` backend testcase and confirmed the native
  AArch64 asm path now matches the expected output instead of diverging in the
  return-value and `myprintf` stdarg sections
- tightened the focused AArch64 adapter regression by keeping the bounded
  native return/stdarg coverage green alongside the refreshed double-literal
  expectation that now permits the current scratch register choice
- taught the AArch64 native asm path to keep the remaining fixed-call argument
  families from `00204` on stdout-native assembly:
  hidden-by-reference `s17`, direct 9-16-byte aggregate chunks after GP-register
  exhaustion, mixed aggregate stack-call materialization after outgoing `sp`
  adjustment, scalar variadic `fp128` forwarding in `q` registers when slots are
  available, and the corrected LLVM hex-float / `fp128` global initializer
  decoding those paths depend on
- added focused backend adapter coverage for the hidden-sret and fixed HFA
  return/call slices, and refreshed the variadic `fp128` adapter expectations
  to match the corrected native register path
- taught the AArch64 general emitter's direct-call path to keep bounded
  variadic HFA-array payloads plus `fp128` variadic forwarding in focused
  adapter slices by carrying those non-scalar call operands as address-backed
  payloads until the call boundary, then exploding them into FP-register,
  GP-chunk, or outgoing-stack pieces instead of collapsing them to one scalar
  slot
- added focused AArch64 backend adapter regressions for both the bounded
  variadic HFA-array call surface and the bounded `fp128` `printf` forwarding
- removed the broad general-emitter rejection for variadic callees that both
  walk `__va_list_tag_` and make nested calls, and replaced it with a narrower
  guard that only keeps truly unsupported shapes off the stdout-native path
- removed the final external/backend AArch64 c-testsuite `backend-file-aarch64`
  mode for `00204` by converting it to the normal allowlist entry, deleted the
  now-dead test-runner file/stdout mode split, and removed the last `c4cll`
  app-layer LLVM asm fallback so
  `--codegen asm` now fails explicitly whenever the backend does not emit
  native assembly
- reconfigured the c-testsuite registration so `c_testsuite_aarch64_backend_src_00204_c`
  remains in the backend suite as the normal stdout-native backend case instead
  of disappearing with the old file-output tag
- taught the AArch64 general emitter to keep bounded direct 9-16-byte
  non-variadic GP aggregate arguments and returns on the native asm path for
  the `s9`-through-`s16`-style single-field byte-array PCS family by splitting
  the payload across `x` register chunks, storing into caller/callee-backed
  stack slots, and re-materializing the same chunks at call and return
  boundaries instead of rejecting those aggregates outright
- added focused AArch64 backend adapter regressions for the bounded direct
  aggregate parameter and return slice, and re-probed
  `c_testsuite_aarch64_backend_src_00216_c` to confirm the new aggregate path
  does not regress the existing native stdout/runtime coverage

Remaining blocker after this slice:

- no remaining native correctness blocker is left inside `00204`; the rescue
  path cleanup is complete
- the next Step 4 work is broader legacy lowering/backend-IR deletion rather
  than more AArch64 testcase-specific fallback cleanup
