Status: Active
Source Idea Path: ideas/open/364_aarch64_hanoi_first_move_peg_selection_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The First Wrong Hanoi Move Boundary

# Current Packet

## Just Finished

Plan Step 1 - Localize The First Wrong Hanoi Move Boundary: localized the
first wrong boundary to `Move`'s destination scan condition, not to Hanoi peg
argument selection. Semantic BIR for `while (j<N && dest[j]==0)` produces
`%t31 = load dest[j]`, `%t32 = eq %t31, 0`, and join `%t36` as the loop
condition. Prepared BIR keeps that shape and records a parallel copy
`logic.rhs.end.26 -> logic.end.27` moving `%t32 -> %t36`, with both values
allocated to `x13`. Generated AArch64 for `.LBB193_22` loads `dest[j]` into
`w9` and spills it to `[sp, #112]`, but `.LBB193_24` lowers the RHS edge
publication as `mov x13, x13`; `.LBB193_25` then compares stale `w13` instead
of the loaded `dest[j] == 0` result. On the first real move the recursive
argument path is still correct (`Hanoi(4,A,B,C)` -> `Hanoi(3,A,C,B)` ->
`Hanoi(2,A,B,C)` -> `Hanoi(1,A,C,B)` -> `Move(A,C)`), but `j` remains `0`;
the consumer store computes `dest[j-1]`, so with `dest == C` it writes disk
`1` to `C[-1]`, which is the adjacent `B[3]`. Expected first state:
`B: 0 0 0 0`, `C: 0 0 0 1`; actual first state: `B: 0 0 0 1`,
`C: 0 0 0 0`. Owner classification: prepared/MIR publication of a
select-materialized logical-AND RHS condition edge, specifically same-register
edge copy materialization for `%t32 -> %t36`; not recursive call state,
peg selection, store-address selection, or store-value publication.

## Suggested Next

Plan Step 2 - Repair The Localized First-Move Owner: repair the general
same-register edge-copy/select-materialization publication path so the RHS
condition result is actually available at the join before the fused compare.
Use a focused backend contract for a logical-AND memory-load condition when
practical, then recheck generated `00181` so `Move(A,C)` scans `C` to `j == 4`
and stores disk `1` at `C[3]`.

## Watchouts

- Do not repair this by matching `00181`, `Move`, Hanoi tower globals, peg
  letters, output text, block labels, stack offsets, ABI registers, or emitted
  instruction neighborhoods.
- The stale join reloads from idea 363 remain absent in live generated `00181`:
  `.LBB193_18` and `.LBB193_25` branch from `w13`, with no reload of the old
  stack-carried condition operands. The new bad fact is that the RHS edge
  publication is a no-op when the loaded/equality producer was not actually in
  `x13`.
- Preserve idea 363's publication-scope guard:
  `source_parallel_copy_successor_label == context.control_flow_block->block_label`.
- Preserve idea 362's address-scaling evidence: the index carrier and
  immediate scale must stay in distinct registers for pointer-derived offsets.
- Preserve idea 361's materialized pointer store writeback evidence.
- Preserve idea 360's correct starting-state evidence.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build up to date; 6/7 tests passed. Passing:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c`. Expected remaining failure:
`c_testsuite_aarch64_backend_src_00181_c` runtime mismatch at the first
subsequent Hanoi state. Proof log: `test_after.log`.
