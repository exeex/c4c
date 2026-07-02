Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Materialization Coverage

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: confirmed the committed
`tests/backend/mir/backend_riscv_object_emission_test.cpp` coverage already
satisfies the focused materialization requirement.

Coverage now includes:

- Accepted coherent same-scalar stack-slot to stack-slot prepared move emission,
  asserting the adjacent `lw t1, 4(sp); sw t1, 8(sp)` object words.
- Mixed stack-destination bundle coverage that emits an already-supported
  register-to-stack move followed by the coherent stack-to-stack move.
- Fail-closed reject coverage for missing source slot id, source size/home
  mismatch, unavailable scratch GPR, incoherent source storage identity,
  explicit storage-plan `frame_slot bank=none`, and reason/source-home mismatch.

No expectation rewrites, scan accounting changes, or gcc_torture row-name
dependencies were needed for this packet.

## Suggested Next

Execute Step 5 from `plan.md`: reclassify the representative rows and record
whether they now clear the owned move-bundle gate or expose separate follow-up
blockers.

## Watchouts

This slice intentionally did not make `pr69447.c` acceptable: the traced first
failure is a `zext i16 to i64` shape with `bank=none` source storage, not a
same-type scalar copy. `20010518-1.c` and `pr27073.c` also need reclassification
because Step 2 found register-source first failures despite the
`consumer_stack_to_stack` reason string.

## Proof

Ran the delegated proof:

- `cmake --build build --target backend_riscv_object_emission_test`
- `ctest --test-dir build -j --output-on-failure -R 'backend_riscv_object_emission|stack_to_stack|move_bundle' | tee test_after.log`
- `git diff --check -- todo.md tests/backend/mir/backend_riscv_object_emission_test.cpp`

`test_after.log` contains the passing focused CTest run: 1/1 test passed.
