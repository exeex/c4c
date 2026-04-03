Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, continue auditing the bounded straight-line
single-block/default-route surface for the next missing direct-BIR coverage
slice now that the constant-only integer `sub` route case is covered end to
end.
Next target: identify the next emitter-facing single-block opcode or tiny
scaffold that still lacks source-level/default-route BIR coverage, keeping the
work scoped to direct BIR text assertions without widening general CFG support
or direct-BIR emitter behavior.

Completed this iteration:
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_sub.c`, proving `return 3 - 3;`
  defaults to the BIR pipeline instead of falling back to legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_return_sub_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `%t0 = bir.sub i32 3, 3` and forbids legacy LLVM IR `define i32 @main()`
  output for that route surface.
- Reconfigured and rebuilt the tree, reran the new route case
  `backend_codegen_route_riscv64_return_sub_defaults_to_bir`, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, then
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `329/329` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2760 -> 2767` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_mul.c`, proving
  `return 6 * 7;` defaults to the BIR pipeline instead of falling back to
  legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_return_mul_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `%t0 = bir.mul i32 6, 7` and forbids legacy LLVM IR `define i32 @main()`
  output for that route surface.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_mul_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `328/328` backend-labeled tests passing.

Completed this iteration:
- Widened the bounded straight-line BIR scaffold with the missing constant-only
  integer `ashr` slice by adding `bir.ashr` support in `bir.hpp`, `bir.cpp`,
  and `lir_to_bir.cpp`, keeping it bounded to immediate integer arithmetic that
  still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_ashr_module()`, proving the widened shift slice reaches
  the BIR text surface as `%t0 = bir.ashr i32 -16, 2`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_ashr.c`, proving signed right
  shift defaults to the BIR pipeline instead of falling back to legacy LLVM IR
  text. The frontend currently materializes the negative literal as
  `bir.sub i32 0, 16` before the `bir.ashr`, so the route assertion records
  that exact BIR shape.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_ashr_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `327/327` backend-labeled tests passing.
- Refreshed `test_before.log` and `test_after.log` with full
  `ctest --test-dir build -j8 --output-on-failure` runs, then passed the
  regression guard with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2764 -> 2765` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases). The first concurrent after-run produced transient failures
  while other binaries were still rebuilding; a clean rerun from the settled
  build was green and is the recorded result.

Completed this iteration:
- Widened the bounded straight-line BIR scaffold with the missing constant-only
  integer `lshr` slice by adding `bir.lshr` support in `bir.hpp`, `bir.cpp`,
  and `lir_to_bir.cpp`, keeping it bounded to immediate integer arithmetic
  that still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_lshr_module()`, proving the widened shift slice reaches
  the BIR text surface as `%t0 = bir.lshr i32 16, 2`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_lshr.c`, proving
  `return (unsigned)16 >> 2;` defaults to the BIR pipeline instead of falling
  back to legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_lshr_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `326/326` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2760 -> 2764` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Widened the bounded straight-line BIR scaffold with the missing constant-only
  integer `shl` slice by adding `bir.shl` support in `bir.hpp`, `bir.cpp`, and
  `lir_to_bir.cpp`, keeping it bounded to immediate integer arithmetic that
  still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_shl_module()`, proving the widened shift slice reaches
  the BIR text surface as `%t0 = bir.shl i32 3, 4`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_shl.c`, proving
  `return 3 << 4;` defaults to the BIR pipeline instead of falling back to
  legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_shl_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `325/325` backend-labeled tests passing.
- Refreshed `test_before.log` and `test_after.log` with full
  `ctest --test-dir build -j8 --output-on-failure` runs and preserved monotonic
  full-suite results (`2762 -> 2763` passed, `0 -> 0` failed, no newly failing
  tests).

Completed this iteration:
- Widened the bounded straight-line BIR scaffold with the missing constant-only
  integer `xor` slice by adding `bir.xor` support in `bir.hpp`, `bir.cpp`, and
  `lir_to_bir.cpp`, keeping it bounded to immediate integer arithmetic that
  still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_xor_module()`, proving the widened bitwise slice reaches
  the BIR text surface as `%t0 = bir.xor i32 12, 10`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_xor.c`, proving
  `return 12 ^ 10;` defaults to the BIR pipeline instead of falling back to
  legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_xor_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `324/324` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2760 -> 2762` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Widened the bounded straight-line BIR scaffold with the missing constant-only
  integer `or` slice by adding `bir.or` support in `bir.hpp`, `bir.cpp`, and
  `lir_to_bir.cpp`, keeping it bounded to immediate integer arithmetic that
  still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_or_module()`, proving the widened bitwise slice reaches
  the BIR text surface as `%t0 = bir.or i32 12, 3`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_or.c`, proving
  `return 12 | 3;` defaults to the BIR pipeline instead of falling back to
  legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_or_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `323/323` backend-labeled tests passing.
- Refreshed `test_fail_before.log` and `test_fail_after.log` with full
  `ctest --test-dir build -j8 --output-on-failure` runs, then passed the
  regression guard with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2760 -> 2761` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Widened the bounded straight-line BIR scaffold with the missing constant-only
  integer `and` slice by adding `bir.and` support in `bir.hpp`, `bir.cpp`, and
  `lir_to_bir.cpp`, keeping it bounded to immediate integer arithmetic that
  still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_and_module()`, proving the widened bitwise slice reaches
  the BIR text surface as `%t0 = bir.and i32 14, 11`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_and.c`, proving
  `return 14 & 11;` defaults to the BIR pipeline instead of falling back to
  legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_and_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `322/322` backend-labeled tests passing.
- Recorded a fresh full-suite baseline in `test_before.log` (`2758/2759`
  passed with one pre-existing timeout in `cpp_eastl_utility_parse_recipe`),
  refreshed `test_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2758 -> 2760` passed, `1 -> 0` failed, no newly failing tests, no new
  `>30s` cases). The previously timing-out suite case
  `cpp_eastl_utility_parse_recipe` passed in `18.50 sec`.

Completed this iteration:
- Widened the bounded BIR straight-line arithmetic scaffold with the missing
  constant-only unsigned division slice by adding `bir.udiv` support in
  `bir.hpp`, `bir.cpp`, and `lir_to_bir.cpp`, keeping it bounded to immediate
  integer arithmetic that still linearizes into one BIR block.
- Added backend BIR printer, lowering, and explicit BIR-pipeline regressions
  via `make_bir_return_udiv_module()`, proving the widened unsigned-division
  slice reaches the BIR text surface as `%t0 = bir.udiv i32 12, 3`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_udiv.c`, proving `return 12u / 3u;`
  defaults to the BIR pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_return_udiv_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `321/321` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2743 -> 2759` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases). The slowest observed suite case was
  `cpp_eastl_utility_parse_recipe` at `19.79 sec`, below the timeout
  threshold.

Completed this iteration:
- Added the missing simple split-predecessor compare-fed phi-join `+ 6 - 2 + 9`
  parity slice via
  `make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_add_module()`,
  proving the existing `split_predecessor_add_phi` `+ 6` and `+ 6 - 2` forms
  stay on the direct BIR path when the join-local tail extends to
  `+ 6 - 2 + 9`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_add_phi_post_add_sub_add.c`,
  proving `(x == y ? x + 5 : y + 9) + 6 - 2 + 9` defaults to the BIR pipeline
  instead of falling back to legacy LLVM IR text.
- Added explicit lowering and direct-BIR pipeline coverage for the simple
  split-predecessor `post_add_sub_add` slice, completing the nearby join-tail
  parity matrix for the base `add_phi` family.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_add_phi_post_add_sub_add_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `320/320` backend-labeled tests passing.
- The simple-family tail-extension slice passed without any `lir_to_bir.cpp`
  widening, confirming the existing bounded compare-fed phi-join matcher
  already generalizes across the full nearby `split_predecessor_add_phi`
  `+ 6`, `+ 6 - 2`, and `+ 6 - 2 + 9` join-tail variants and only regression
  coverage was missing.

Completed this iteration:
- Added the missing simple split-predecessor compare-fed phi-join `+ 6 - 2`
  parity slice via
  `make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_module()`,
  proving the existing `split_predecessor_add_phi` `+ 6` form stays on the
  direct BIR path when the join-local tail extends to `+ 6 - 2`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_add_phi_post_add_sub.c`,
  proving `(x == y ? x + 5 : y + 9) + 6 - 2` defaults to the BIR pipeline
  instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_add_phi_post_add_sub_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `319/319` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2743 -> 2757` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases). The slowest observed suite case was
  `cpp_eastl_utility_parse_recipe` at `19.39 sec`, below the timeout
  threshold.
- The simple-family parity slice passed without any `lir_to_bir.cpp`
  widening, confirming the existing bounded compare-fed phi-join matcher
  already generalizes across the `split_predecessor_add_phi` `+ 6` and
  `+ 6 - 2` join-tail variants and only regression coverage was missing.

Completed this iteration:
- Added the missing non-deeper split-predecessor compare-fed phi-join
  `+ 6 - 2 + 9` parity slice via
  `make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module()`,
  proving the already-covered `mixed_affine` `+ 6` and `+ 6 - 2` forms stay on
  the direct BIR path when the join-local tail extends to `+ 6 - 2 + 9`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_add.c`,
  proving `(x == y ? x + 8 - 3 : y + 11 - 4) + 6 - 2 + 9` defaults to the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_add_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `318/318` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2743 -> 2756` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).
- The non-deeper extended-tail slice passed without any `lir_to_bir.cpp`
  widening, confirming the existing bounded compare-fed phi-join matcher
  already generalizes across the full nearby `mixed_affine` `+ 6`,
  `+ 6 - 2`, and `+ 6 - 2 + 9` join-tail variants and only regression
  coverage was missing.

Completed this iteration:
- Added the remaining asymmetric deeper-then / mixed-else split-predecessor
  compare-fed phi-join `+ 6 - 2 + 9` tail slice via
  `make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module()`,
  proving the already-covered `+ 6 - 2` form stays on the direct BIR path when
  the join-local tail extends to `+ 6 - 2 + 9`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add.c`,
  proving `(x == y ? x + 8 - 3 + 5 : y + 11 - 4) + 6 - 2 + 9` defaults to the
  BIR pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `317/317` backend-labeled tests passing.
- The extended asymmetric slice passed without any `lir_to_bir.cpp` widening,
  confirming the existing bounded compare-fed phi-join matcher already
  generalizes across the full nearby `+ 6`, `+ 6 - 2`, and `+ 6 - 2 + 9`
  branch-depth variants and only regression coverage was missing.

Completed this iteration:
- Added the mirrored asymmetric deeper-else split-predecessor `+ 6 - 2 + 9`
  tail slice via
  `make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module()`,
  proving the already-covered `mixed_then_deeper` `+ 6 - 2` form stays on the
  direct BIR path when the join-local tail extends to `+ 6 - 2 + 9`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add.c`,
  proving `(x == y ? x + 8 - 3 : y + 11 - 4 + 7) + 6 - 2 + 9` defaults to the
  BIR pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `316/316` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2743 -> 2754` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases). The slowest observed suite case was
  `cpp_eastl_utility_parse_recipe` at `20.38 sec`, still below the timeout
  threshold.
- The mirrored extended-tail slice passed without any `lir_to_bir.cpp`
  widening, confirming the existing bounded compare-fed phi-join matcher
  already generalizes to this deeper-else asymmetric join-tail extension and
  only regression coverage was missing.

Completed this iteration:
- Added the next join-local arithmetic extension on the symmetric deeper
  split-predecessor compare-fed phi-join path via
  `make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module()`,
  proving the already-covered `+ 6 - 2` tail also stays on the direct BIR path
  when extended to `+ 6 - 2 + 9`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add.c`,
  proving `(x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7) + 6 - 2 + 9` defaults to
  the BIR pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `315/315` backend-labeled tests passing.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2753` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases). The slowest observed suite case
  was `cpp_eastl_utility_parse_recipe` at `20.07 sec`, below the existing
  timeout threshold.
- The extended tail slice passed without any `lir_to_bir.cpp` widening,
  confirming the existing join-block add/sub loop already preserves a longer
  bounded post-select affine chain and only regression coverage was missing.

Completed this iteration:
- Added the missing mirrored asymmetric split-predecessor compare-fed phi-join
  post-add parity slice via
  `make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module()`,
  proving the already-covered deeper-then / mixed-else post-add form now has
  its mixed-then / deeper-else branch-depth mirror on the direct BIR path too.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add.c`,
  proving `(x == y ? x + 8 - 3 : y + 11 - 4 + 7) + 6` defaults to the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8`.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2752` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases). One transient
  `cpp_typedef_owned_functional_cast_perf` timeout from the first full-suite
  pass did not reproduce on isolated rerun or on the guard-passing rerun.
- The mirrored post-add slice passed without any `lir_to_bir.cpp` widening,
  confirming the existing bounded matcher was already symmetric across then/else
  branch-depth assignment for both the post-add and post-add/sub asymmetric
  compare-fed phi-join forms.

Completed this iteration:
- Added the mirrored asymmetric split-predecessor compare-fed phi-join
  regression slice via
  `make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module()`,
  proving the already-covered deeper-then / mixed-else bounded ternary is now
  matched by the mixed-then / deeper-else branch-depth mirror on the direct BIR
  path.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub.c`,
  proving `(x == y ? x + 8 - 3 : y + 11 - 4 + 7) + 6 - 2` defaults to the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8`.
- Refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run, then passed the
  regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2751` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).
- The mirror slice passed without any `lir_to_bir.cpp` widening, confirming the
  bounded compare-fed phi-join matcher is already symmetric across then/else
  branch-depth assignment for this join-local add/sub form.

Completed this iteration:
- Added the next asymmetric split-predecessor compare-fed phi-join regression
  slice via
  `make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_module()`,
  proving the already-covered deeper-then / mixed-else bounded ternary now also
  stays on the BIR path when the join-local tail extends from `+ 6` to
  `+ 6 - 2`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub.c`,
  proving `(x == y ? x + 8 - 3 + 5 : y + 11 - 4) + 6 - 2` defaults to the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the affected tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8`.
- The new slice passed without any `lir_to_bir.cpp` widening, confirming the
  existing bounded matcher already generalizes to this asymmetric join-tail
  variant; backend validation stayed green at `312/312` tests passed.

Completed this iteration:
- Added direct BIR lowering and explicit BIR-pipeline regressions for the next
  bounded mixed split-predecessor compare-fed phi-join slice with a richer
  join-local add/sub tail via
  `make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_module()`,
  proving the fused `bir.select` can now be regression-checked with a
  follow-on `add` plus `sub` chain instead of only a single post-join add for
  the mixed-affine arm shape.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_mixed_affine_post_add_sub.c`,
  proving `(x == y ? x + 8 - 3 : y + 11 - 4) + 6 - 2` stays on the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the tree so the new backend BIR support, explicit
  BIR pipeline checks, and source-route coverage can be exercised together for
  this bounded Step 2 slice.
- Reran `ctest --test-dir build -R backend_bir_tests --output-on-failure`,
  reran the new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8`, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j8
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2749` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Added direct BIR lowering and explicit BIR-pipeline regressions for the next
  bounded join-local arithmetic slice after the already-covered symmetric
  deeper split-predecessor compare-fed phi join via
  `make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_module()`,
  proving the fused `bir.select` can now be regression-checked with a
  follow-on `add` plus `sub` chain instead of only a single post-join add.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_deeper_affine_post_add_sub.c`,
  proving `(x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7) + 6 - 2` stays on the
  BIR pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the tree so the new backend BIR support, explicit
  BIR pipeline checks, and source-route coverage can be exercised together for
  this bounded Step 2 slice.
- Reran `ctest --test-dir build -R backend_bir_tests --output-on-failure`,
  reran the new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8`, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j8
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2748` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Added the missing backend BIR lowering assertions for the already-covered
  asymmetric richer split-predecessor slice via
  `make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module()`,
  so the deeper-then / mixed-else case is now checked consistently across
  lowering, explicit BIR routing, and source-driven backend routing.
- Added the next symmetric deeper-chain split-predecessor parity slice via
  `make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_module()`
  plus the source-level/default-route case
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_deeper_affine_post_add.c`,
  proving `(x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7) + 6` stays on the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Reconfigured and rebuilt the tree, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, and reran
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_defaults_to_bir`;
  both passed without requiring any `lir_to_bir.cpp` widening, confirming the
  bounded split-predecessor matcher was already general enough for the
  symmetric deeper-arm case.
- Reran `ctest --test-dir build -L backend --output-on-failure -j8` with all
  `309/309` backend-labeled tests passing, refreshed `test_fail_after.log`
  with a full `ctest --test-dir build -j8 --output-on-failure` run, then
  passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2747` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Added direct BIR lowering and explicit BIR-pipeline regressions for the next
  richer split-predecessor compare-fed phi-join slice where the then arm now
  carries a three-op add/sub affine chain while the else arm keeps the prior
  two-op mixed affine chain before empty end blocks and the join still keeps a
  post-add, via
  `make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module()`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add.c`,
  proving `(x == y ? x + 8 - 3 + 5 : y + 11 - 4) + 6` stays on the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Rebuilt `backend_bir_tests` and `c4cll`, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8`, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j8
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2746` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Added direct BIR lowering and explicit BIR-pipeline regressions for the next
  split-predecessor compare-fed phi-join slice where both arms carry bounded
  add/sub affine chains before empty end blocks and the join keeps a post-add,
  via
  `make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_module()`.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_split_predecessor_mixed_affine_post_add.c`,
  proving `(x == y ? x + 8 - 3 : y + 11 - 4) + 6` now stays on the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Rebuilt `backend_bir_tests` and `c4cll`, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_affine_post_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8`, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j8
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2745` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Widened `lir_to_bir.cpp` so the bounded compare-fed phi-join matcher now
  accepts the split-predecessor six-block ternary shape where each arm computes
  an add/sub chain before an otherwise-empty end block that is still the direct
  incoming `phi` predecessor.
- Added direct BIR lowering and explicit BIR-pipeline regressions for that
  split-predecessor join-local arithmetic slice via
  `make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_module()`,
  proving `%t8 = bir.add ...`, `%t9 = bir.add ...`,
  `%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9`, `%t11 = bir.add i32 %t10, 6`,
  and the final `bir.ret i32 %t11` stay on the BIR text path.
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/two_param_select_eq_predecessor_add_post_add.c`,
  proving the frontend-produced six-block ternary now auto-selects into the BIR
  pipeline instead of falling back to legacy LLVM IR text.
- Rebuilt `backend_bir_tests` and `c4cll`, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran the
  new route case
  `backend_codegen_route_riscv64_two_param_select_eq_predecessor_add_post_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8`, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j8
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2744` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Widened `lir_to_bir.cpp` so bounded compare-fed `phi` joins can keep a
  join-local add/sub chain after the fused `bir.select`, instead of requiring
  the collapsed select result to return immediately.
- Added direct BIR lowering and explicit BIR-pipeline regressions for the
  mixed predecessor/immediate post-join arithmetic slice via
  `make_bir_mixed_predecessor_add_phi_post_join_add_module()`, proving
  `%t4 = bir.select slt i32 2, 3, %t3, 9`, `%t5 = bir.add i32 %t4, 6`, and the
  final `bir.ret i32 %t5` stay on the BIR text path.
- Rebuilt `backend_bir_tests`, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, reran
  `ctest --test-dir build -L backend --output-on-failure -j8`, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2743` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Widened the bounded compare-fed phi-join BIR matcher so two-parameter
  predecessor-local add/sub arms can be hoisted into the single BIR block
  before the final `bir.select`, instead of requiring empty predecessor arms.
- Added direct BIR lowering and explicit BIR-pipeline regressions for the
  bounded predecessor-compute ternary slice via
  `make_bir_two_param_select_eq_predecessor_add_phi_module()`, proving
  `%t3 = bir.add ...`, `%t4 = bir.add ...`, and the final
  `bir.select eq i32 %p.x, %p.y, %t3, %t4` stay on the BIR text path.
- Rebuilt the tree, reran `ctest --test-dir build -R backend_bir_tests
  --output-on-failure`, reran `ctest --test-dir build -L backend
  --output-on-failure -j8`, then refreshed `test_fail_after.log` with a full
  `ctest --test-dir build -j8 --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2743 -> 2743` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Added bounded Step 2 regression coverage for the already-supported
  two-parameter compare-fed ternary slice (`x == y ? x : y`) across backend
  BIR lowering, explicit BIR-pipeline text output, and a new RISC-V default
  route case via `tests/c/internal/backend_route_case/two_param_select_eq.c`.
- Added `make_bir_two_param_select_eq_phi_module()` so the backend BIR tests
  exercise the source-shaped six-block phi-join form that collapses into
  `bir.select eq i32 %p.x, %p.y, %p.x, %p.y`.
- Reconfigured and rebuilt `backend_bir_tests`, reran the targeted new checks,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  all `305/305` backend-labeled tests passing.

Completed this iteration:
- Widened the bounded BIR control-flow scaffold from straight-line compare-fed
  `select` materialization to the smallest source-shaped ternary forms by
  collapsing compare-driven branch-to-return and empty branch-only goto plus
  `phi` join patterns into a single-block `bir.select` on the BIR text path.
- Taught `lir_to_bir.cpp` to recognize the bounded one-parameter
  `icmp`/`zext`/`icmp ne 0` conditional branch surface in both the direct
  branch-return and empty goto-chain-plus-`phi` variants, reusing the existing
  BIR `select` shape without introducing general BIR CFG support.
- Added backend lowering and explicit-BIR pipeline regressions for both bounded
  ternary variants plus a new source-level RISC-V default-route regression via
  `tests/c/internal/backend_route_case/single_param_select_eq.c`.
- Reconfigured and rebuilt the tree, reran the focused backend BIR and route
  tests, reran `ctest --test-dir build -L backend --output-on-failure -j8`
  with all `304/304` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2742` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).

Completed this iteration:
- Added a fused `bir.select` instruction shape to the bounded BIR surface, with
  printer and validator coverage for straight-line integer compare-fed select
  materialization on the single-block text path.
- Taught `lir_to_bir.cpp` to lower bounded integer `icmp` + `select` + `ret`
  patterns into the new BIR select instruction and to keep the existing affine
  bookkeeping valid by evaluating only constant compare predicates for this
  slice.
- Updated the direct-BIR affine parsers in the x86 and AArch64 emitters to
  treat non-binary BIR instructions as unsupported rather than assuming every
  BIR instruction is a binary op.
- Added backend BIR lowering and explicit-BIR pipeline regressions for the new
  select slice via `make_bir_return_select_eq_module()`, covering printer,
  validator, lowering, and explicit RISC-V BIR-pipeline text output.
- Rebuilt `backend_bir_tests` and reran
  `ctest --test-dir build -L backend --output-on-failure -j8`; all `303/303`
  backend-labeled tests passed.

Completed this iteration:
- Widened the bounded BIR compare scaffold with the last constant-only
  inequality materialization slice by folding `icmp ne` plus
  `zext i1 -> i32` return patterns into `bir.ne`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7 != 3;`.
- Reconfigured and rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_ne_defaults_to_bir` coverage, then
  reran `ctest --test-dir build -L backend --output-on-failure -j8`; all
  `303/303` backend-labeled tests passed.
- Widened the bounded BIR compare scaffold with a constant-only unsigned
  greater-than-or-equal materialization slice by folding `icmp uge` plus
  `zext i1 -> i32` return patterns into `bir.uge`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7u >= 7u;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_uge_defaults_to_bir` coverage, then
  reran `ctest --test-dir build -L backend --output-on-failure -j8`; all
  `302/302` backend-labeled tests passed.
- Refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728 -> 2740` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).
- Widened the bounded BIR compare scaffold with a constant-only unsigned
  greater-than materialization slice by folding `icmp ugt` plus
  `zext i1 -> i32` return patterns into `bir.ugt`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7u > 3u;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_ugt_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2739` passed, `0 -> 0` failed, no
  newly failing tests, no new `>30s` cases).
- Refactored internal backend test registration by introducing
  `tests/c/internal/cmake/BackendTests.cmake`, centralizing the shared
  `internal;backend` labels and the repeated backend codegen-route CMake
  wiring into helper functions.
- Added a dedicated `backend_route` label on the RISC-V backend codegen-route
  regression cases so they can be selected independently while still
  remaining part of the broader `backend` subset.
- Reconfigured the tree and reran `ctest --test-dir build -L backend
  --output-on-failure -j8`; all `300/300` backend-labeled tests passed after
  the CMake test-registration refactor.
- Audited the current production legacy boundaries in `backend.cpp`,
  `backend.hpp`, `llvm_codegen.cpp`, and `c4cll.cpp`.
- Recorded the concrete removal checklist and test owners in
  `ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md`.
- Rebuilt and revalidated the full suite after this slice, recording the
  green run in `test_after.log` (`2728/2728` passing).
- Replaced the inline `BackendModuleInput` ownership model with an out-of-line
  wrapper that can own either legacy `BackendModule` or BIR `bir::Module`
  inputs without forcing `backend.hpp` to include `ir.hpp`.
- Removed `../../ir.hpp` from `src/backend/x86/codegen/emit.hpp` and
  `src/backend/aarch64/codegen/emit.hpp` by forward-declaring
  `BackendModule` at the emitter-header boundary.
- Added a direct-BIR backend pipeline test plus a compile-time header-boundary
  regression.
- Moved the direct-BIR conversion seam out of `backend.cpp` and into the
  x86/aarch64 emitter layer by adding direct `bir::Module` emitter entry
  points and routing backend BIR execution through them.
- Added end-to-end x86/aarch64 regression coverage proving direct
  `BackendModuleInput{bir::Module}` callers still reach backend assembly
  emission without pre-lowering to `BackendModule`.
- Replaced the x86/aarch64 direct `bir::Module` arithmetic-entry fallback with
  native emitter-side handling for return-immediate, return-add/sub-immediate,
  and bounded affine return slices, while keeping `lower_bir_to_backend_module`
  only as the unsupported direct-BIR fallback.
- Added direct-BIR staged-affine regression coverage for both x86 and AArch64
  emitter entry points.
- Extended direct `bir::Module` affine parsing in the x86 and AArch64 emitters
  to accept zero-parameter staged constant chains, so multi-instruction direct
  BIR constant-return slices now emit natively instead of falling back through
  `bir_to_backend_ir.*`.
- Added BIR text-surface coverage plus direct-BIR x86/AArch64 end-to-end
  regressions for the zero-parameter staged constant chain slice.
- Revalidated the full suite after the zero-parameter staged-constant direct
  BIR slice with `test_after.log` (`2728/2728` passing) and confirmed
  monotonic no-regression behavior against `test_before.log`.
- Removed the x86/AArch64 direct-`bir::Module` rescue path through
  `bir_to_backend_ir.*`; unsupported manual direct-BIR input now fails
  explicitly instead of silently converting back into legacy backend IR.
- Deleted `src/backend/lowering/bir_to_backend_ir.cpp` and
  `src/backend/lowering/bir_to_backend_ir.hpp`, removed their build wiring, and
  switched the remaining bridge-style backend tests to
  `lower_lir_to_backend_module(...)`.
- Added direct-BIR regression coverage proving unsupported multi-function BIR
  modules now throw explicit x86/AArch64 backend errors rather than re-entering
  the legacy backend-IR route.
- Rebuilt from a clean `build/` directory and reran the full suite into
  `test_fail_after.log`; the regression guard passed against
  `test_fail_before.log` with `after: 2728 passed, 0 failed` and no newly
  failing tests.
- Expanded the BIR scaffold with `i8` scalar support in `bir.hpp`,
  `bir.cpp`, and `lir_to_bir.cpp`, including typed immediates, printer output,
  validator acceptance, and bounded `i8` add/sub lowering coverage.
- Added targeted BIR lowering and pipeline regressions proving the widened
  `i8` slice reaches the RISC-V BIR text surface without re-entering the
  legacy backend-IR route.
- Widened `lir_to_bir.cpp` to accept `i64` scalar signatures and arithmetic
  text operands in addition to the existing `i8`/`i32` surface, and updated
  the scaffold failure message to reflect the new bounded type coverage.
- Added `i64` BIR printer, validator, lowering, and RISC-V pipeline
  regressions so the widened word-sized arithmetic slice is covered on the BIR
  text path without implying new x86/AArch64 direct-emitter support yet.
- Rebuilt the tree, reran focused backend BIR validation, and refreshed
  `test_fail_after.log`; the regression guard passed against
  `test_fail_before.log` with `after: 2728 passed, 0 failed` and no newly
  failing tests.
- Widened the BIR straight-line arithmetic scaffold with bounded `mul`
  coverage by adding `bir.mul` printer/lowering support plus targeted
  regression helpers and RISC-V BIR pipeline tests for the constant-only slice.
- Tightened `src/codegen/llvm/llvm_codegen.cpp` so automatic BIR-pipeline
  selection remains RISC-V-only; x86/AArch64 backend codegen no longer picks up
  newly lowerable BIR slices unless the caller explicitly requests the BIR
  pipeline.
- Rebuilt from a clean `build/` directory, reran targeted backend validation,
  refreshed `test_fail_after.log`, and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728/2728` before and after, no newly failing tests).
- Widened the BIR straight-line arithmetic scaffold with bounded constant-only
  signed division by adding `bir.sdiv` printer/lowering support, backend test
  fixtures, and targeted BIR text-surface regressions for the explicit
  pipeline plus the RISC-V default-route case.
- Rebuilt from a clean `build/` directory, reran `backend_bir_tests` plus the
  new `backend_codegen_route_riscv64_return_sdiv_defaults_to_bir` coverage,
  refreshed `test_fail_after.log`, and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728 -> 2729` passed, `0 -> 0` failed, no newly failing tests).
- Widened the BIR straight-line arithmetic scaffold with bounded constant-only
  signed remainder by adding `bir.srem` printer/lowering support, backend test
  fixtures, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 14 % 5;`.
- Rebuilt the tree and reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_srem_defaults_to_bir` coverage to keep
  the `srem` slice bounded to the intended BIR text surfaces before the full
  regression pass.
- Refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728 -> 2730` passed, `0 -> 0` failed, no newly failing tests).
- Widened the BIR straight-line arithmetic scaffold with bounded constant-only
  unsigned remainder by adding `bir.urem` printer/lowering support, backend
  test fixtures, explicit BIR pipeline coverage, and a new RISC-V
  default-route regression for `return 14u % 5u;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_urem_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2731` passed, `0 -> 0` failed, no
  newly failing tests).
- Widened the BIR scaffold with a bounded compare-materialization slice by
  folding constant-only `icmp eq` plus `zext i1 -> i32` return patterns into
  `bir.eq` on the BIR text path, with explicit lowering and pipeline coverage
  plus a new RISC-V default-route regression for `return 7 == 7;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_eq_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2732` passed, `0 -> 0` failed, no
  newly failing tests).
- Widened the BIR compare scaffold with a bounded constant-only signed
  less-than materialization slice by folding `icmp slt` plus `zext i1 -> i32`
  return patterns into `bir.slt`, with printer/lowering coverage, explicit BIR
  pipeline coverage, and a new RISC-V default-route regression for
  `return 3 < 7;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_slt_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2733` passed, `0 -> 0` failed, no
  newly failing tests).
- Widened the BIR compare scaffold with a bounded constant-only unsigned
  less-than materialization slice by folding `icmp ult` plus `zext i1 -> i32`
  return patterns into `bir.ult`, with printer/lowering coverage, explicit BIR
  pipeline coverage, and a new RISC-V default-route regression for
  `return 3u < 7u;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_ult_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2734` passed, `0 -> 0` failed, no
  newly failing tests).
- Widened the BIR compare scaffold with a bounded constant-only unsigned
  less-than-or-equal materialization slice by folding `icmp ule` plus
  `zext i1 -> i32` return patterns into `bir.ule`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7u <= 7u;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_ule_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2735` passed, `0 -> 0` failed, no
  newly failing tests).
- Widened the BIR compare scaffold with a bounded constant-only signed
  greater-than materialization slice by folding `icmp sgt` plus
  `zext i1 -> i32` return patterns into `bir.sgt`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7 > 3;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_sgt_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a clean rebuild and full
  `ctest --test-dir build -j --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2736` passed, `0 -> 0` failed, no
  newly failing tests).
- Widened the BIR compare scaffold with a bounded constant-only signed
  less-than-or-equal materialization slice by folding `icmp sle` plus
  `zext i1 -> i32` return patterns into `bir.sle`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7 <= 7;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_sle_defaults_to_bir` coverage, then
  refreshed `test_after.log` and `test_fail_after.log` with a full
  `ctest --test-dir build -j --output-on-failure` pass.
- Passed the regression guard against `test_before.log` (`2736 -> 2737`
  passed, `0 -> 0` failed, no newly failing tests) and against
  `test_fail_before.log` (`2728 -> 2737` passed, `0 -> 0` failed, no newly
  failing tests). A first full-suite attempt hit transient timeouts plus one
  isolated `llvm_gcc_c_torture_src_pr28982b_c` failure, but all three suspect
  cases passed immediately on targeted rerun and the subsequent full rerun
  completed green.
- Widened the BIR compare scaffold with a bounded constant-only signed
  greater-than-or-equal materialization slice by folding `icmp sge` plus
  `zext i1 -> i32` return patterns into `bir.sge`, with printer/lowering
  coverage, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 7 >= 7;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_sge_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with two full `ctest --test-dir build -j
  --output-on-failure` passes.
- On the first full-suite attempt, 20 unrelated tests failed or timed out but
  all passed immediately under `ctest --rerun-failed --output-on-failure`,
  indicating a transient run rather than a stable regression from the `sge`
  slice.
- On the refreshed full-suite pass, only `cpp_eastl_utility_parse_recipe`
  timed out at the suite level; it passed immediately on targeted rerun
  (`15.78 sec`), and the regression guard against `test_fail_before.log`
  passed with `--allow-non-decreasing-passed` (`2728 -> 2737` passed, `0 -> 1`
  failed, no newly failing tests, no new `>30s` cases).

Next intended slice:
- Continue Phase 2 parity with the next bounded straight-line instruction gap
  that still linearizes into one BIR block, likely the next constant-only
  bitwise opcode after `and` (`or` or `xor`) before considering shift support
  or any broader matcher widening.

Resume notes:
- `backend.cpp` still contains the legacy route (`emit_legacy_module`), but
  direct BIR backend execution now dispatches into emitter-owned BIR entry
  points instead of pre-lowering in the backend facade.
- `backend.hpp` still exposes both legacy `BackendModule` and BIR-capable input
  seams, but x86/aarch64 direct-BIR entry points now have no fallback back into
  legacy backend IR; only the native affine-return subset is accepted there.
- `lir_to_bir.cpp` now accepts bounded straight-line `i8` arithmetic slices in
  addition to the existing `i32` and `i64` scaffolds, but backend asm emitters
  still only consume the narrower direct-BIR affine subset.
- `lir_to_bir.cpp` now folds constant-only compare materialization through the
  full bounded integer predicate set
  (`eq`/`ne`/`slt`/`sle`/`sgt`/`sge`/`ult`/`ule`/`ugt`/`uge`) when the result is
  immediately widened back to the same integer type for return.
- `lir_to_bir.cpp` now also accepts the smallest constant-only bitwise integer
  slice for direct BIR routing via `and`; the remaining straight-line
  instruction gap is still the rest of the bitwise/shift surface.
- `lir_to_bir.cpp` now also accepts the smallest source-shaped compare-driven
  ternary control-flow surfaces when they can be collapsed back into one BIR
  block: direct branch-to-return and empty branch-only goto chains feeding a
  final constant/parameter `phi` join, now covered for both one-parameter and
  two-parameter compare-fed ternary inputs.
- `lir_to_bir.cpp` now also hoists the bounded 4-block predecessor-compute
  compare-fed phi-join shape when each arm is an add/sub-only affine chain over
  params/immediates and the join still collapses into a single `bir.select`.
- `lir_to_bir.cpp` now also preserves bounded join-local add/sub chains after a
  collapsed compare-fed `phi` join, as long as the whole slice still fits in
  one BIR block and only references values made available by the fused select
  and prior bounded instructions.
- `lir_to_bir.cpp` is now regression-covered for the 6-block split-predecessor
  ternary variant where each arm computes a bounded add/sub affine chain before
  an empty end block and the join still linearizes into one `bir.select`
  followed by a bounded post-join add.
- The next bounded gap is instruction coverage rather than another scalar type:
  the BIR scaffold still rejects straight-line integer arithmetic outside
  `add/sub/mul/and/sdiv/udiv/srem/urem` plus the newly added bounded
  `eq`/`ne`/`slt`/`sle`/`sgt`/`sge`/`ult`/`ule`/`ugt`/`uge` materialization
  slices, and it still lacks the broader compare/select/control-flow clusters
  once the branch shape stops collapsing into the bounded single-block BIR
  `select` surface.
- Auto-selection into the BIR pipeline in `llvm_codegen.cpp` is intentionally
  constrained to `Target::Riscv64`; explicit BIR pipeline options are still the
  only supported way to exercise non-RISC-V direct-BIR emitter slices.
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.
