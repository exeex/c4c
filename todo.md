Status: Active
Source Idea Path: ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Prepared Select Condition Join Rule

# Current Packet

## Just Finished

Plan Step 2 - Repair The Prepared Select Condition Join Rule: narrowed the
stale prepared select/condition join reload repair after
`review/363_step2_stale_join_reload_review.md`.  Block-entry publication
lookups and join producer-skip checks now only accept out-of-SSA parallel-copy
bundles whose successor label is the block currently being lowered, keeping
unrelated block-entry bundles out of the current join authority boundary.

Focused backend coverage now covers both the synthetic false-edge immediate
`0` fused-compare path and the existing materialized latch fused-compare route.
The synthetic case includes a non-current successor publication for the same
join value in `w12` and proves the current join still consumes the current
publication in `w13`.  Live generated `00181` no longer has stale
branch-operand reloads at the two scan joins: `.LBB193_18` now goes directly
to `cmp w13, #0` without a `[sp, #64]` reload, and `.LBB193_25` now goes
directly to `cmp w13, #0` without a `[sp, #112]` reload.

New residual after this repair: `c_testsuite_aarch64_backend_src_00181_c`
progresses beyond the previous `[RUNTIME_NONZERO] exit=Segmentation fault`
and now fails as `[RUNTIME_MISMATCH]`.  The first printed move places disk `1`
on `B` instead of expected `C` (`actual` first subsequent state has
`B: 0 0 0 1`, `C: 0 0 0 0`; expected has `B: 0 0 0 0`,
`C: 0 0 0 1`).  That residual is beyond the stale reload fixed here.

## Suggested Next

Localize the new `00181` runtime mismatch independently of the stale
select/condition join reload.  The next coherent packet should start from the
first wrong Hanoi move after the scans now terminate correctly and determine
whether the residual is argument/peg selection, recursive call state, or a
later store-address/value publication issue.

## Watchouts

- Do not repair this by matching `00181`, `Move`, `.LBB193_18`,
  `.LBB193_25`, `x13`, `[sp, #64]`, or `[sp, #112]`.
- The stale join reloads are gone in live generated `00181`; do not re-open
  Step 2 through another `[sp, #64]`/`[sp, #112]` reload route unless they
  reappear.
- Publication helpers must stay scoped to
  `source_parallel_copy_successor_label == context.control_flow_block->block_label`;
  accepting unrelated block-entry bundles reintroduces the reviewer-blocking
  stale-join broadening.
- The stale slots are true-edge load homes (`%t11` at `[sp, #64]`, `%t31` at
  `[sp, #112]`); the expected join-visible values are `%t16=0` and `%t36=0`.
- The pointer-derived offset repair still appears stable in generated `Move`:
  source and destination index scaling use a distinct scale register (`mov x10,
  #4`; `mul x9, x9, x10`) rather than folding the scale into the index carrier.
- Preserve idea 362's address-scaling evidence: the index carrier and immediate
  scale must stay in distinct registers for pointer-derived offsets.
- Preserve idea 361's materialized pointer store writeback evidence.
- Preserve idea 360's correct starting-state evidence.
- Do not special-case `00181`, `Move`, Hanoi tower globals, block labels,
  stack offsets, or emitted registers.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: focused subset was 6/7 with
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c` passing.  The in-scope
representative `c_testsuite_aarch64_backend_src_00181_c` now fails later with
`[RUNTIME_MISMATCH]` instead of the previous segfault.  Proof log:
`test_after.log`.  Also ran `git diff --check`, which passed.
