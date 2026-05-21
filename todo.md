Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Starting-State Value-Flow Boundary

# Current Packet

## Just Finished

Plan Step 1 investigation completed without implementation edits. The
starting-state mismatch localizes to generated AArch64 for `main`'s first
initialization loop, not semantic BIR, prepared BIR, or `PrintAll`'s consumer.

Producing source operation: `for(i=0;i<N;i++) A[i]=i+1;` in `00181.c`.
Expected value: `A[2] == 3`, from the `i == 2` iteration.
Semantic/prepared BIR: `block_24` keeps the correct per-element sequence:
`%t10.outer0.elt2 = bir.load_global i32 @A, offset 8`,
`%t10.outer0.elt2.inner.store = bir.select eq i64 %t9, 2, i32 %t7, %t10.outer0.elt2`,
then `bir.store_global @A, offset 8, i32 %t10.outer0.elt2.inner.store`.
Prepared addressing records that as global `A` offset 8 load/store, size 4,
align 4.

Lowered storage/register location: prepared BIR assigns
`%t10.outer0.elt2` to stack slot `#87` at `main` stack offset 80 and
`%t10.outer0.elt2.inner.store` to stack slot `#88` at offset 84. Generated
AArch64 materializes the true `i == 2` value through `[sp, #84]` and stores it
to `A+8`, but the following `i == 3` iteration's non-selected element-2 arm
loads `[sp, #80]`, which still holds the old pre-store `A+8` value `0`, writes
that through `[sp, #84]`, and overwrites `A+8` with `0`.

First wrong boundary: AArch64 emission/scheduling for the expanded
global-array select-store sequence in `main` reads the non-selected element
spill before refreshing it from the current global element. This turns the
semantic "preserve old element unless index matches" value into a stale
frame-slot reuse across loop iterations. The precise bad edge for the visible
`3` is `A+8`: correct store during `i == 2`, stale `[sp, #80]` read during
`i == 3`, then wrong store of `0` back to `A+8`.

Consumer/print path: `PrintAll`'s A-row loop loads `@A` offsets 0, 4, 8, and
12 into local slots, selects by loop index, and passes the selected value as
`printf("%d")` arg1. For the bad row, `PrintAll` consumes the already-wrong
global `A+8 == 0`; its prepared callsite routes arg1 from `PrintAll` stack
offset 72 to `x1`, and emitted AArch64 selects `[sp, #56]` for `A+8` before
`bl printf`.

Adjacent same-shape evidence: the `B` and `C` zeroing loops in `main` use the
same expanded global-array select-store shape over offsets 0, 4, 8, and 12,
but zero initialization masks the stale-preserve bug because the selected value
and expected preserved values are zero. `Move` also contains pointer-indexed
array update/select-store shapes, but the reproduced starting-state mismatch is
already present before `Hanoi` or `Move` is called.

## Suggested Next

Execute plan Step 2 by repairing the general AArch64 lowering boundary for
expanded select-store array updates so each non-selected element preserve uses
the current memory value before the store, not a stale frame slot from a prior
iteration. A focused backend test should cover at least a looped global array
update that writes `arr[i] = i + 1` via the same select-store expansion and
then observes all elements.

## Watchouts

- Do not reopen idea 359's stack-preserved pointer formal post-call repair
  unless fresh first-bad-fact evidence points back there.
- The known visible mismatch remains expected `A: 1 2 3 4`, actual
  `A: 1 2 0 4`. It is fully explained before recursive post-call `%p.spare`
  reuse is observable.
- Avoid a `00181`-specific fix. The owner is the general ordering/value-flow
  rule for selected stores that preserve non-selected array elements.
- Preserve idea 357, 358, and 359 coverage while repairing this boundary.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build was already up to date; 6 of 7 tests passed. The only failure is
`c_testsuite_aarch64_backend_src_00181_c`, with expected starting state
`A: 1 2 3 4` and actual `A: 1 2 0 4`. Proof log: `test_after.log`.
