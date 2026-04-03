Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, add the bounded zero-parameter `unsigned char`
compare-return family beyond the now-landed zero-parameter `unsigned char`
`eq`/`ne` parity pair.
Next target: audit the next smallest bounded widened compare-return gap adjacent
to `return_eq_u8` / `return_ne_u8` before opening a broader widened-width
compare-return family.

Completed this iteration:
- Re-audited the adjacent zero-parameter compare-return backend-route inventory
  in `tests/c/internal/InternalTests.cmake` and confirmed the smallest missing
  widened-width parity case beside the landed `return_eq_u8` slice was
  `return_ne_u8`.
- Added `tests/c/internal/backend_route_case/return_ne_u8.c`, proving the
  bounded zero-parameter `unsigned char` inequality-return wrapper is available
  to the backend-route harness.
- Registered
  `backend_codegen_route_riscv64_return_ne_u8_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.func @choose_ne_u() -> i8 {`, `%t1 = bir.ne i8 7, 3`, `bir.ret i8 %t1`,
  and forbids legacy LLVM IR `define i8 @choose_ne_u()`.
- Added `make_bir_i8_return_ne_module()` to
  `tests/backend/backend_bir_test_support.*` plus
  `test_bir_lowering_accepts_i8_return_ne()` in
  `tests/backend/backend_bir_lowering_tests.cpp` to keep the widened `i8`
  compare-return `ne` shape covered below the backend-route harness.
- Rebuilt the tree, reran `backend_bir_tests`, reran
  `backend_codegen_route_riscv64_return_ne_u8_defaults_to_bir`, reran the
  adjacent compare-return quartet
  (`backend_codegen_route_riscv64_return_eq_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_ne_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_eq_u8_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_ne_u8_defaults_to_bir`), reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `363/363` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2800 -> 2801` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Re-audited the bounded `i32` versus widened-width/source-level `unsigned char`
  compare/select backend-route inventory in `tests/c/internal/InternalTests.cmake`
  and confirmed the previously active `select` parity matrix is now fully
  covered; the next smallest adjacent widened-width gap moved to the plain
  zero-parameter compare-return family at `return_eq` / `return_ne`.
- Added `tests/c/internal/backend_route_case/return_eq_u8.c`, proving the
  bounded zero-parameter `unsigned char` equality-return wrapper is available to
  the backend-route harness.
- Registered
  `backend_codegen_route_riscv64_return_eq_u8_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.func @choose_eq_u() -> i8 {`, `%t1 = bir.eq i8 7, 7`, `bir.ret i8 %t1`,
  and forbids legacy LLVM IR `define i8 @choose_eq_u()`.
- Used the new route test to expose a real Step 2 direct-BIR gap: the
  zero-parameter widened-`i8` compare-return case still lowered as legacy LLVM
  IR `icmp i32 -> zext i32 -> trunc i8 -> ret`.
- Fixed `src/backend/lowering/lir_to_bir.cpp` with a dedicated bounded widened
  `i8` compare-return lowering path so the zero-parameter compare-plus-zext-plus
  trunc shape lowers directly to `bir.eq i8`.
- Added `make_bir_i8_return_eq_module()` to
  `tests/backend/backend_bir_test_support.*` plus
  `test_bir_lowering_accepts_i8_return_eq()` in
  `tests/backend/backend_bir_lowering_tests.cpp` to keep the widened `i8`
  compare-return shape covered below the backend-route harness.
- Rebuilt the tree, reran `backend_bir_tests`, reran
  `backend_codegen_route_riscv64_return_eq_u8_defaults_to_bir`, reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `362/362` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2800` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the immediate zero-parameter compare/select parity gap in
  `tests/c/internal/InternalTests.cmake` and confirmed `i32`
  `return_select_ne` already had route coverage while widened-width/source-level
  `unsigned char` only had the adjacent `return_select_eq_u8` case.
- Added `tests/c/internal/backend_route_case/return_select_ne_u8.c`, proving the
  bounded zero-parameter `unsigned char` `!=` constant-return wrapper is
  available to the backend-route harness.
- Registered
  `backend_codegen_route_riscv64_return_select_ne_u8_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.func @choose_const_ne_u() -> i8 {` and `bir.ret i8 11`, and forbids
  legacy LLVM IR `define i8 @choose_const_ne_u()`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_return_select_ne_u8_defaults_to_bir`, reran
  the adjacent zero-parameter select parity quartet
  (`backend_codegen_route_riscv64_return_select_eq_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_select_ne_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_select_eq_u8_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_select_ne_u8_defaults_to_bir`), reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `361/361` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2799` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the adjacent compare/select backend-route inventory and confirmed the
  smallest remaining parity gap after the single-parameter `unsigned char`
  `eq`/`ne` slice was the zero-parameter constant-return `u8` `select eq`
  family, because `i32` already had `return_select_eq` / `return_select_ne`
  coverage while widened `u8` had neither zero-parameter counterpart.
- Added `tests/c/internal/backend_route_case/return_select_eq_u8.c` and
  registered
  `backend_codegen_route_riscv64_return_select_eq_u8_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.func @choose_const_u() -> i8 {` and `bir.ret i8 11`, and forbids legacy
  LLVM IR `define i8 @choose_const_u()`.
- Used the new route test to expose a real Step 2 direct-BIR gap: the
  zero-parameter widened-`i8` case constant-folded to a trunc-only
  `i32 -> i8` return shape that still fell back to legacy LLVM IR text.
- Fixed `src/backend/lowering/lir_to_bir.cpp` so the widened-`i8`
  add/sub-chain lowering path accepts the minimal trunc-only form instead of
  requiring extra setup instructions before the final `trunc`.
- Added `make_bir_i8_return_immediate_module()` to
  `tests/backend/backend_bir_test_support.*` plus
  `test_bir_lowering_accepts_i8_return_immediate()` in
  `tests/backend/backend_bir_lowering_tests.cpp` to keep the trunc-only
  widened-`i8` return-immediate slice covered below the backend-route harness.
- Rebuilt the tree, reran
  `backend_codegen_route_riscv64_return_select_eq_u8_defaults_to_bir`, reran
  `backend_bir_tests`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `360/360` backend-labeled tests passing.

Completed this iteration:
- Audited the remaining adjacent direct-BIR compare/select inventory in
  `tests/c/internal/InternalTests.cmake` and confirmed the
  single-parameter constant-return `unsigned char` family is currently bounded
  to `eq`/`ne`, so the next smallest Step 2 pivot is the zero-parameter
  constant-return select family rather than opening a wider predicate matrix.
- Added `tests/c/internal/backend_route_case/return_select_ne.c`, proving the
  bounded `return 7 != 3 ? 11 : 4;` backend-route slice can be exercised
  directly through the same RISC-V backend harness as `return_select_eq`.
- Registered `backend_codegen_route_riscv64_return_select_ne_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @main() -> i32 {` and `bir.ret i32 11`, and forbids
  legacy LLVM IR `define i32 @main()`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_return_select_ne_defaults_to_bir`, reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `359/359` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2797` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the current `i32` versus widened-width/source-level `unsigned char`
  compare/select route inventory in `tests/c/internal/InternalTests.cmake` and
  confirmed the two-parameter `eq` split-predecessor matrix already has full
  bounded `u8` parity; the remaining adjacent widening gap was the
  single-parameter `!=` case.
- Added `tests/c/internal/backend_route_case/single_param_u8_select_ne.c`,
  proving the bounded single-parameter `u8` `!=` compare/select wrapper can be
  exercised directly through the backend-route harness.
- Registered
  `backend_codegen_route_riscv64_single_param_u8_select_ne_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.func @choose_ne_u(i8 %p.x) -> i8 {`, `bir.select ne i8 %p.x, 7, 11, 4`,
  and forbids legacy LLVM IR `define i8 @choose_ne_u(i8 %p.x)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_single_param_u8_select_ne_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `358/358` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2796` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the remaining non-split predecessor-arm widened-width/source-level
  `unsigned char` compare/select add/select shape and confirmed the default
  `--codegen asm` route already stays on the direct BIR pipeline for
  `unsigned char choose2_add_post_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 5 : y + 9) + 6); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_predecessor_add_post_add.c`,
  proving the bounded two-parameter `u8` non-split predecessor add/select
  post-add wrapper reaches the backend through BIR instead of legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_predecessor_add_post_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_add_post_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 5`, `%t13 = bir.add i8 %p.y, 9`,
  `%t14 = bir.select eq i8 %p.x, %p.y, %t11, %t13`,
  `%t15 = bir.add i8 %t14, 6`, and forbids legacy LLVM IR
  `define i8 @choose2_add_post_u(i8 %p.x, i8 %p.y)`.
- Reconfigured the tree, reran the targeted parity pair
  (`backend_codegen_route_riscv64_two_param_u8_select_eq_predecessor_add_post_add_defaults_to_bir`
  and
  `backend_codegen_route_riscv64_two_param_select_eq_predecessor_add_post_add_defaults_to_bir`),
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `357/357` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2795` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the signed `i32` versus widened-width/source-level `unsigned char`
  split-predecessor compare/select matrix and found the actual remaining parity
  gap was the `add_phi` family, not the already-landed deeper/mixed variants.
- Confirmed the default `--codegen asm` route already stays on the direct BIR
  pipeline for
  `unsigned char choose2_add_post_chain_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 5 : y + 9) + 6 - 2); }`
  and
  `unsigned char choose2_add_post_chain_tail_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 5 : y + 9) + 6 - 2 + 9); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub.c`
  and
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_add.c`,
  proving the bounded two-parameter `u8` split-predecessor add-phi
  post-add-sub and post-add-sub-add wrappers reach the backend through BIR
  instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_defaults_to_bir`
  and
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_add_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 5`, `%t13 = bir.add i8 %p.y, 9`,
  `%t14 = bir.select eq i8 %p.x, %p.y, %t11, %t13`,
  `%t15 = bir.add i8 %t14, 6`, `%t16 = bir.sub i8 %t15, 2`,
  plus `bir.func @choose2_add_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t17 = bir.add i8 %t16, 9`, and forbids the legacy LLVM IR
  `define i8 @choose2_add_post_chain_u(i8 %p.x, i8 %p.y)` /
  `define i8 @choose2_add_post_chain_tail_u(i8 %p.x, i8 %p.y)`.

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  deeper-then-mixed predecessor-arm compare/select wrapper with the adjacent
  join-local post-add-sub-add tail, and confirmed the default `--codegen asm`
  route already stays on the direct BIR pipeline for
  `unsigned char choose2_deeper_post_chain_tail_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 + 5 : y + 11 - 4) + 6 - 2 + 9); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add.c`,
  proving the bounded two-parameter `u8` deeper-then-mixed predecessor-arm
  post-add-sub-add join-tail wrapper reaches the backend through BIR instead of
  legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t13 = bir.add i8 %t12, 5`, `%t15 = bir.add i8 %p.y, 11`,
  `%t16 = bir.sub i8 %t15, 4`, `%t17 = bir.select eq i8 %p.x, %p.y, %t13, %t16`,
  `%t18 = bir.add i8 %t17, 6`, `%t19 = bir.sub i8 %t18, 2`,
  `%t20 = bir.add i8 %t19, 9`, and forbids legacy LLVM IR
  `define i8 @choose2_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `354/354` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2792` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Current active item: Step 2, re-audit the widened-width/source-level
`unsigned char` compare/select matrix against the already-covered `i32`
split-predecessor variants and identify the next bounded parity slice.
Next target: confirm whether any emitter-facing `i32` split-predecessor
compare/select backend-route cases still lack widened-`u8` coverage before
adding the next route assertion.

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  deeper-then-mixed predecessor-arm compare/select wrapper with the adjacent
  join-local post-add-sub chain, and confirmed the default `--codegen asm`
  route already stays on the direct BIR pipeline for
  `unsigned char choose2_deeper_post_chain_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 + 5 : y + 11 - 4) + 6 - 2); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub.c`,
  proving the bounded two-parameter `u8` deeper-then-mixed predecessor-arm
  post-add-sub wrapper reaches the backend through BIR instead of legacy LLVM
  IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_deeper_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t13 = bir.add i8 %t12, 5`, `%t15 = bir.add i8 %p.y, 11`,
  `%t16 = bir.sub i8 %t15, 4`, `%t17 = bir.select eq i8 %p.x, %p.y, %t13, %t16`,
  `%t18 = bir.add i8 %t17, 6`, `%t19 = bir.sub i8 %t18, 2`, and forbids legacy
  LLVM IR `define i8 @choose2_deeper_post_chain_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `352/352` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2790` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  deeper-on-both-arms split-predecessor compare/select wrapper with the
  adjacent join-local post-add-sub chain, and confirmed the default
  `--codegen asm` route already stays on the direct BIR pipeline for
  `unsigned char choose2_deeper_both_post_chain_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7) + 6 - 2); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub.c`,
  proving the bounded two-parameter `u8` deeper-on-both-arms split-predecessor
  post-add-sub join-chain route reaches the backend through BIR instead of
  legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_deeper_both_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t13 = bir.add i8 %t12, 5`, `%t15 = bir.add i8 %p.y, 11`,
  `%t16 = bir.sub i8 %t15, 4`, `%t17 = bir.add i8 %t16, 7`,
  `%t18 = bir.select eq i8 %p.x, %p.y, %t13, %t17`,
  `%t19 = bir.add i8 %t18, 6`, `%t20 = bir.sub i8 %t19, 2`, and forbids
  legacy LLVM IR `define i8 @choose2_deeper_both_post_chain_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `351/351` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2789` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  deeper-on-both-arms split-predecessor compare/select wrapper with the simpler
  join-local post-add tail, and confirmed the default `--codegen asm` route
  already stays on the direct BIR pipeline for
  `unsigned char choose2_deeper_both_post_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7) + 6); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add.c`,
  proving the bounded two-parameter `u8` deeper-on-both-arms split-predecessor
  post-add wrapper reaches the backend through BIR instead of legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_deeper_both_post_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t13 = bir.add i8 %t12, 5`, `%t15 = bir.add i8 %p.y, 11`,
  `%t16 = bir.sub i8 %t15, 4`, `%t17 = bir.add i8 %t16, 7`,
  `%t18 = bir.select eq i8 %p.x, %p.y, %t13, %t17`,
  `%t19 = bir.add i8 %t18, 6`, and forbids legacy LLVM IR
  `define i8 @choose2_deeper_both_post_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `350/350` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2788` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  deeper-on-both-arms split-predecessor compare/select wrapper with the
  join-local post-add-sub-add tail, and confirmed the default `--codegen asm`
  route already stays on the direct BIR pipeline for
  `unsigned char choose2_deeper_both_post_chain_tail_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7) + 6 - 2 + 9); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_add.c`,
  proving the bounded two-parameter `u8` deeper-on-both-arms split-predecessor
  post-add-sub-add join-tail route reaches the backend through BIR instead of
  legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_deeper_both_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t13 = bir.add i8 %t12, 5`, `%t15 = bir.add i8 %p.y, 11`,
  `%t16 = bir.sub i8 %t15, 4`, `%t17 = bir.add i8 %t16, 7`,
  `%t18 = bir.select eq i8 %p.x, %p.y, %t13, %t17`,
  `%t19 = bir.add i8 %t18, 6`, `%t20 = bir.sub i8 %t19, 2`,
  `%t21 = bir.add i8 %t20, 9`, and forbids legacy LLVM IR
  `define i8 @choose2_deeper_both_post_chain_tail_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `349/349` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2787` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Current active item: Step 2, continue tightening the widened-width/source-level
`unsigned char` route matrix by locking in the next bounded compare/select
predecessor-arm variant already covered for `i32`.
Next target: audit the widened-`i8` deeper-then-mixed predecessor-arm
compare/select with a join-local post-add tail
(`x == y ? x + 8 - 3 + 5 : y + 11 - 4`, then `+ 6`) and keep the slice within
Step 2 conditional backend-route parity before considering the mirrored
mixed-then-deeper variant.

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  deeper-then-mixed predecessor-arm compare/select wrapper and confirmed the
  default `--codegen asm` route already stays on the direct BIR pipeline for
  `unsigned char choose2_deeper_post_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 + 5 : y + 11 - 4) + 6); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add.c`,
  proving the bounded two-parameter `u8` deeper-then-mixed predecessor-arm
  post-add wrapper reaches the backend through BIR instead of legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_deeper_post_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t13 = bir.add i8 %t12, 5`, `%t15 = bir.add i8 %p.y, 11`,
  `%t16 = bir.sub i8 %t15, 4`, `%t17 = bir.select eq i8 %p.x, %p.y, %t13, %t16`,
  `%t18 = bir.add i8 %t17, 6`, and forbids legacy LLVM IR
  `define i8 @choose2_deeper_post_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `345/345` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2783` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Current active item: Step 2, continue tightening the widened-width/source-level
`unsigned char` route matrix by carrying the mirrored mixed-then-deeper
predecessor-arm coverage forward from the now-validated post-add-sub slice to
the remaining bounded post-add-sub-add join-chain variant already covered for
`i32`.
Next target: audit the widened-`i8` mirrored mixed-then-deeper predecessor-arm
compare/select with the adjacent join-local post-add-sub-add tail
(`x == y ? x + 8 - 3 : y + 11 - 4 + 7`, then `+ 6 - 2 + 9`) and keep the slice
within Step 2 conditional backend-route parity.

Current active item: Step 2, continue tightening the widened-width/source-level
`unsigned char` route matrix by carrying the bounded predecessor-arm compare/select
coverage forward from the now-validated mirrored mixed-then-deeper
post-add-sub-add slice to the remaining deeper-on-both-arms join-tail variant
already covered for `i32`.
Next target: audit the widened-`i8` two-parameter compare/select with deeper
predecessor-local affine work on both arms and the adjacent join-local
post-add-sub-add tail
(`x == y ? x + 8 - 3 + 5 : y + 11 - 4 + 7`, then `+ 6 - 2 + 9`) and keep the
slice within Step 2 conditional backend-route parity.

Completed this iteration:
- Audited the widened-width/source-level two-parameter `unsigned char`
  compare/select wrapper with the mirrored mixed-then-deeper predecessor-arm
  shape and the bounded join-local post-add-sub-add tail, and confirmed the
  default `--codegen asm` route already stays on the direct BIR pipeline for
  `unsigned char choose2_mixed_then_deeper_post_chain_tail_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 : y + 11 - 4 + 7) + 6 - 2 + 9); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add.c`,
  proving the bounded two-parameter `u8` mirrored mixed-then-deeper
  predecessor-arm post-add-sub-add join chain reaches the backend through BIR
  instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_mixed_then_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t14 = bir.add i8 %p.y, 11`, `%t15 = bir.sub i8 %t14, 4`,
  `%t16 = bir.add i8 %t15, 7`,
  `%t17 = bir.select eq i8 %p.x, %p.y, %t12, %t16`,
  `%t18 = bir.add i8 %t17, 6`, `%t19 = bir.sub i8 %t18, 2`,
  `%t20 = bir.add i8 %t19, 9`, and forbids legacy LLVM IR
  `define i8 @choose2_mixed_then_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran the mirrored mixed-then-deeper
  widened-`i8` route quartet
  (`backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_defaults_to_bir`,
  and
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_defaults_to_bir`),
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `348/348` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2786` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the mirrored widened-width/source-level two-parameter
  `unsigned char` compare/select wrapper with a mixed-then-deeper
  predecessor-arm shape and an adjacent join-local post-add-sub chain, and
  confirmed the default `--codegen asm` route already stays on the direct BIR
  pipeline for
  `unsigned char choose2_mixed_then_deeper_post_chain_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 : y + 11 - 4 + 7) + 6 - 2); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub.c`,
  proving the bounded two-parameter `u8` mixed-then-deeper predecessor-arm
  post-add-sub wrapper reaches the backend through BIR instead of legacy LLVM
  IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_mixed_then_deeper_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t14 = bir.add i8 %p.y, 11`, `%t15 = bir.sub i8 %t14, 4`,
  `%t16 = bir.add i8 %t15, 7`,
  `%t17 = bir.select eq i8 %p.x, %p.y, %t12, %t16`,
  `%t18 = bir.add i8 %t17, 6`, `%t19 = bir.sub i8 %t18, 2`, and forbids legacy
  LLVM IR `define i8 @choose2_mixed_then_deeper_post_chain_u(i8 %p.x, i8 %p.y)`.

Iteration note: the mirrored two-parameter widened-`i8` mixed-then-deeper
predecessor-arm compare/select with a join-local post-add tail is now covered
on the default asm path. Defer only the adjacent post-add-sub and
post-add-sub-add join-chain variants.

Completed this iteration:
- Audited the mirrored widened-width/source-level two-parameter
  `unsigned char` compare/select wrapper with a mixed-then-deeper
  predecessor-arm shape and a bounded join-local post-add tail, and confirmed
  the default `--codegen asm` route already stays on the direct BIR pipeline for
  `unsigned char choose2_mixed_then_deeper_post_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 : y + 11 - 4 + 7) + 6); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add.c`,
  proving the bounded two-parameter `u8` mixed-then-deeper predecessor-arm
  post-add wrapper reaches the backend through BIR instead of legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_mixed_then_deeper_post_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t14 = bir.add i8 %p.y, 11`, `%t15 = bir.sub i8 %t14, 4`,
  `%t16 = bir.add i8 %t15, 7`,
  `%t17 = bir.select eq i8 %p.x, %p.y, %t12, %t16`,
  `%t18 = bir.add i8 %t17, 6`, and forbids legacy LLVM IR
  `define i8 @choose2_mixed_then_deeper_post_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran the mirrored predecessor-arm route
  quartet
  (`backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir`,
  and
  `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir`),
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `346/346` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2784` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the adjacent two-parameter widened-width/source-level `unsigned char`
  compare/select wrapper with predecessor-local mixed-affine arms and a bounded
  join-local post-add-sub-add tail, and confirmed the default `--codegen asm`
  route already stays on the direct BIR pipeline for
  `unsigned char choose2_mixed_post_chain_tail_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 : y + 11 - 4) + 6 - 2 + 9); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_add.c`,
  proving the bounded two-parameter `u8` predecessor-affine post-add-sub-add
  join chain reaches the backend through BIR instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_mixed_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t14 = bir.add i8 %p.y, 11`, `%t15 = bir.sub i8 %t14, 4`,
  `%t16 = bir.select eq i8 %p.x, %p.y, %t12, %t15`, `%t17 = bir.add i8 %t16, 6`,
  `%t18 = bir.sub i8 %t17, 2`, `%t19 = bir.add i8 %t18, 9`, and forbids legacy
  LLVM IR `define i8 @choose2_mixed_post_chain_tail_u(i8 %p.x, i8 %p.y)`.

Completed this iteration:
- Audited the adjacent two-parameter widened-width/source-level `unsigned char`
  compare/select wrapper with predecessor-local mixed-affine arms and a
  bounded join-local post-add-sub chain, and confirmed the default
  `--codegen asm` route already stays on the direct BIR pipeline for
  `unsigned char choose2_mixed_post_chain_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 : y + 11 - 4) + 6 - 2); }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for this slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub.c`,
  proving the bounded two-parameter `u8` predecessor-affine post-add-sub join
  chain reaches the backend through BIR instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_mixed_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t14 = bir.add i8 %p.y, 11`, `%t15 = bir.sub i8 %t14, 4`,
  `%t16 = bir.select eq i8 %p.x, %p.y, %t12, %t15`, `%t17 = bir.add i8 %t16, 6`,
  `%t18 = bir.sub i8 %t17, 2`, and forbids legacy LLVM IR
  `define i8 @choose2_mixed_post_chain_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `343/343` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2781` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the next adjacent widened-width/source-level two-parameter
  `unsigned char` compare/select wrapper with predecessor-local affine arms and
  a join-local post-add, and confirmed it was a real Step 2 gap: the default
  `--codegen asm` route still fell through to legacy LLVM IR text for
  `unsigned char choose2_mixed_post_u(unsigned char x, unsigned char y) { return (unsigned char)((x == y ? x + 8 - 3 : y + 11 - 4) + 6); }`
  before this patch.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add.c`,
  proving the bounded two-parameter `u8` predecessor-affine post-add wrapper
  now reaches the backend through BIR instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose2_mixed_post_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t11 = bir.add i8 %p.x, 8`, `%t12 = bir.sub i8 %t11, 3`,
  `%t14 = bir.add i8 %p.y, 11`, `%t15 = bir.sub i8 %t14, 4`,
  `%t16 = bir.select eq i8 %p.x, %p.y, %t12, %t15`, `%t17 = bir.add i8 %t16, 6`,
  and forbids legacy LLVM IR `define i8 @choose2_mixed_post_u(i8 %p.x, i8 %p.y)`.
- Extended `src/backend/lowering/lir_to_bir.cpp` so the widened-`i8`
  conditional phi/select lowering accepts both the existing branch-local trunc
  form and the next bounded predecessor-affine plus join-local post-add form,
  preserving the earlier single-parameter `u8` select-eq slice while covering
  the new two-parameter route.
- Rebuilt the tree, reran
  `backend_codegen_route_riscv64_single_param_u8_select_eq_defaults_to_bir`
  and
  `backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `342/342` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2780` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the adjacent two-parameter widened-width/source-level `unsigned char`
  compare/select wrapper and confirmed the default `--codegen asm` route
  already stays on the direct BIR pipeline for
  `unsigned char choose2_u(unsigned char x, unsigned char y) { return x == y ? x : y; }`;
  no `src/backend/lowering/lir_to_bir.cpp` change was required for the plain
  two-parameter `u8` select-eq slice.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_select_eq.c`, proving the
  two-parameter `u8` compare/select wrapper reaches the backend through BIR
  instead of falling through legacy LLVM IR text on the default asm route.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_select_eq_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.func @choose2_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `bir.select eq i8 %p.x, %p.y, %p.x, %p.y`, and forbids legacy LLVM IR
  `define i8 @choose2_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_select_eq_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `341/341` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2779` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the next adjacent widened-width/source-level `unsigned char`
  conditional wrapper and confirmed the missing route was a real Step 2 gap:
  `unsigned char choose_u(unsigned char x) { return x == 7 ? (unsigned char)11 : (unsigned char)4; }`
  still fell through to legacy LLVM IR text on the default `--codegen asm`
  path before this patch.
- Added
  `tests/c/internal/backend_route_case/single_param_u8_select_eq.c`, proving
  the single-parameter `u8` conditional wrapper now reaches the backend through
  BIR instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_single_param_u8_select_eq_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose_u(i8 %p.x) -> i8 {`, `bir.select eq i8 %p.x, 7, 11, 4`,
  and forbids legacy LLVM IR `define i8 @choose_u(i8 %p.x)`.
- Extended `src/backend/lowering/lir_to_bir.cpp` with bounded widened-`i8`
  conditional lowering for promoted compare/select slices, including the
  compare-fed phi-join form with branch-local `trunc i32 -> i8` values and the
  matching direct return-select form for the same promoted `i8` surface.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_single_param_u8_select_eq_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `340/340` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2778` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the missing single-parameter widened-width/default-route
  `unsigned char` add/sub chain and confirmed the existing default
  `--codegen asm` route already stays on the direct BIR pipeline for
  `(x + 2) - 1`; no `src/backend/lowering/lir_to_bir.cpp` change was required
  for the unsigned-char single-parameter wrapper.
- Added
  `tests/c/internal/backend_route_case/single_param_u8_add_sub_chain.c`,
  proving `unsigned char tiny_u(unsigned char x) { return (unsigned char)((x + 2) - 1); }`
  reaches the backend through BIR instead of falling through legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_single_param_u8_add_sub_chain_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @tiny_u(i8 %p.x) -> i8 {`, `%t1 = bir.add i8 %p.x, 2`,
  `%t2 = bir.sub i8 %t1, 1`, and forbids legacy LLVM IR
  `define i8 @tiny_u(i8 %p.x)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_single_param_u8_add_sub_chain_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `339/339` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2777` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the adjacent two-parameter unsigned-char staged-affine wrapper and
  confirmed the existing default `--codegen asm` route already stays on the
  direct BIR pipeline for `((x + 2) + y) - 1`; no
  `src/backend/lowering/lir_to_bir.cpp` change was required for the `zext`
  staged form.
- Added
  `tests/c/internal/backend_route_case/two_param_u8_staged_affine.c`, proving
  `unsigned char tiny_mix_u_staged(unsigned char x, unsigned char y) { return (unsigned char)(((x + 2) + y) - 1); }`
  reaches the backend through BIR instead of falling through legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_staged_affine_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @tiny_mix_u_staged(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t1 = bir.add i8 %p.x, 2`, `%t3 = bir.add i8 %t1, %p.y`,
  `%t4 = bir.sub i8 %t3, 1`, and forbids legacy LLVM IR
  `define i8 @tiny_mix_u_staged(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_staged_affine_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `338/338` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2776` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Added
  `tests/c/internal/backend_route_case/two_param_u8_add_sub_chain.c`, proving
  `unsigned char tiny_mix_u(unsigned char x, unsigned char y) { return (unsigned char)(((x + y) + 2) - 1); }`
  reaches the backend through the default BIR asm route for the widened
  unsigned-char surface.
- Registered
  `backend_codegen_route_riscv64_two_param_u8_add_sub_chain_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted asm-route
  text contains `bir.func @tiny_mix_u(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t2 = bir.add i8 %p.x, %p.y`, `%t3 = bir.add i8 %t2, 2`,
  `%t4 = bir.sub i8 %t3, 1`, and forbids legacy LLVM IR
  `define i8 @tiny_mix_u(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_u8_add_sub_chain_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `337/337` backend-labeled tests passing.
- Audited the harness behavior and confirmed this slice is coverage-only for the
  active Step 2 backend route matrix: the backend-route test exercises
  `--codegen asm`, which already emits direct BIR text for the unsigned-char
  add/sub chain even though a direct `--codegen llvm` probe still prints legacy
  LLVM IR text outside the current asm-route contract.

Completed this iteration:
- Audited the widened-width two-parameter `i64` add/sub chain shape and
  confirmed it already stays on the direct BIR pipeline; no
  `src/backend/lowering/lir_to_bir.cpp` change was required for
  `((x + y) + 2) - 1`.
- Added
  `tests/c/internal/backend_route_case/two_param_i64_add_sub_chain.c`,
  proving `long long wide_mix(long long x, long long y) { return ((x + y) + 2) - 1; }`
  reaches the backend through BIR instead of legacy LLVM IR text.
- Registered
  `backend_codegen_route_riscv64_two_param_i64_add_sub_chain_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @wide_mix(i64 %p.x, i64 %p.y) -> i64 {`,
  `%t0 = bir.add i64 %p.x, %p.y`, `%t2 = bir.add i64 %t0, 2`,
  `%t4 = bir.sub i64 %t2, 1`, and forbids legacy LLVM IR
  `define i64 @wide_mix(i64 %p.x, i64 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_i64_add_sub_chain_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `336/336` backend-labeled tests passing, then refreshed
  `test_fail_before.log` and `test_fail_after.log` with full-suite runs and
  passed the regression guard with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2773 -> 2774` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Added `tests/c/internal/backend_route_case/two_param_i64_staged_affine.c`,
  proving `long long wide_mix_staged(long long x, long long y) { return ((x + 2)
  + y) - 1; }` stays on the default backend BIR route.
- Registered
  `backend_codegen_route_riscv64_two_param_i64_staged_affine_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @wide_mix_staged(i64 %p.x, i64 %p.y) -> i64 {`,
  `%t1 = bir.add i64 %p.x, 2`, `%t2 = bir.add i64 %t1, %p.y`,
  `%t4 = bir.sub i64 %t2, 1`, and forbids legacy LLVM IR
  `define i64 @wide_mix_staged(i64 %p.x, i64 %p.y)`.
- Audited the emitted output for the new route case and confirmed no
  `src/backend/lowering/lir_to_bir.cpp` change was needed because the existing
  bounded direct-BIR path already lowers the widened source-level staged affine
  wrapper correctly.
- Reconfigured the tree, reran the new route case plus nearby staged-affine and
  widened-width route coverage, reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with `335/335`
  backend-labeled tests passing, then refreshed `test_before.log` and
  `test_after.log` with full-suite runs and passed the regression guard with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2772 -> 2773` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Audited the source-level/default-route widened-width RISC-V staged
  two-parameter `i8` affine chain and confirmed it already stays on the direct
  BIR pipeline; no new `src/backend/lowering/lir_to_bir.cpp` recognizer work
  was needed for the `(x + 2) + y - 1` wrapper shape.
- Added
  `tests/c/internal/backend_route_case/two_param_i8_staged_affine.c`, proving
  `signed char tiny_mix_staged(signed char x, signed char y) { return (signed char)(((x + 2) + y) - 1); }`
  reaches the backend through BIR instead of falling through legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_i8_staged_affine_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @tiny_mix_staged(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t1 = bir.add i8 %p.x, 2`, `%t3 = bir.add i8 %t1, %p.y`,
  `%t4 = bir.sub i8 %t3, 1`, and forbids legacy LLVM IR
  `define i8 @tiny_mix_staged(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_i8_staged_affine_defaults_to_bir`,
  then reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `334/334` backend-labeled tests passing.

Completed this iteration:
- Audited the source-level/default-route two-parameter widened-width RISC-V
  `i8` affine chain and confirmed it already stays on the direct BIR pipeline;
  the missing piece was route coverage rather than additional
  `src/backend/lowering/lir_to_bir.cpp` work.
- Added
  `tests/c/internal/backend_route_case/two_param_i8_add_sub_chain.c`, proving
  `signed char tiny_mix(signed char x, signed char y) { return (signed char)(((x + y) + 2) - 1); }`
  reaches the backend through BIR instead of falling through legacy LLVM IR
  text.
- Registered
  `backend_codegen_route_riscv64_two_param_i8_add_sub_chain_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @tiny_mix(i8 %p.x, i8 %p.y) -> i8 {`,
  `%t2 = bir.add i8 %p.x, %p.y`, `%t3 = bir.add i8 %t2, 2`,
  `%t4 = bir.sub i8 %t3, 1`, and forbids legacy LLVM IR
  `define i8 @tiny_mix(i8 %p.x, i8 %p.y)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_two_param_i8_add_sub_chain_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `333/333` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2760 -> 2771` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Completed this iteration:
- Added source-level/default-route widened-width RISC-V coverage via
  `tests/c/internal/backend_route_case/single_param_i8_add_sub_chain.c`,
  proving `signed char tiny_char(signed char x) { return (signed char)((x + 2)
  - 1); }` now stays on the BIR pipeline instead of falling through the legacy
  widened `sext i8 -> i32` / `trunc i32 -> i8` route.
- Registered
  `backend_codegen_route_riscv64_single_param_i8_add_sub_chain_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @tiny_char(i8 %p.x) -> i8 {`, `%t1 = bir.add i8 %p.x, 2`,
  `%t2 = bir.sub i8 %t1, 1`, and forbids legacy LLVM IR
  `define i8 @tiny_char(i8 %p.x)`.
- Widened `src/backend/lowering/lir_to_bir.cpp` so minimal scalar lowering
  accepts signed/unsigned char as `i8`, and added a bounded recognizer for the
  source-level `i8` promotion wrapper that rewrites `sext`/`zext i8 -> i32`,
  `add`/`sub i32`, and trailing `trunc i32 -> i8` back into the existing
  direct BIR `i8` chain.
- Rebuilt the tree, reran
  `backend_codegen_route_riscv64_single_param_i8_add_sub_chain_defaults_to_bir`,
  reran `ctest --test-dir build -R backend_bir_tests --output-on-failure`, then
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `332/332` backend-labeled tests passing.

Completed this iteration:
- Added source-level/default-route widened-width RISC-V coverage via
  `tests/c/internal/backend_route_case/single_param_i64_add_sub_chain.c`,
  proving `long long wide_add(long long x) { return (x + 2) - 1; }` now stays
  on the BIR pipeline instead of falling through to LLVM/asm rescue.
- Registered
  `backend_codegen_route_riscv64_single_param_i64_add_sub_chain_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @wide_add(i64 %p.x) -> i64 {`, `%t1 = bir.add i64 %p.x, 2`,
  `%t3 = bir.sub i64 %t1, 1`, and forbids legacy LLVM IR
  `define i64 @wide_add(i64 %p.x)`.
- Widened the straight-line BIR recognizer in `src/backend/lowering/lir_to_bir.cpp`
  to fold bounded lossless immediate `sext`/`zext` casts into canonical BIR
  immediates, which covers the frontend-emitted `sext i32 2 to i64` /
  `sext i32 1 to i64` pattern without widening CFG support or direct-BIR
  emitter behavior.
- Reconfigured and rebuilt the tree, reran the new route case, reran
  `ctest --test-dir build -R backend_bir_tests --output-on-failure`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `331/331` backend-labeled tests passing.

Completed this iteration:
- Added source-level/default-route RISC-V coverage via
  `tests/c/internal/backend_route_case/return_select_eq.c`, proving
  `return 7 == 7 ? 11 : 4;` still stays on the BIR pipeline even though the
  frontend constant-folds the ternary before BIR lowering.
- Registered
  `backend_codegen_route_riscv64_return_select_eq_defaults_to_bir` in
  `tests/c/internal/InternalTests.cmake`, asserting the emitted text contains
  `bir.ret i32 11` and forbids legacy LLVM IR `define i32 @main()` output for
  that folded default-route surface.
- Audited the constant-only compare-fed `select` gap against the direct BIR
  pipeline tests and confirmed the source-level form cannot currently exercise
  `bir.select` because `build/c4cll --target riscv64-unknown-linux-gnu
  --codegen llvm tests/c/internal/backend_route_case/return_select_eq.c`
  emits a constant return directly.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_return_select_eq_defaults_to_bir`, then reran
  `ctest --test-dir build -L backend --output-on-failure -j8` with
  `330/330` backend-labeled tests passing.

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

Completed this iteration:
- Audited the current Step 2 frontier against the tree and confirmed the
  earlier `todo.md` note about the next constant-only bitwise opcode was stale:
  the repo already contains bounded direct-BIR route coverage for
  `or`/`xor`/`shl`/`lshr`/`ashr`, and backend validation now reports
  `353/353` backend-labeled tests passing with those slices in place.
- Added
  `tests/c/internal/backend_route_case/single_param_select_ne.c`,
  extending the compare-fed single-block select route matrix beyond `eq` with
  the bounded one-parameter `ne` ternary
  `int choose_ne(int x) { return x != 7 ? 11 : 4; }`.
- Registered
  `backend_codegen_route_riscv64_single_param_select_ne_defaults_to_bir`
  in `tests/c/internal/InternalTests.cmake`, asserting the emitted text
  contains `bir.func @choose_ne(i32 %p.x) -> i32 {`,
  `%t8 = bir.select ne i32 %p.x, 7, 11, 4`, and `bir.ret i32 %t8`, while
  forbidding legacy LLVM IR `define i32 @choose_ne(i32 %p.x)`.
- Reconfigured and rebuilt the tree, reran
  `backend_codegen_route_riscv64_single_param_select_ne_defaults_to_bir`,
  reran `ctest --test-dir build -L backend --output-on-failure -j8` with
  `353/353` backend-labeled tests passing, then refreshed
  `test_fail_after.log` with a full `ctest --test-dir build -j8 --output-on-failure`
  run and passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed --timeout-threshold 30 --enforce-timeout`
  (`2782 -> 2791` passed, `0 -> 0` failed, no newly failing tests, no new
  `>30s` cases).

Next intended slice:
- Continue Phase 2 parity by widening compare-fed `bir.select` predicate
  coverage beyond the new `i32 ne` route, likely the analogous bounded
  source-level `unsigned char` single-parameter `ne` ternary before moving on
  to broader non-`eq` predecessor-arm select matrices.

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
- `lir_to_bir.cpp` now also accepts the bounded constant-only straight-line
  bitwise/shift surface currently covered in-tree
  (`and`/`or`/`xor`/`shl`/`lshr`/`ashr`) in addition to the earlier arithmetic
  and compare-materialization slices.
- `lir_to_bir.cpp` now also accepts the smallest source-shaped compare-driven
  ternary control-flow surfaces when they can be collapsed back into one BIR
  block: direct branch-to-return and empty branch-only goto chains feeding a
  final constant/parameter `phi` join, now covered for both one-parameter and
  two-parameter compare-fed ternary inputs; the single-parameter scalar route
  matrix now includes both `eq` and `ne` predicate coverage for the direct
  `bir.select` surface.
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
- The next bounded gap is now more about predicate-matrix coverage than opcode
  inventory: the straight-line constant-only arithmetic/bitwise/shift scaffold
  exercised by the current route tests is in place, but broader non-`eq`
  compare/select/control-flow clusters are still only partially regression-
  covered once the branch shape stops collapsing into the bounded single-block
  BIR `select` surface.
- Auto-selection into the BIR pipeline in `llvm_codegen.cpp` is intentionally
  constrained to `Target::Riscv64`; explicit BIR pipeline options are still the
  only supported way to exercise non-RISC-V direct-BIR emitter slices.
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.
