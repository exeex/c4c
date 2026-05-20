Status: Active
Source Idea Path: ideas/open/342_aarch64_duff_do_while_latch_condition_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Step 3 proved the idea-342 latch repair still holds under the delegated
representative subset. Backend instruction dispatch, machine printer, prepared
memory operand records, `00086`, and `00111` all pass. `00143` still fails
`[RUNTIME_NONZERO]`, so the remaining residual was classified with temporary
evidence under `/tmp`.

The original duplicate Duff latch decrement is repaired in generated AArch64.
In `/tmp/c4c_00143.s`, `dowhile.cond.6` emits a single decrement sequence:
`ldr w9, [sp, #176]`, `mov w10, #-1`, `add w13, w9, w10`,
`str w13, [sp, #176]`, then both the materialized `cmp/cset` and terminator
`cmp/b.gt` use `w13`. No second subtract is present on the latch path.

The new first bad fact is outside that latch condition repair. Prepared BIR is
correct for the Duff fallthrough copy chain: `/tmp/c4c_00143.prepared.txt`
shows `block_9` copying `addr %lv.a.0+2` to `addr %lv.b.0+2`, `block_10`
copying `+4` to `+4`, continuing consecutively through `block_15`. Generated
AArch64 diverges after the first copy: `.LBB1_27` copies `[sp] -> [sp,#78]`
then branches to `.LBB1_8`, but `.LBB1_8` copies `[sp,#4] -> [sp,#82]`
instead of the prepared-BIR `+2` copy. Subsequent fallthrough blocks continue
skipping every other short offset (`+8 -> +8`, `+12 -> +12`, ...), explaining
the runtime mismatch after the latch fix.

## Suggested Next

Next packet should hand this back to supervisor/plan-owner routing as an
outside-idea-342 residual. Recommended direction: close or split idea 342 as
the latch-condition repair is proven, then open/activate a separate AArch64
Duff fallthrough copy emission repair for the generated-label/fixed-offset
skip.

## Watchouts

- Do not treat `00143` as disproving the idea-342 latch repair; the current
  failure is after the corrected latch and prepared BIR has the right copy
  offsets.
- Do not reopen idea 341's fixed-offset fallthrough copy emission inside idea
  342 unless the supervisor explicitly chooses that scope change.
- Do not special-case `00143`, `.LBB1_29`, Duff-device labels, local name `n`,
  source lines, block numbers, or emitted instruction spellings.
- Keep expectation, unsupported, runner, timeout, CTest registration, and
  proof-log policy unchanged.
- The latch path still emits a materialized `cmp/cset` for `%t35` before the
  terminator compare; that is redundant but no longer branches on a second
  decremented counter value.

## Proof

Delegated Step 3 proof ran:
`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test backend_aarch64_prepared_memory_operand_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer|prepared_memory_operand_records)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`

Result: build succeeded. `backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`,
`backend_aarch64_prepared_memory_operand_records`, `00086`, and `00111`
passed. `c_testsuite_aarch64_backend_src_00143_c` still fails
`[RUNTIME_NONZERO]` after the latch decrement repair, with the residual
classified above as generated AArch64 Duff fallthrough copy emission rather
than latch compare/decrement emission. Proof log: `test_after.log`.

Temporary evidence files:
`/tmp/c4c_00143.s`, `/tmp/c4c_00143.prepared.txt`,
`/tmp/c4c_00143_probe.c`, `/tmp/c4c_00143_probe.s`, and
`/tmp/c4c_00143_probe.prepared.txt`.
