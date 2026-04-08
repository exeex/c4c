Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 4 follow-on deletion. Continue removing the
remaining x86 production-side literal-`main` caller anchors in bounded
multi-block emitter families after the conditional-return slice, starting with
the still-literal lowered conditional-phi-join matcher in
[`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
and proving each deletion on the explicit lowered backend emit surface plus
normal backend selection.

Completed in this slice:

- removed the bounded x86 conditional-return production-side literal-`main`
  caller anchor from both the explicit lowered backend parser and the direct
  LIR parser in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  by switching the family to structural zero-argument `i32` caller matching
  and carrying the observed caller symbol through the emitted assembly instead
  of hardcoding `main`
- proved the lowered/backend-selection seam deletion with a new renamed-caller
  regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the bounded lowered conditional-return family now emits the renamed entry
  symbol on identical x86 assembly output without falling back to backend IR
  text
- kept focused x86 adapter coverage green at `1` passed / `0` failed via
  `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' -j1 --output-on-failure`
- reran the required backend regression scope to `401` passed / `1` failed via
  `ctest --test-dir build -R backend -j --output-on-failure > test_backend_after.log`;
  the only failing case is the independently reproducing unrelated blocker
  `c_testsuite_aarch64_backend_src_00023_c`
- the monotonic backend guard against the checked-in baseline currently reports
  one new failing test because `test_backend_before.log` records an older clean
  `182`-test snapshot while the current tree still has the unrelated aarch64
  failure above

Completed in this slice:

- removed the direct x86 production-side literal-`main` caller anchor from the
  bounded single-function plain return-immediate and goto-only constant-return
  families in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  by teaching the direct LIR parsers to accept any single structural
  zero-argument `i32` definition and carry the observed caller symbol through
  native asm emission instead of hardcoding `main`
- proved the production deletion with new renamed-caller regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so both the plain return-immediate and goto-only constant-return families
  keep the renamed direct-LIR entry surface, the explicit lowered backend
  seam, and normal backend selection on identical x86 assembly output without
  falling back to LLVM text
- kept focused x86 adapter coverage green at `1` passed / `0` failed via
  `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' -j1 --output-on-failure`
- kept full-suite regression coverage monotonic at `2840` passed / `1` failed
  before and after via `test_before.log`, `test_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Completed in this slice:

- removed the first live emitter-local
  `lower_lir_to_backend_module(...)` owner from
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  by routing the explicit LIR entrypoint through direct LIR-native/BIR-native
  fast paths first, then reusing the already-pruned prepared-module fallback
  for the remaining legacy-backed cases
- kept the bounded nonminimal local-array / return-immediate subset on the same
  prepared lowered seam so deleting the eager owner did not regress the
  existing direct-vs-lowered aarch64 parity checks
- proved the dead-alloca-prune route with a new regression in
  [`tests/backend/backend_lir_adapter_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_aarch64_tests.cpp)
  so the global int-pointer round-trip family stays on identical assembly after
  unused allocas are pruned instead of depending on the removed eager lowering
  owner
- kept focused aarch64 adapter coverage green at `1` passed / `0` failed via
  `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' -j1 --output-on-failure`
- kept backend regression coverage monotonic from `182` passed / `0` failed to
  `402` passed / `0` failed via `test_backend_before.log`,
  `test_backend_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

Completed in this slice:

- removed the bounded countdown-while lowered/backend-selection
  production-side literal-`main` caller anchor from
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
  by teaching the structural zero-argument `i32` countdown-loop adapter to
  accept renamed callers instead of rejecting anything not named `main`
- proved the lowered seam deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  so the bounded countdown-while family stays on the explicit loop-carried
  backend path instead of collapsing through the generic `ret i32 0` fallback
- tightened the existing x86 renamed countdown-while regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  to lower the renamed LIR module directly, proving the explicit lowered seam
  now matches the direct LIR asm path without post-lowering signature patching
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R '^backend_lir_adapter_tests$|^backend_lir_adapter_x86_64_tests$' -j1 --output-on-failure`
- kept backend regression coverage monotonic from `182` passed / `0` failed to
  `402` passed / `0` failed via `test_backend_before.log`,
  `test_backend_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

Completed in this slice:

- removed the bounded countdown-do-while lowered/backend-selection
  production-side literal-`main` caller anchor from
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
  by teaching the structural single-local countdown loop adapter to accept any
  zero-argument `i32` definition instead of rejecting renamed callers up front
- proved the lowered seam deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  so the bounded countdown-do-while family keeps renamed callers on the
  explicit lowered backend path and still collapses to the same direct return
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R '^backend_lir_adapter_tests$|^backend_lir_adapter_x86_64_tests$' -j1 --output-on-failure`
- kept backend regression coverage monotonic from a clean `HEAD` baseline of
  `182` passed / `0` failed to the patched tree at `402` passed / `0` failed
  via `test_backend_before.log`, `test_backend_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

- removed the bounded x86 countdown-do-while direct-LIR production-side
  literal-`main` caller anchor from
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  by teaching the direct parser to recognize any structural zero-argument
  `i32` definition and carry the observed caller name through the folded
  return-immediate asm path instead of hardcoding `main`
- proved the production deletion with a renamed-caller regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the bounded countdown-do-while family keeps renamed direct-LIR callers on
  native x86 assembly output without falling back to LLVM text
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic at `402` passed / `0` failed
  before and after via `test_backend_before.log`, `test_backend_after.log`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

- removed the bounded x86 countdown-while production-side literal-`main`
  caller anchor from
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  by teaching both the direct LIR parser and the explicit lowered
  backend-module parser to accept any structural zero-argument `i32`
  definition, then carry the observed function name through native asm
  emission instead of hardcoding `main`
- proved the production deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the bounded countdown-while family keeps the renamed direct-LIR entry
  surface and the explicit lowered backend seam on identical assembly output
  without falling back to LLVM text
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic at `402` passed / `0` failed
  before and after via `test_backend_before.log`, `test_backend_after.log`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

- removed the lowered structured dual-identity direct-call subtraction
  production-side literal-`main` caller anchor from
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  to identify the zero-argument `i32` caller structurally instead of requiring
  a literal `main` symbol for the bounded dual-identity helper family
- proved the shared matcher deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  covering `parse_backend_minimal_dual_identity_direct_call_sub_module(...)`
  on the lowered backend-module seam
- proved the x86 lowered/backend-selection behavior with a new renamed-caller
  regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the bounded dual-identity subtraction slice keeps the renamed caller on
  the same direct-vs-lowered asm path instead of falling back when the caller
  is no longer named `main`
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic from `291` passed / `111` failed
  to `402` passed / `0` failed via `test_backend_before.log`,
  `test_backend_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

Latest completed slice:

- taught the shared structured void direct-call parser in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  to identify the zero-argument `i32` caller structurally instead of requiring
  a literal `main` symbol for the bounded void-helper / immediate-return
  family
- added a shared LIR-native parser for that bounded void-helper slice in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  so the explicit x86 LIR entrypoint can keep the family on the direct asm
  path without routing through `lower_lir_to_backend_module(...)`
- taught the x86 direct LIR emit surface plus the explicit lowered backend seam
  in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to render the bounded void helper body, call it, and materialize the
  caller's immediate return while preserving renamed caller symbols
- proved the shared matcher deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  covering the structured lowered void-helper parser seam
- proved the production deletion with new x86 regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  covering direct-vs-lowered parity for the bounded void-helper family plus
  renamed-caller coverage on both the explicit LIR emit surface and the
  explicit lowered backend emit surface
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic from `291` passed / `111` failed
  to `402` passed / `0` failed via `test_backend_before.log`,
  `test_backend_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

- removed the lowered structured declared-direct-call production-side
  literal-`main` caller anchor from
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  by teaching the shared backend-module matcher to identify the zero-argument
  `i32` caller structurally instead of selecting it by symbol name
- taught the lowered x86 declared-direct-call emitter path in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to publish the parsed caller symbol instead of hardcoding the emitted entry
  symbol to `main`, so renamed lowered callers stay on the native asm path for
  this bounded extern-call family
- proved the shared matcher deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  covering `parse_backend_minimal_declared_direct_call_module(...)` on the
  lowered backend-module seam
- proved the x86 lowered-emitter behavior with a new renamed-caller regression
  in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the bounded lowered declared-direct-call seam emits the observed caller
  symbol and stays on assembly output
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic at `402` passed / `0` failed
  before and after via `test_backend_before.log`, `test_backend_after.log`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

- removed the remaining shared structured single-add-immediate direct-call
  production-side literal-`main` caller anchor from
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  by teaching both the lowered backend-module matcher and the direct LIR
  single-add-immediate matcher to identify the zero-argument caller
  structurally instead of selecting it by symbol name
- taught the lowered x86 single-add-immediate emitter path in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to publish the parsed caller symbol instead of hardcoding the emitted entry
  symbol to `main`, so renamed lowered callers stay on the native asm path for
  this bounded helper family
- proved the shared matcher deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  covering the structured lowered single-add-immediate direct-call slice
- proved the x86 lowered-emitter behavior with a new renamed-caller regression
  in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the bounded lowered single-add-immediate seam emits the observed caller
  symbol and stays on assembly output
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic at `402` passed / `0` failed
  before and after via `test_backend_before.log`, `test_backend_after.log`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_backend_before.log --after test_backend_after.log --allow-non-decreasing-passed`

- removed the shared declared-direct-call LIR production-side `main` anchor
  from
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  by teaching the shared parser to identify the zero-argument `i32` caller
  structurally instead of requiring a literal `main` definition for that seam
- taught the direct x86 declared-direct-call LIR emitter in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to publish the parsed caller symbol instead of hardcoding the emitted entry
  symbol to `main`, so renamed callers stay on the native asm path
- proved the shared parser deletion with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  that exercises `parse_backend_minimal_declared_direct_call_lir_module(...)`
  directly
- proved the x86 emitter behavior with a new renamed-caller regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  so the explicit x86 LIR emit surface publishes the observed caller symbol
  and stays on assembly output for the bounded declared-direct-call family
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept full-suite regression monotonic at `2841` passed / `0` failed before
  and after via `test_before.log`, `test_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Completed in this slice:

- taught the shared direct-call LIR parsers in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  to identify zero-argument `i32` callers structurally for the bounded
  identity, two-arg, folded-two-arg, and dual-identity helper families
  instead of requiring a literal `main` symbol
- taught the x86 direct-LIR emit helpers in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to publish the parsed caller symbol for those bounded helper families,
  including the folded-two-arg return-immediate fast path, so renamed callers
  stay on the native asm path without routing through legacy backend IR
- proved the shared parser change with a renamed-caller regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  covering the bounded two-arg direct-call parser path
- proved the x86 explicit-LIR behavior with renamed-caller regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  covering the identity, two-arg, folded-two-arg, and dual-identity helper
  families so the native entry surface no longer assumes a literal `main`
- kept focused adapter coverage green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- kept backend regression coverage monotonic at `402` passed / `0` failed
  before and after via `test_before.log`, `test_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

- removed the shared call-crossing direct-call production-side `main` anchor
  from
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  by teaching the shared parser to identify the zero-argument `i32` caller
  structurally for that seam instead of rejecting renamed callers up front
- taught the x86 shared call-crossing direct-call direct-LIR seam in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to carry the parsed caller symbol through emission and synthesize the
  preserved-source regalloc from backend-owned call-crossing metadata instead
  of rescanning the LIR module for a literal `main`
- taught the aarch64 shared call-crossing direct-call backend seam in
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  to emit the parsed caller symbol and use backend-owned synthesized
  call-crossing regalloc for the lowered seam instead of hardcoding `main` in
  the emitter path
- proved the deletion with new renamed-caller regressions in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp),
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp),
  and
  [`tests/backend/backend_lir_adapter_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_aarch64_tests.cpp)
  so the shared parser plus both backend seams stay on the asm path without a
  production-side `main` fixture anchor for this bounded family

- removed the emitter-local `find_lir_function(...)` helper from both
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  and
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
  by inlining the bounded `main` discovery at the remaining direct-emitter
  call sites, so this seam no longer carries a reusable name-based helper that
  encourages special-casing by symbol lookup
- taught the direct x86 member-array runtime parser in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to discover the helper from the observed non-`main` definition and key both
  the helper signature match and `main` call target from that live symbol
  instead of hardcoding `get_second`, removing the renamed by-value
  member-array runtime dependency on `lower_lir_to_backend_module(...)` from
  the explicit x86 LIR entrypoint
- proved the production deletion with a new x86 regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  that keeps a renamed by-value member-array helper/runtime slice on the
  direct asm path and aligned with backend selection
- added an explicit-LIR parity regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  for the spacing-tolerant rewritten first-local two-argument helper family so
  that already-direct seam stays pinned to identical direct-vs-lowered asm

- added a shared LIR-side parser for the bounded folded two-argument direct-call
  helper family in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  so the direct x86 entry surface can recognize the constant-plus-left-minus-right
  helper shape without lowering through legacy backend IR first
- taught the direct x86 LIR emit surface and the lowered backend-module x86 seam
  in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to fold that bounded helper family directly to the final return-immediate asm
  path, removing another live x86-local `lower_lir_to_backend_module(...)`
  dependency for explicit LIR emission
- proved the shared parser with a new regression in
  [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
  covering the renamed folded helper/call shape on the LIR side
- proved the production deletion with new x86 regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  covering the structured lowered seam plus direct/lowered/backend-selection
  asm-path coverage for the folded two-argument slice

- deleted the bespoke x86-local single-argument direct-call parser/emitter
  branch in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so the direct x86 LIR entrypoint now relies on the shared
  `parse_backend_minimal_direct_call_add_imm_lir_module(...)` path for that
  bounded helper family instead of keeping a duplicate target-local decoder
- proved the slot-backed helper family with a new explicit-LIR parity
  regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  that keeps the direct x86 entry surface, the explicit lowered seam, and the
  backend-selection path on identical asm for the bounded param-slot runtime
  slice
- added a direct x86 LIR parser/emitter path for the bounded dual-identity
  direct-call subtraction family in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so the explicit x86 LIR entrypoint no longer needs
  `lower_lir_to_backend_module(...)` for that slice before staying on the asm
  path
- added a shared LIR-side decoder for the bounded dual-identity direct-call
  subtraction family in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  so the x86 direct-entry seam can recognize the two identity helpers, paired
  immediate calls, and final subtraction without backend-IR adaptation first
- taught the lowered x86 backend-module seam in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to render the same bounded dual-identity direct-call subtraction family so
  the direct and lowered x86 seams stay on identical asm for the slice
- proved the production deletion with new x86 regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  covering both backend selection and explicit direct-vs-lowered parity for
  the bounded dual-identity direct-call subtraction slice
- removed the remaining x86 member-array runtime production-side `main` anchor
  from
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  by teaching the direct parser to identify the bounded zero-argument caller
  structurally instead of requiring `function.name == "main"` for the
  by-value, nested by-value, and nested member-pointer runtime variants
- taught the x86 member-array runtime emitter in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  to carry the observed caller symbol through the bounded asm slice instead of
  hardcoding the emitted entry symbol to `main`, so renamed caller fixtures
  stay on the direct asm path without routing through legacy backend IR
- proved the production deletion with new renamed-caller regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  covering the nested by-value and nested member-pointer member-array runtime
  families on both backend selection and the explicit x86 LIR emit surface
- wired the existing plain two-argument direct-call LIR parser into the x86
  explicit emit surface in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so that bounded register-only two-argument helper calls no longer need
  `lower_lir_to_backend_module(...)` on the direct x86 entrypoint
- proved the production deletion with a new explicit-LIR parity regression in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  that keeps the direct x86 LIR emit surface, the structured lowered seam,
  and normal backend selection on identical asm for the bounded plain
  two-argument helper slice
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
- added a shared LIR-native identity-helper parser in
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
  plus matching x86 emitter hooks in
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
  so the bounded single-argument identity direct-call family no longer needs
  `lower_lir_to_backend_module(...)` on the explicit x86 entrypoint
- proved the production deletion with new x86 regressions in
  [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
  covering both backend selection and the explicit x86 LIR emit surface for
  the bounded identity direct-call slice

Next intended slice:

- resume the next emitter-local `lower_lir_to_backend_module(...)` deletion now
  that the lowered declared-direct-call caller-anchor cleanup is complete
- keep deleting emitter-local legacy conveniences that encode symbol-specific
  assumptions when a bounded structural match is enough for the live slice
- target another renamed or reordered x86-local helper/runtime family that
  still falls through to legacy lowering or hardcodes `main` on a lowered
  backend-module seam
- prefer another bounded single-helper x86 helper/runtime family first so the
  next batch can reuse the same helper/caller structural matcher shape without
  widening scope
- target another bounded x86-local helper/runtime family that still hits
  `lower_lir_to_backend_module(...)` on the explicit LIR entrypoint now that
  the folded two-argument helper seam is gone
- target another explicit x86 LIR helper/runtime family whose direct parser or
  emitter still relies on symbol-specific fixture assumptions beyond the
  member-array runtime slice, and delete that production anchor together with
  matching direct-vs-lowered regressions
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
- tighten another explicit x86 LIR-entry seam that still depends on
  `lower_lir_to_backend_module(...)` after the shared call-crossing caller
  anchor removal, preferably a bounded local-runtime helper family that can be
  deleted together with its matching compatibility-only test ownership

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
- this slice keeps the serial backend scope monotonic at `402` passed / `0`
  failed via `ctest --test-dir build -R backend -j1 --output-on-failure`
- latest Step 4 follow-through also removes the renamed by-value member-array
  runtime dependency on `lower_lir_to_backend_module(...)` from the direct x86
  LIR entrypoint by keying the helper/runtime parse off the observed live
  symbol instead of the hardcoded `get_second` name
- focused adapter coverage is green at `2` passed / `0` failed via
  `ctest --test-dir build -R 'backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests' -j1 --output-on-failure`
- full-suite regression guard is green at `2841` passed / `0` failed before and
  after via `test_fail_before.log`, `test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
- latest Step 4 follow-through also removes the bounded local-slot arithmetic /
  two-local scalar-slot dependency on `lower_lir_to_backend_module(...)` from
  the direct x86 LIR entrypoint
- latest Step 4 follow-through also removes the bounded plain two-argument
  direct-call dependency on `lower_lir_to_backend_module(...)` from the direct
  x86 LIR entrypoint and keeps the direct/lowered/backend-selection seams on
  identical asm
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
- latest Step 4 follow-through also removes the bounded dual-identity
  direct-call subtraction dependency on `lower_lir_to_backend_module(...)`
  from the direct x86 LIR entrypoint and keeps the direct/lowered/backend-
  selection seams on identical asm
- latest Step 4 follow-through also removes the bounded folded two-argument
  direct-call dependency on `lower_lir_to_backend_module(...)` from the direct
  x86 LIR entrypoint and keeps the direct/lowered/backend-selection seams on
  the folded return-immediate asm path
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
