Status: Active
Source Idea Path: ideas/open/364_aarch64_hanoi_first_move_peg_selection_mismatch.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Localized First-Move Owner

# Current Packet

## Just Finished

Plan Step 2 - Repair The Localized First-Move Owner: repaired same-register
select-materialized predecessor edge publication so the edge materializer owns
non-immediate out-of-SSA same-register moves instead of leaving a no-op
`mov x13, x13`. When pointer-addressed edge load materialization fails closed,
the publication now falls back to the prepared home of the already-lowered edge
load result, preserving the actual producer snapshot. Added
`predecessor_join_source_publication_materializes_loaded_edge_compare` coverage
for a loaded RHS compare feeding a same-register join destination. Generated
`00181` now emits `ldr w13, [sp, #112]`, `cmp w13, #0`, `cset w13, eq` at
`.LBB193_24`; the old `mov x13, x13` RHS publication is gone, and
`c_testsuite_aarch64_backend_src_00181_c` passes.

## Suggested Next

Supervisor should review the completed Step 2 slice for acceptance and choose
whether the active runbook has another owner boundary to pursue or is ready for
plan-owner lifecycle review.

## Watchouts

- Do not repair this by matching `00181`, `Move`, Hanoi tower globals, peg
  letters, output text, block labels, stack offsets, ABI registers, or emitted
  instruction neighborhoods.
- The stale join reloads from idea 363 remain absent in live generated `00181`;
  `.LBB193_18` and `.LBB193_25` branch from `w13` after fresh edge
  publication, with no reload of old stack-carried condition operands.
- Preserve idea 363's publication-scope guard:
  `source_parallel_copy_successor_label == context.control_flow_block->block_label`.
- Preserve idea 362's address-scaling evidence: the index carrier and
  immediate scale must stay in distinct registers for pointer-derived offsets.
- Preserve idea 361's materialized pointer store writeback evidence.
- Preserve idea 360's correct starting-state evidence.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build up to date after the slice; 7/7 tests passed:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00181_c`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c`. Proof log: `test_after.log`.
