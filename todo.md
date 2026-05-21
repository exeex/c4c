Status: Active
Source Idea Path: ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Snapshot Writeback Boundary

# Current Packet

## Just Finished

Executed Step 1 localization for idea 371 with the delegated `00157`/`00176`
subset. The proof still fails both representatives: `00157` prints
`0, 43690, 65535, ...` instead of squares, and `00176` prints the original
array plus an incorrectly partially swapped second line instead of sorted
output.

`00157` first bad fact is in the first `Array[Count-1] = Count * Count` loop.
Semantic and prepared BIR preserve the intended selected lane writeback:
`%t12.storeN = bir.select eq i64 %t11, N, i32 %t8, %t12.eltN` followed by
`bir.store_local %lv.Array.N, i32 %t12.storeN`. Prepared storage gives those
selected store values stack homes such as `%t12.store0 stack92`,
`%t12.store1 stack100`, etc. Generated AArch64 computes the square at
`00157.c.s:29-32` and the index at `00157.c.s:35-45`, but then immediately
reloads the unmaterialized selected-store homes and writes them over the local
array lanes, e.g. `ldr w9, [sp, #92]; str w9, [sp]` at `00157.c.s:49-50`,
then `ldr w9, [sp, #100]; str w9, [sp, #4]` at `00157.c.s:53-54`. No select
materialization for `%t12.storeN` is emitted before the `store_local`
writebacks. The later print-loop load select consumes already-corrupted array
contents, so it is downstream.

`00176` has the same snapshot/select family, but the first observable bad fact
is earlier than the final `swap` writeback: in `partition`, at the
`if (array[i] < pivotValue)` dynamic global-array load/compare. BIR and
prepared BIR preserve `%t14.outer0 = select(indexed @array lanes by %t12)`,
then `%t16 = bir.slt i32 %t14.outer0, %t15`, and prepared storage homes
`%t14.outer0` at stack offset 260. Generated AArch64 snapshots global
`array[0]` into `w21` and `array[1..15]` into stack slots at
`00176.c.s:3618-3682`, but it never emits the `%t14.outer0` select chain before
the comparison. It instead loads `x21` from `sp + 260` at
`00176.c.s:3684-3686` and compares that stale/unmaterialized home with
`pivotValue`. The `swap` function also contains the expected per-lane selected
global writeback shape (`%t7.outer0.eltN.inner.store` and
`%t10.outer0.eltN.inner.store`, committed at `00176.c.s:2895-3003` and onward),
but the first bad fact for the failing `00176` run is the dynamic selected
global load feeding the partition branch.

Owning boundary for both representatives is AArch64 lowering/publication of
selected scalar values produced by dynamic indexed aggregate snapshot lowering.
`00157` fails when selected values are used as local aggregate lane store
operands (`StoreLocal`); `00176` fails when a selected dynamic global aggregate
load value feeds scalar compare/control publication. BIR and prepared records
still know the selected value and its consumer; AArch64 must materialize the
selected value before loading its home as a store source or compare operand.

Closed idea 348 is related but should remain archived. Its closure covered the
earlier selected-address/writeback route for `00130`, `00187`, `00195`, and
then `00176`; the current evidence is a fresh follow-up shape where selected
snapshot *values* for local/global aggregate loads and stores are stack-homed
and then consumed without materialization. Treat idea 371 as the live owner
rather than reopening closed 348.

## Suggested Next

Execute Step 2 by adding focused backend coverage for selected aggregate
snapshot value materialization before consumers. The coverage should include a
dynamic local `int` array store lowered to per-lane selected `store_local`
values, and a dynamic global array load lowered to a per-lane selected value
feeding an integer compare/branch. If practical, include the adjacent
global-swap selected `store_global` shape as a second assertion, but the
current `00176` first bad fact is the load-to-compare publication boundary.

## Watchouts

Do not reopen or edit closed idea 348. Do not special-case the representative
filenames, array names, stack offsets, registers, or emitted instruction
sequences. Keep scalar FP, conditional/switch, external-call return,
pointer/subobject, metadata, initializer, bit-field, and timeout buckets parked
under idea 295 unless fresh localization proves a handoff.

The current first bad fact is not address arithmetic, local frame layout,
global symbol address materialization, or loop-control publication. Both reps
compute the dynamic index and preserve selected values in BIR/prepared form;
the missing piece is selected snapshot value materialization before a concrete
consumer reloads the value's home.

## Proof

Delegated proof/localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00176_c)$' > test_after.log 2>&1
```

Result: build completed, CTest selected 2 tests, 0 passed, 2 failed
(`c_testsuite_aarch64_backend_src_00157_c` and
`c_testsuite_aarch64_backend_src_00176_c`). Proof log is `test_after.log`.
Read-only localization also inspected generated assembly under
`build/c_testsuite_aarch64_backend/src/`, sources under
`tests/c/external/c-testsuite/src/`, and temporary BIR/prepared dumps under
`/tmp`.
