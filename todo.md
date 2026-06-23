Status: Active
Source Idea Path: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair RV64 Consumption and Runtime Behavior

# Current Packet

## Just Finished

Completed Step 4 RV64 consumption repair for the loop-carried pointer
post-increment path.

RV64 prepared scalar emission now consumes pointer `PointerBasePlusOffset`
homes as pointer values: pointer moves can read register or 8-byte stack homes,
and prepared pointer add can publish computed pointer results either to a
register home or an 8-byte stack home. Frame-slot address materialization also
publishes stack-homed computed-address results, so local pointer initialization
does not load stale pointer homes.

RV64 prepared local-memory store emission now stores pointer values through the
same prepared move path when the source is a computed pointer home rather than a
direct pointer register.

The regression fix keeps dynamic local-array GEP compatibility intact: the
frame-slot address-materialization helper only emits a stack-homed computed
address when the prepared materialization belongs to the pointer-add result.
When the materialization belongs to the base object for an already-handled
dynamic GEP, it preserves the old no-op path instead of failing emission.

`backend_codegen_route_riscv64_loop_carried_pointer_postincrement` was flipped
from expected-repair to a positive codegen contract and
`backend_rv64_runtime_riscv64_loop_carried_pointer_postincrement` was added.
Together with the existing dump test, the focused coverage now proves that the
loop body loads current pointer locals, computes the next pointer by adding the
element stride to that current value, stores the updated pointer local, copies
the halfword through the old pointer value, and returns 0 under qemu.

The fixture no longer compares `from`/`to` against `&source[3]` and `&sink[3]`.
Those endpoint checks introduced a separate unprepared local-array GEP compare
route (`%t62`/`%t68` register homes without address materialization), which is
not the Step 4 RV64 consumption boundary. The runtime assertion remains focused
on copied values, while dump/codegen contracts reject fixed array-base
rematerialization for the loop-carried pointer updates.

## Suggested Next

Run Step 5 reprobe for `src/00143.c`. The focused loop-carried pointer body is
ready for candidate reprobe, but endpoint local-array GEP comparison may need a
separate prepared publication route if it appears in broader candidates.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block layout, `%lv.from`,
  `%lv.to`, `%t29`, fixed stack offsets, block names, or fixed array sizes.
- Do not fake endpoint `&array[3]` comparison values from source names or
  observed stack offsets; require semantic/prepared address materialization or
  pointer-base facts if that route becomes owned later.
- Keep the dynamic-GEP no-op compatibility in the frame-slot materialization
  path. Treating base-object materializations as failed result materializations
  regresses stack-homed fused compare, empty loop-exit successor, and i16
  local-array select/store routes.

## Proof

Focused loop-carried proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_loop_carried_pointer_postincrement'`

Result: passed, 3/3 tests.

Regression-focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route|rv64_runtime)_riscv64_(loop_carried_pointer_postincrement|empty_loop_exit_successor|i16_local_array_select_store|stack_homed_fused_compare_missing_false_label)|backend_rv64_runtime_packed_local_member_offsets'`

Result: passed, 9/9 tests. The loop-carried codegen/runtime coverage stayed
positive, and the previously regressed stack-homed fused compare, empty
loop-exit successor, i16 local-array select/store, and packed local member
runtime routes passed.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 284/285 tests
passing (`test_after.log`). The only remaining failure is the existing
`backend_riscv_prepared_edge_publication` contract. The loop-carried
codegen/runtime tests and all listed regression routes passed in the broad run.
