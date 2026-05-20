Status: Active
Source Idea Path: ideas/open/342_aarch64_duff_do_while_latch_condition_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Do-While Latch Condition Emission

# Current Packet

## Just Finished

Step 2 repaired AArch64 materialized compare branch lowering for stack-homed
fused-compare conditions. `lower_materialized_compare_condition_branch()` now
receives the current block scalar state and reuses an already emitted GP
register for the compare lhs/rhs before falling back to producer replay. A
scratch-clobber guard rejects reuse when publishing the other compare operand
would overwrite the reused operand.

Added focused instruction-dispatch coverage for a latch-shaped block that
loads a counter, emits one decrement, stores it back, materializes a
stack-homed compare condition, and branches by comparing the already emitted
decrement value. The fixture rejects a second subtract and requires the branch
compare to use the emitted latch value.

`00143` still fails `[RUNTIME_NONZERO]`, but the original duplicate decrement
is gone. The generated `dowhile.cond.6` latch now has one decrement:
`ldr w9, [sp, #176]`, `mov w10, #-1`, `add w13, w9, w10`,
`str w13, [sp, #176]`, followed by both materialized and branch compares on
`w13`. The new first bad fact is back in Duff fallthrough copy emission: after
the corrected latch, the case-copy blocks still copy wrong fixed short
offsets. For example the case-7 block copies `ldrh w9, [sp, #4]` to
`strh w9, [sp, #82]`, skipping the expected next element at `[sp, #2]` /
`[sp, #80]`.

## Suggested Next

Next packet should decide whether the active idea should continue into the
remaining Duff fixed-offset copy lowering fault or hand that residual back to a
plan review. If continuing here, repair the general post-increment/fallthrough
copy offset selection so case blocks copy consecutive short elements instead
of skipping every other element.

## Watchouts

- Do not reopen idea 341's fixed-offset fallthrough copy emission unless the
  supervisor accepts that the first bad fact has moved back to wrong Duff copy
  offsets after the latch fix.
- Do not special-case `00143`, `.LBB1_29`, Duff-device labels, local name `n`,
  source lines, block numbers, or emitted instruction spellings.
- Keep expectation, unsupported, runner, timeout, CTest registration, and
  proof-log policy unchanged.
- The latch path still emits a materialized `cmp/cset` for `%t35` before the
  terminator compare; that is redundant but no longer branches on a second
  decremented counter value.

## Proof

Delegated proof ran:
`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test backend_aarch64_prepared_memory_operand_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer|prepared_memory_operand_records)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`

Result: build succeeded. `backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`,
`backend_aarch64_prepared_memory_operand_records`, `00086`, and `00111`
passed. `c_testsuite_aarch64_backend_src_00143_c` still fails
`[RUNTIME_NONZERO]` after the latch decrement repair, with the residual
classified above. Proof log: `test_after.log`.
