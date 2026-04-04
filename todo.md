Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4, keep shrinking the remaining app-layer LLVM asm
rescue one bounded surface at a time; after converting the bounded
`branch_if_*` runtime-positive family onto the explicit stdout-native asm
path, continue with the next remaining file-routed runtime-positive family.
Next intended slice: convert the bounded global pointer-difference/runtime
family (`global_char_pointer_diff`, `global_int_pointer_diff`, and
`global_int_pointer_roundtrip`) in `tests/c/internal/InternalTests.cmake`
onto the explicit stdout-native asm path, then rerun a focused proving subset
plus the required backend and monotonic regression gates before picking the
next remaining file-routed family.
Blocker: none. The rebuilt `backend_lir_adapter_aarch64_tests` expectation
drift has been realigned to the current native aarch64 backend seam, and the
required backend/full-suite acceptance gates are green again.
Completed in this slice: converted the bounded `branch_if_*`
runtime-positive family in `tests/c/internal/InternalTests.cmake` onto the
explicit stdout-native asm path, so these already-native conditional branch
runtime checks no longer depend on the app-layer `-o <file>.s` LLVM asm
rescue route.
Completed in this slice: probed the same `branch_if_*` family directly
through `build/c4cll --codegen asm` and confirmed that both
`x86_64-unknown-linux-gnu` and `aarch64-unknown-linux-gnu` already emit
backend-native stdout assembly for all ten cases on the current backend seam.
Completed in this slice: regenerated the build files, reran the focused
proving tests (`backend_runtime_branch_if_eq`, `backend_runtime_branch_if_ge`,
`backend_runtime_branch_if_gt`, `backend_runtime_branch_if_le`,
`backend_runtime_branch_if_lt`, `backend_runtime_branch_if_ne`,
`backend_runtime_branch_if_uge`, `backend_runtime_branch_if_ugt`,
`backend_runtime_branch_if_ule`, and `backend_runtime_branch_if_ult`) with
`100% tests passed, 0 tests failed out of 10`, reran the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`)
with `100% tests passed, 0 tests failed out of 401`, regenerated
`test_fail_after.log`, and reran the monotonic full-suite guard over
`test_fail_before.log` versus the refreshed `test_fail_after.log`, with guard
result `PASS` and `before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: converted the bounded `local_temp` runtime-positive
case in `tests/c/internal/InternalTests.cmake` onto the explicit stdout-native
asm path, so this already-native local temporary runtime check no longer
depends on the app-layer `-o <file>.s` LLVM asm rescue route.
Completed in this slice: regenerated the build files, reran the focused
proving test (`backend_runtime_local_temp`) with `100% tests passed, 0 tests
failed out of 1`, reran the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`) with `100% tests
passed, 0 tests failed out of 401`, regenerated `test_fail_after.log`, and
reran the monotonic full-suite guard over `test_fail_before.log` versus the
refreshed `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: re-inventoried the remaining file-routed
`backend_runtime_*` families in `tests/c/internal/InternalTests.cmake` and
probed them directly through `build/c4cll --codegen asm` on both
`x86_64-unknown-linux-gnu` and `aarch64-unknown-linux-gnu`, confirming that
`local_temp`, the full `branch_if_*` family, `global_char_pointer_diff`,
`global_int_pointer_diff`, `global_int_pointer_roundtrip`,
`string_literal_char`, `call_helper`, and `extern_global_array` already emit
backend-native stdout assembly on both targets.
Completed in this slice: converted the bounded `local_array` runtime-positive
case in `tests/c/internal/InternalTests.cmake` onto the explicit stdout-native
asm path, so this already-native local aggregate runtime check no longer
depends on the app-layer `-o <file>.s` LLVM asm rescue route.
Completed in this slice: added a bounded x86_64 direct-emitter slice for the
whole-module member-array helper/main shapes currently emitted for
`param_member_array`, `nested_param_member_array`, and
`nested_member_pointer_array`, so these runtime-positive local
aggregate/member-access cases now emit backend-native stdout assembly instead
of exiting through the explicit stdout-only unsupported diagnostic.
Completed in this slice: added focused x86 backend adapter coverage for those
three member-array runtime shapes and converted the matching runtime-positive
cases in `tests/c/internal/InternalTests.cmake` onto the explicit
stdout-native asm path.
Completed in this slice: rebuilt `backend_lir_adapter_x86_64_tests` and
`c4cll`, reran the focused proving set (`backend_lir_adapter_x86_64_tests`,
`backend_runtime_param_member_array`,
`backend_runtime_nested_param_member_array`, and
`backend_runtime_nested_member_pointer_array`) with `100% tests passed, 0
tests failed out of 4`, reran the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`) with `100% tests
passed, 0 tests failed out of 401`, regenerated `test_fail_after.log`, and
reran the monotonic full-suite guard over `test_fail_before.log` versus the
refreshed `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: probed the local aggregate/member access family
directly through `c4cll --codegen asm` and confirmed that `local_array`
already emits backend-native stdout assembly for both
`x86_64-unknown-linux-gnu` and `aarch64-unknown-linux-gnu`, while
`nested_member_pointer_array`, `nested_param_member_array`, and
`param_member_array` still require the file-output rescue seam on x86_64.
Completed in this slice: regenerated the build files, reran the focused
proving test (`backend_runtime_local_array`) with `100% tests passed, 0 tests
failed out of 1`, reran the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`) with `100% tests
passed, 0 tests failed out of 401`, regenerated `test_fail_after.log`, and
reran the monotonic full-suite guard over `test_fail_before.log` versus the
refreshed `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: converted the bounded local call/parameter runtime
family (`param_slot`, `local_arg_call`, `two_arg_helper`,
`two_arg_local_arg`, `two_arg_second_local_arg`,
`two_arg_first_local_rewrite`, `two_arg_second_local_rewrite`,
`two_arg_both_local_arg`, `two_arg_both_local_first_rewrite`,
`two_arg_both_local_second_rewrite`, and
`two_arg_both_local_double_rewrite`) in
`tests/c/internal/InternalTests.cmake` onto the explicit stdout-native asm
path, so these already-native runtime checks no longer depend on the app-layer
`-o <file>.s` LLVM asm rescue route.
Completed in this slice: probed that same local call/parameter family directly
through `c4cll --codegen asm` and confirmed that both
`aarch64-unknown-linux-gnu` and `x86_64-unknown-linux-gnu` already emit
backend-native stdout assembly for the entire bounded slice on the current
backend seam.
Completed in this slice: rebuilt `c4cll`, reran the focused proving tests
(`backend_runtime_param_slot`, `backend_runtime_local_arg_call`, and the nine
adjacent `backend_runtime_two_arg_*` cases) with `100% tests passed, 0 tests
failed out of 11`, reran the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`) with `100% tests
passed, 0 tests failed out of 401`, regenerated `test_fail_after.log`, and
reran the monotonic full-suite guard over `test_fail_before.log` versus the
refreshed `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: converted the bounded runtime-positive simple return
family (`return_zero`, `return_add`, and `return_add_sub_chain`) in
`tests/c/internal/InternalTests.cmake` onto the explicit stdout-native asm
path, so these backend-native runtime checks no longer depend on the app-layer
`-o <file>.s` LLVM asm rescue route.
Completed in this slice: probed the same simple return family directly through
`c4cll --codegen asm` and confirmed that both `aarch64-unknown-linux-gnu` and
`x86_64-unknown-linux-gnu` already emit backend-native stdout assembly for
these cases on the current backend seam.
Completed in this slice: regenerated the build files, reran the focused
runtime-positive proving tests (`backend_runtime_return_add`,
`backend_runtime_return_add_sub_chain`, and `backend_runtime_return_zero`) with
`100% tests passed, 0 tests failed out of 3`, reran the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`)
with `100% tests passed, 0 tests failed out of 401`, regenerated
`test_fail_after.log`, and reran the monotonic full-suite guard over
`test_fail_before.log` versus the refreshed `test_fail_after.log`, with guard
result `PASS` and `before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: taught the runtime-positive backend harness
(`tests/c/internal/cmake/run_backend_positive_case.cmake`) an explicit
stdout-native asm path for bounded `--codegen asm` runtime cases, while
preserving the existing file-output path for the remaining rescue-dependent
families.
Completed in this slice: converted the bounded runtime-positive
`global_load` / `global_load_zero_init` family in
`tests/c/internal/InternalTests.cmake` onto that stdout-native asm path, so
these backend-native data-surface runtime checks no longer depend on the
app-layer `-o <file>.s` LLVM asm rescue route.
Completed in this slice: rebuilt `c4cll`, reran the focused proving tests
(`backend_runtime_global_load` and `backend_runtime_global_load_zero_init`)
with `100% tests passed, 0 tests failed out of 1` for each, reran the
required backend regression scope (`ctest --test-dir build -R backend
--output-on-failure`) with `100% tests passed, 0 tests failed out of 401`,
regenerated `test_fail_after.log`, and reran the monotonic full-suite guard
over `test_fail_before.log` versus the refreshed `test_fail_after.log`, with
guard result `PASS` and `before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: traced the remaining aarch64 extern-call stdout block
to an unnecessary `adapted.type_decls.empty()` gate in the aarch64 declared-
direct-call matcher, which caused source-lowered modules carrying unrelated
type declarations to skip the native asm path and fall back to LLVM on stdout.
Completed in this slice: removed that aarch64 type-decl gate, added a backend
adapter regression proving the declared-direct-call asm path still matches when
irrelevant lowered type declarations are present, and converted the internal
aarch64 extern-call object contract from the file-output runner onto the
explicit stdout-native contract seam as
`backend_contract_aarch64_extern_call_stdout_object`.
Completed in this slice: probed the remaining `extern_call` file-output
contract family and confirmed the bounded split in current backend behavior:
`x86_64` already emits backend-native stdout assembly for
`tests/c/internal/backend_case/call_helper.c`, while `aarch64` still exits on
the explicit stdout-only guard and therefore must remain on the file-output
contract seam for now.
Completed in this slice: converted the already-native
`backend_contract_x86_64_extern_call_object` file-output contract onto the
explicit stdout-native contract seam as
`backend_contract_x86_64_extern_call_stdout_object`, so this bounded x86_64
extern-call surface no longer depends on `-o <file>.s` rescue assumptions in
the internal backend contract suite.
Completed in this slice: rebuilt the tree, reran the focused renamed stdout
contract (`backend_contract_x86_64_extern_call_stdout_object`) with `100%
tests passed, 0 tests failed out of 1`, reran the required backend regression
scope (`ctest --test-dir build -R backend --output-on-failure`) with `100%
tests passed, 0 tests failed out of 399`, and regenerated
`test_fail_after.log` for the monotonic full-suite guard.
Completed in this slice: reran the monotonic full-suite guard over
`test_fail_before.log` versus the refreshed `test_fail_after.log`, with guard
result `PASS` and `before: passed=394 failed=0 total=394`,
`after: passed=2838 failed=0 total=2838`, and `new failing tests: 0`.
Completed in this slice: repaired the rebuilt
`backend_lir_adapter_aarch64_tests` expectation drift by converting the stale
aarch64 assertions that still expected LLVM-style text or exact direct-vs-
lowered string identity onto the current native-asm seam, including the
parameter-slot, member-array GEP, nested member-pointer/by-value GEP,
bitcast/trunc, phi-join, malformed-signature, and conditional-return
regressions.
Completed in this slice: rebuilt `backend_lir_adapter_aarch64_tests`, reran
that focused suite with `100% tests passed, 0 tests failed out of 1`, reran
the required backend regression scope (`ctest --test-dir build -R backend
--output-on-failure`) with `100% tests passed, 0 tests failed out of 399`,
and regenerated `test_fail_after.log` for the monotonic full-suite guard.
Completed in this slice: reran the monotonic full-suite guard over
`test_fail_before.log` versus the freshly regenerated `test_fail_after.log`,
with guard result `PASS` and `before: passed=394 failed=0 total=394`,
`after: passed=2838 failed=0 total=2838`, and `new failing tests: 0`.
Completed in this slice: restored the previously disabled
`string_literal_char` backend object contract coverage as explicit stdout-native
contract tests for both `aarch64` and `x86_64`, proving that this bounded
family no longer needs the app-layer `-o <file>.s` LLVM asm rescue path to
assemble into object files.
Completed in this slice: rebuilt `c4cll`, reran the focused stdout object
contracts (`backend_contract_aarch64_string_literal_char_stdout_object` and
`backend_contract_x86_64_string_literal_char_stdout_object`) with `100% tests
passed, 0 tests failed out of 1` for each, reran the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`)
with `100% tests passed, 0 tests failed out of 401`, regenerated
`test_fail_after.log`, and reran the monotonic full-suite guard over
`test_fail_before.log` versus the refreshed `test_fail_after.log`, with guard
result `PASS` and `before: passed=394 failed=0 total=394`,
`after: passed=2840 failed=0 total=2840`, and `new failing tests: 0`.
Completed in this slice: reran the focused proving checks around the existing
stdout-native scalar-global-load family plus nearby aarch64 backend c-testsuite
coverage (`backend_contract_aarch64_global_load_stdout_object`,
`backend_contract_aarch64_global_load_zero_init_stdout_object`,
`backend_contract_x86_64_global_load_stdout_object`,
`c_testsuite_aarch64_backend_src_00012_c`,
`c_testsuite_aarch64_backend_src_00064_c`, and
`c_testsuite_aarch64_backend_src_00080_c`) and confirmed they all stayed green
before extending the next stdout-native contract family.
Completed in this slice: converted the already-native `extern_global_array`
backend contract family from file-output coverage onto the explicit stdout
contract seam for `aarch64`, and added the matching `x86_64` stdout-native
object contract so this family no longer depends on `-o <file>.s` rescue
assumptions in the internal backend contract suite.
Completed in this slice: converted the already-native
`backend_contract_aarch64_return_add_object` file-output contract onto the
explicit stdout-native contract seam as
`backend_contract_aarch64_return_add_stdout_object`, so this bounded contract
family no longer relies on `-o <file>.s` rescue assumptions in the internal
backend contract suite.
Completed in this slice: rebuilt `c4cll`, proved the renamed focused stdout
contract with `100% tests passed, 0 tests failed out of 1`, reran the
required backend regression scope (`ctest --test-dir build -R backend
--output-on-failure`) with `100% tests passed, 0 tests failed out of 399`,
and reran the monotonic full-suite guard over `test_fail_before.log` versus
the freshly regenerated `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2838 failed=0 total=2838`, and `new failing tests: 0`.
Completed in this slice: rebuilt the affected targets and reran the focused
stdout-contract proving set
(`backend_contract_aarch64_extern_global_array_stdout_object`,
`backend_contract_x86_64_extern_global_array_stdout_object`, plus the adjacent
scalar-global-load stdout contracts and aarch64 backend c-testsuite cases),
with `100% tests passed, 0 tests failed out of 8`.
Completed in this slice: refreshed stale x86_64 compare-and-branch label
expectations and one aarch64 select-based direct-LIR scaffold expectation in
the backend adapter suites while investigating the broader regression gate;
`backend_lir_adapter_x86_64_tests` is green again, but the rebuilt
`backend_lir_adapter_aarch64_tests` still has wider expectation drift that must
be repaired before the backend and full-suite monotonic gates can pass.
Completed in this slice: moved the existing non-zero scalar-global-load
contract off the file-output runner and onto the explicit stdout-native
contract seam for aarch64, and added the matching x86_64 stdout-native object
contract so this family no longer relies on `-o <file>.s` rescue assumptions in
the internal backend suite.
Completed in this slice: rebuilt `c4cll`, reran the focused proving checks
(`backend_contract_aarch64_global_load_stdout_object`,
`backend_contract_aarch64_global_load_zero_init_stdout_object`, and
`backend_contract_x86_64_global_load_stdout_object`), reran the required
backend regression scope (`ctest --test-dir build -R backend
--output-on-failure`) with `100% tests passed, 0 tests failed out of 398`, and
reran the monotonic full-suite guard over `test_fail_before.log` versus a
freshly regenerated `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2837 failed=0 total=2837`, and `new failing tests: 0`.
Completed in this slice: made unsupported stdout-only `c4cll --codegen asm`
requests fail explicitly instead of silently printing LLVM-derived fallback
assembly, while preserving the current file-based `.s` fallback path that
still keeps the backend and c-testsuite regressions green.
Completed in this slice: added an internal backend regression proving the
aarch64 variadic-double case now exits non-zero on stdout-only asm requests
with a clear diagnostic instead of silently rescuing through LLVM.
Completed in this slice: rebuilt `c4cll`, reran the focused proving tests
(`backend_lir_aarch64_variadic_double_ir`,
`backend_lir_aarch64_variadic_double_asm_unsupported`,
`backend_lir_aarch64_variadic_long_double_ir`, and
`c_testsuite_aarch64_backend_src_00080_c`), reran the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`),
and regenerated `test_fail_after.log` for the monotonic full-suite guard.
Completed in this slice: changed the backend LIR-entry route selector so fresh
LIR input now enters through the BIR-owned route by default, deleted the old
`LegacyFromLirEntry` branch from `backend.hpp/.cpp`, and updated the
target-neutral backend route tests to pin both the new supported-slice BIR
default and the still-temporary unsupported-LIR LLVM-text fallback on RISC-V.
Completed in this slice: removed the production `llvm_codegen.cpp`
pre-lowering path that fabricated legacy `BackendModule(ir.*)` state before asm
codegen, so production callers now hand fresh LIR directly to
`backend::emit_module(...)` instead of routing through
`lower_lir_to_backend_module(...)`.
Completed in this slice: preserved current x86_64/aarch64 asm behavior while
removing that production legacy-IR route by teaching the BIR-first LIR entry
path to fall back to the existing direct-LIR native emitters when BIR lowering
is unsupported or when the current direct-BIR native subset rejects an
otherwise-lowerable module.
Completed in this slice: rebuilt `backend_bir_tests` and `c4cll`, reran the
focused `backend_bir_tests`, reran the proving contract regressions
(`backend_contract_aarch64_extern_global_array_object`,
`backend_contract_x86_64_extern_call_object`,
`c_testsuite_aarch64_backend_src_00012_c`, and
`c_testsuite_aarch64_backend_src_00064_c`), reran the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`),
and reran the monotonic full-suite guard over `test_fail_before.log` versus a
freshly regenerated `test_fail_after.log`, with `100% tests passed, 0 tests
failed out of 394` for backend scope and guard result `PASS` with
`before: passed=394 failed=0 total=394`,
`after: passed=2833 failed=0 total=2833`, and `new failing tests: 0`.
Completed in this slice: added explicit x86 regressions proving the direct
`x86::emit_module(LirModule)` local-array, global-store-reload,
extern-global-array, global-char-pointer-diff, and global-int-pointer-diff
surfaces still match the explicit lowered backend-module seam byte-for-byte,
so the remaining x86 scaffold removals stay pinned to identical native asm
output.
Completed in this slice: removed the matching x86 direct-LIR `BackendModule`
scaffolds for bounded two-arg direct-call, local-array,
extern-global-array, scalar-global-load/store-reload, and global
pointer-difference helpers by threading `target_triple` directly into the
minimal asm helpers instead of fabricating legacy backend modules just for
symbol selection.
Completed in this slice: removed the remaining aarch64 direct-LIR/direct-BIR
string-literal helper `BackendModule` stub by threading `target_triple`
directly into the private-data label and minimal string-literal asm helpers,
while preserving the explicit lowered backend-module path for the structured
string-pool seam.
Completed in this slice: added an explicit aarch64 regression proving the
direct `aarch64::emit_module(LirModule)` string-literal surface still matches
the explicit lowered backend-module seam byte-for-byte, so the final
string-pool scaffold removal stays pinned to identical native asm output.
Completed in this slice: rebuilt the affected backend binaries, reran the
focused `backend_lir_adapter_aarch64_tests` and `backend_bir_tests`, reran the
required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`), and reran the
monotonic full-suite guard over `test_fail_before.log` versus a freshly
regenerated `test_fail_after.log`, with `100% tests passed, 0 tests failed out
of 394` for backend scope and guard result `PASS` with
`before: passed=394 failed=0 total=394`,
`after: passed=2833 failed=0 total=2833`, and `new failing tests: 0`.
Completed in this slice: removed the remaining aarch64 direct-LIR fast-path
`BackendModule` scaffolds for bounded constant-return/global-label helpers by
threading `target_triple` directly into the minimal return, countdown, scalar
global, extern-global, and global-pointer-diff asm helpers, while preserving
the original backend-module symbol path for non-`main` lowered functions.
Completed in this slice: added an explicit aarch64 regression proving the
direct `aarch64::emit_module(LirModule)` extern-global-array surface still
matches the explicit lowered backend-module seam byte-for-byte, so the
scaffold-removal refactor stays pinned to the same native asm output.
Completed in this slice: rebuilt the affected backend test binaries, reran the
targeted `backend_lir_adapter_aarch64_tests` and `backend_bir_tests`, reran
the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`), and reran the
monotonic full-suite guard over `test_fail_before.log` versus a freshly
regenerated `test_fail_after.log`, with `100% tests passed, 0 tests failed out
of 394` for backend scope and guard result `PASS` with
`before: passed=394 failed=0 total=394`,
`after: passed=2833 failed=0 total=2833`, and `new failing tests: 0`.
Completed in this slice: tightened both native emitter headers so
`x86/codegen/emit.hpp` and `aarch64/codegen/emit.hpp` now forward-declare
`c4c::codegen::lir::LirModule` instead of completing the full frontend LIR IR
surface just to expose emitter entrypoints, with a header-boundary regression
assertion proving those includes no longer complete `LirModule`.
Completed in this slice: removed the direct-BIR x86_64/aarch64 conditional-
return `BackendModule` scaffold usage from the BIR emitter path by threading
`target_triple` directly into the symbol/prelude helpers for the bounded
conditional-return family, and also dropped the same stand-in from the direct-
BIR minimal symbol/return/affine/cast helper path where the target triple was
the only required legacy-field input.
Completed in this slice: rebuilt and reran the proving tests
(`backend_shared_util_tests` and `backend_bir_tests`), the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`),
and the monotonic full-suite guard over the existing
`test_fail_before.log` versus a freshly regenerated `test_fail_after.log`,
with `100% tests passed, 0 tests failed out of 394` for backend scope and guard
result `PASS` with `before: passed=394 failed=0 total=394`,
`after: passed=2833 failed=0 total=2833`, and `new failing tests: 0`.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the existing `i32`
`choose2_mixed_then_deeper_post` / `choose2_mixed_then_deeper_post_chain` /
`choose2_mixed_then_deeper_post_chain_tail` split-predecessor
mixed-then-deeper join family, proving that both native emitters now accept
the bounded then-arm `add`/`sub` arithmetic, the bounded else-arm
`add`/`sub`/`add` deeper arithmetic, and the short post-select
`add` / `add`-`sub` / `add`-`sub`-`add` tails without falling back to legacy
backend IR.
Completed in this slice: extended the minimal direct-BIR x86_64 and aarch64
native emitter entrypoints to accept the bounded `i32` compare-fed
split-predecessor `bir.select` family with predecessor-local affine chains and
short post-select arithmetic tails, mirroring the already-supported widened
`i8` path instead of rejecting the module as outside the affine-return subset.
Completed in this slice: reran the focused direct-BIR backend BIR suite
(`ctest --test-dir build -R backend_bir_tests --output-on-failure`), the
required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`), and the monotonic
guard script over `test_fail_before.log` vs `test_fail_after.log`, with
`100% tests passed, 0 tests failed out of 394` for backend scope and guard
result `PASS` with `before: passed=394 failed=0 total=394`,
`after: passed=2833 failed=0 total=2833`, and `new failing tests: 0`.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8`
`choose2_deeper_both_post_chain_tail_u` split-predecessor deeper-affine join
family, proving that both native emitters already preserve the bounded
predecessor-local `add`/`sub`/`add` chains on both sides plus the short
post-select `add`/`sub`/`add` arithmetic tail without falling back to legacy
backend IR.
Completed in this slice: reran the focused direct-BIR backend BIR suite
(`ctest --test-dir build -R backend_bir_tests --output-on-failure`), the
required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`), and the monotonic
full-suite guard with `test_fail_before.log` vs `test_fail_after.log`, with
`100% tests passed, 0 tests failed out of 394` for backend scope and `100%
tests passed, 0 tests failed out of 2833` before and after the full-suite
comparison.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8`
`choose2_deeper_post_chain_tail_u` split-predecessor deeper-then-mixed-affine
join family, proving that both native emitters already preserve the bounded
deeper predecessor-local `add`/`sub`/`add` chain plus the short post-select
`add`/`sub`/`add` arithmetic tail without falling back to legacy backend IR.
Completed in this slice: reran the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`) and the required
monotonic full-suite guard with `test_fail_before.log` vs
`test_fail_after.log`, with `100% tests passed, 0 tests failed out of 2833`
before and after.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the existing `choose2_add_post_chain_tail_ne_u`
split-predecessor join family, proving that both native emitters already
preserve the bounded post-select `add`/`sub`/`add` arithmetic tail without
falling back to legacy backend IR.
Completed in this slice: reran the required regression checks with monotonic
full-suite results (`100% tests passed, 0 tests failed out of 2833` in both
`test_before.log` and `test_after.log`), so the added native-emitter coverage
did not introduce new failures.
Completed in this slice: taught the direct-BIR x86_64 and aarch64 emitter
entrypoints to accept the bounded widened `i8` compare-fed `bir.select`
family with predecessor-local add chains and a short post-select arithmetic
tail, without routing through legacy backend IR adaptation.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the narrow `choose2_add_post_ne_u` fixture so the new
native path is exercised at the backend seam instead of only through the
legacy-backend-module surface.
Completed in this slice: added a structured backend route-selection seam in
`backend.hpp/.cpp` and covered legacy-LIR, BIR-LIR, direct-BIR, and pre-lowered
legacy inputs without relying on `riscv64` passthrough text.
Completed in this slice: extended target-neutral BIR pipeline coverage so the
existing widened `i8` two-parameter add lowering is now exercised through the
explicit BIR route surface instead of living only in lowering-level tests.
Completed in this slice: batch-added widened `i8` compare-return route
coverage (`eq/ne/ult/ule/ugt/uge/slt/sle/sgt/sge`) so this whole lowering
cluster is now exercised at the explicit BIR pipeline seam instead of only in
lowering-only tests.
Completed in this slice: batch-added widened `i8` arithmetic/bitwise/immediate
route coverage (`add/sub/mul/and/or/xor/shl/lshr/ashr/sdiv/udiv/srem/urem`
plus immediate return), so the remaining zero-parameter widened `i8` scaffold
family now has explicit BIR pipeline coverage.
Completed in this slice: refreshed the Step 1 removal-map snapshot in
`ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md` so it matches the
current mixed BIR/legacy contract, including the cleaned header boundary, the
remaining production legacy routes, the app-layer LLVM rescue hooks, and the
still-legacy test buckets.
Completed in this slice: added explicit BIR cast support (`sext`/`zext`/`trunc`)
to the straight-line scaffold, including printer/validator coverage plus
target-neutral lowering and explicit BIR-pipeline tests for parameter-fed cast
shapes that do not collapse into the older compare/immediate folds.
Completed in this slice: extended the x86_64 and aarch64 BIR-pipeline tests to
cover straight-line `sext`/`zext`/`trunc` fixtures, and taught both direct-BIR
native emitters to lower that bounded cast-return family instead of rejecting
it as unsupported.
Completed in this slice: repaired the stale backend baseline around the renamed
`widen_signed` cast fixture, so the backend regression gate improved from
`99%` (`1/394` failing) to `100%` (`0/394` failing).
Completed in this slice: taught both direct-BIR native emitters to accept the
bounded constant compare-fed `bir.select` return shape by routing it through
their existing minimal conditional-return asm path, and added explicit direct-
BIR x86_64/aarch64 pipeline coverage for that family.
Completed in this slice: generalized the direct-BIR x86_64/aarch64
conditional-select path so the native emitters now accept the bounded
parameter-fed `bir.select` family as direct input, including one-parameter
immediate-arm and two-parameter value-select shapes, with explicit pipeline
coverage for both targets.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the existing `choose2_add_post_chain_ne_u` split-
predecessor join family, proving that both native emitters already preserve the
bounded post-select `add`/`sub` arithmetic tail without falling back to legacy
backend IR.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8` `choose2_mixed_post_ne_u`
split-predecessor mixed-affine join family, proving that both native emitters
already preserve the bounded predecessor-local `add`/`sub` affine chains plus
the short post-select `add` tail without falling back to legacy backend IR.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8` `choose2_mixed_post_chain_ne_u`
split-predecessor mixed-affine join family, proving that both native emitters
already preserve the bounded predecessor-local `add`/`sub` affine chains plus
the short post-select `add`/`sub` tail without falling back to legacy backend
IR.
Completed in this slice: reran the required regression checks with monotonic
full-suite results (`100% tests passed, 0 tests failed out of 2833` in both
`test_before.log` and `test_after.log`), so the added native-emitter coverage
did not introduce new failures.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8` `choose2_mixed_post_chain_tail_u`
split-predecessor mixed-affine join family, proving that both native emitters
already preserve the bounded predecessor-local `add`/`sub` affine chains plus
the short post-select `add`/`sub`/`add` tail without falling back to legacy
backend IR.
Completed in this slice: taught the direct aarch64/x86 scalar-global-load
parsers and emitters to treat `i32 zeroinitializer` globals as native asm-owned
input instead of rejecting them until the app-layer `.s` LLVM rescue took over,
including emitting `.bss`/`.zero 4` for that bounded family rather than
printing LLVM IR on stdout.
Completed in this slice: added an internal aarch64 stdout-only backend
contract for `global_load_zero_init.c`, proving that `c4cll --codegen asm`
now emits backend-native assembly directly to stdout for the zero-init scalar
global-load family and assembles to the expected relocatable object without the
file-based fallback path.
Completed in this slice: rebuilt `c4cll`, reran the focused proving checks
(`backend_contract_aarch64_global_load_zero_init_stdout_object`,
`backend_contract_aarch64_global_load_object`,
`backend_runtime_global_load_zero_init`,
`c_testsuite_aarch64_backend_src_00110_c`, and
`c_testsuite_aarch64_backend_src_00142_c`), reran the required backend
regression scope (`ctest --test-dir build -R backend --output-on-failure`),
and reran the monotonic full-suite guard over `test_fail_before.log` versus a
freshly regenerated `test_fail_after.log`, with `100% tests passed, 0 tests
failed out of 397` for backend scope and guard result `PASS` with
`before: passed=394 failed=0 total=394`,
`after: passed=2836 failed=0 total=2836`, and `new failing tests: 0`.
Completed in this slice: reran the required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`) and the required
monotonic full-suite guard with `test_fail_before.log` vs
`test_fail_after.log`, with `100% tests passed, 0 tests failed out of 2833`
before and after.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8`
`choose2_mixed_then_deeper_post_ne_u` split-predecessor mixed-then-deeper
join family, proving that both native emitters already preserve the bounded
then-arm `add`/`sub` mixed arithmetic, the bounded else-arm
`add`/`sub`/`add` deeper arithmetic, and the short post-select `add` tail
without falling back to legacy backend IR.
Completed in this slice: reran the focused direct-BIR backend BIR suite
(`ctest --test-dir build -R backend_bir_tests --output-on-failure`), the
required backend regression scope
(`ctest --test-dir build -R backend --output-on-failure`), and the monotonic
full-suite guard with `test_before.log` vs `test_after.log`, with `100%
tests passed, 0 tests failed out of 394` for backend scope and `100% tests
passed, 0 tests failed out of 2833` before and after the full-suite
comparison.
Completed in this slice: removed the last emitter-local one-field
`BackendModule` fabrication helpers from x86_64/aarch64 native emission by
teaching the x86 string-literal path and the x86 direct-LIR conditional-return
path to call target-triple helpers directly, and by flattening the aarch64
direct-BIR conditional-affine i8/i32 target-triple overloads so they no longer
wrap a synthetic legacy backend module just to reuse symbol/prelude logic.
Completed in this slice: rebuilt the affected backend binaries, reran the
focused `backend_lir_adapter_x86_64_tests`,
`backend_lir_adapter_aarch64_tests`, and `backend_bir_tests`, reran the
required backend regression scope (`ctest --test-dir build -R backend
--output-on-failure`) with `100% tests passed, 0 tests failed out of 394`, and
reran the monotonic guard script over `test_fail_before.log` vs a freshly
regenerated `test_fail_after.log`, with guard result `PASS` and
`before: passed=394 failed=0 total=394`,
`after: passed=2833 failed=0 total=2833`, and `new failing tests: 0`.
Next target: continue Step 4 by auditing whether any other production caller or
route still depends on `BackendModule(ir.*)` ownership after this
`llvm_codegen.cpp` cutover, then start deleting the remaining legacy pre-
lowered route/data structures and the app-layer LLVM asm rescue helpers in
bounded slices that keep current x86_64/aarch64 asm behavior pinned.
