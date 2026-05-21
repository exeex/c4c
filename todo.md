Status: Active
Source Idea Path: ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Stale Join Reload Boundary

# Current Packet

## Just Finished

Plan Step 1 - Localize The Stale Join Reload Boundary: localized the first
prepared select/condition join stale reload in `Move` without implementation
edits.

First bad fact: the source scan join for
`while (i<N && (source[i])==0) i++;` lowers semantic
`%t16 = bir.phi i32 [logic.rhs.end.6, %t12] [logic.skip.5, 0]` into prepared
`%t16 = bir.select slt i32 %t0, 4, i32 %t12, 0` in `logic.end.7`.  Prepared
control-flow records an authoritative `select_materialization` join transfer
for `logic.skip.5 -> logic.end.7` with `incoming=0 destination=%t16`, and
prepared value locations emit a block-entry move bundle for that false edge:
`source_imm_i32=0`, `to_value_id=65` (`%t16`), destination register `x13`.

The generated false edge does materialize the expected join-visible value:
`.LBB193_16: mov x13, #0; b .LBB193_18`.  The first wrong boundary is the
join/condition lowering at `.LBB193_18`, which reloads `%t11`'s stack home
instead of consuming the already-materialized `%t16` value: `ldr x13, [sp,
#64]`, then `mov w9, w13`, `cmp w9, #0`, and branches on that stale value.
`[sp, #64]` is prepared storage for `%t11`, the source-side `source[i]` load
from the true/RHS path, not the false-edge `%t16 = 0` value.  The consumer path
is the prepared branch condition for `logic.end.7`, `condition=%t17 compare=ne
i32 %t16, 0`, which should decide whether to loop to `block_15` or exit to
`block_16`.

Adjacent same-shape example: the destination scan join for
`while (j<N && (dest[j])==0) j++;` has prepared
`%t36 = bir.select slt i32 %t20, 4, i32 %t32, 0`; its false edge
`logic.skip.25 -> logic.end.27` records `incoming=0 destination=%t36` and emits
`.LBB193_23: mov x13, #0; b .LBB193_25`, but the join at `.LBB193_25` reloads
`ldr x13, [sp, #112]`, `%t31`'s stack home from the true/RHS destination load,
before branching on `%t37 = ne %t36, 0`.

Code-owner boundary for the next repair: AArch64 block dispatch already has
predecessor join publication helpers (`lower_predecessor_select_parallel_copy_sources`,
`lower_predecessor_join_source_publication`, and
`emit_edge_value_publication_to_register`), but the current publication path
skips immediate parallel-copy moves (`move.source_immediate_i32.has_value()`)
and the join block later rematerializes the compare/select condition from the
true-path stack load.  The repair should make the general prepared
select/condition join-visible value authoritative at the join/branch consumer,
not special-case `00181`, `Move`, labels, registers, or stack offsets.

## Suggested Next

Execute Plan Step 2. Repair the general prepared select/condition join
publication rule so immediate false/zero edge values remain authoritative for
the join-visible result consumed by fused branch conditions.  A focused backend
contract should cover the same shape independently of `00181`: predecessor
false edge publishes immediate `0` to a register-homed select/phi destination,
and the join branch must not reload the true-edge stack source over it.

## Watchouts

- Do not repair this by matching `00181`, `Move`, `.LBB193_18`,
  `.LBB193_25`, `x13`, `[sp, #64]`, or `[sp, #112]`.
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

Result: build reported `ninja: no work to do`; focused subset was 6/7 with
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c` passing.  The known in-scope
representative `c_testsuite_aarch64_backend_src_00181_c` still fails fast with
`[RUNTIME_NONZERO] exit=Segmentation fault`.  Proof log: `test_after.log`.
