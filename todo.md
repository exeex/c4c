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
rescue one bounded surface at a time, in this iteration by moving the proven
native scalar-global-load stdout path onto explicit contract coverage so the
non-zero initialized global-load family stops depending on file-output rescue
assumptions in the internal backend contract suite.
Next intended slice: rerun the focused backend contracts and c-testsuite cases
around scalar global loads, then either convert the next already-native family
from `-o <file>.s` coverage to stdout-native coverage or delete the matching
app-layer LLVM asm rescue branch once the affected family has direct coverage.
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
