Status: Active
Source Idea Path: ideas/open/342_aarch64_duff_do_while_latch_condition_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Duplicate Latch Decrement Boundary

# Current Packet

## Just Finished

Step 1 localized the duplicate Duff latch decrement boundary. Prepared BIR is
correct at `dowhile.cond.6`: `%t34 = %lv.n - 1`, `store_local %lv.n, %t34`,
`%t35 = %t34 > 0`, and `cond_br %t35`. Prepared control-flow metadata also
records `branch_condition dowhile.cond.6 kind=fused_compare condition=%t35
compare=sgt i32 %t34, 0 can_fuse_with_branch=yes`.

The first bad owner is AArch64 prepared-block terminator lowering in
`src/backend/mir/aarch64/codegen/dispatch.cpp`, specifically
`lower_materialized_compare_condition_branch()`. For `00143`, `%t35` is stack
homed, so `dispatch_prepared_block()` materializes the compare as `cmp/cset`,
then selects `lower_materialized_compare_condition_branch()` for the
terminator. That path rebuilds the branch compare by calling
`emit_value_publication_to_register()` on the compare lhs `%t34` at the `%t35`
producer index. Since `%t34` has a same-block add producer, publication
recursively re-emits `%t34 = load n + -1` instead of reusing the already
emitted/stored post-decrement value. This creates the second `n - 1` used by
the branch.

Probe artifacts:
- `/tmp/c4c_342_00143_semantic_bir.txt`
- `/tmp/c4c_342_00143_prepared_bir.txt`
- `/tmp/c4c_342_focus_dowhile_prepared.txt`
- `/tmp/c4c_342_00143.s`
- `/tmp/c4c_342_min_dowhile.c`, `/tmp/c4c_342_min_dowhile_prepared.txt`,
  `/tmp/c4c_342_min_dowhile.s`

## Suggested Next

Step 2 should repair AArch64 conditional-branch lowering so a fused compare
whose condition value was already materialized does not reconstruct mutable
same-block compare operands after intervening writeback. Preferred repair
boundary: teach `lower_materialized_compare_condition_branch()` or its operand
publication helper to reuse an emitted/current prepared home for compare
operands such as `%t34` before falling back to producer replay, while preserving
general compare/branch semantics and avoiding any `00143` or label-specific
matching.

## Watchouts

- Do not reopen idea 341's fixed-offset fallthrough copy emission unless the
  old empty fallthrough copy labels return.
- Do not special-case `00143`, `.LBB1_29`, Duff-device labels, local name `n`,
  source lines, block numbers, or emitted instruction spellings.
- Keep expectation, unsupported, runner, timeout, CTest registration, and
  proof-log policy unchanged.
- The minimal `/tmp/c4c_342_min_dowhile.c` probe does not duplicate the
  decrement because its condition value remains register-homed and falls
  through the `cbnz` path. The failing boundary needs coverage where the
  compare condition is stack-homed but the compare lhs has an already-current
  post-writeback value.

## Proof

Delegated proof ran:
`cmake --build build --target c4cll -j 2 && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c' > test_after.log 2>&1`

Result: build was up to date, and the focused CTest still fails
`c_testsuite_aarch64_backend_src_00143_c` as `[RUNTIME_NONZERO]`, matching the
localized unfixed residual. Proof log: `test_after.log`.
