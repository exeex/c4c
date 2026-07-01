# 501 Step 2 Register-To-Stack Consumer Moves

## Implementation

Step 2 implemented the narrow RV64 materialization path for coherent prepared
before-instruction `consumer_register_to_stack` move bundles.

Touched production surface:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `fragment_for_prepared_move_bundle` now keeps the existing register
    destination path unchanged.
  - Stack-slot destinations are accepted only when the bundle/move shape is:
    `phase=BeforeInstruction`, `authority=None`, `destination_kind=Value`,
    `destination_storage=StackSlot`, `op_kind=Move`, no cycle-temp source, no
    synthetic `destination_stack_offset_bytes`, no source immediate, and
    `reason=consumer_register_to_stack`.
  - The source route is derived from `move.from_value_id` plus prepared value
    homes and must resolve to a GPR.
  - The destination route is derived from `move.to_value_id` plus prepared value
    homes and must resolve to a scalar stack slot through
    `prepared_stack_slot_home_offset`.
  - Emission uses the existing `append_rv64_store_register_to_stack` helper.

## Focused Coverage

Updated `tests/backend/mir/backend_riscv_object_emission_test.cpp`:

- Added a before-instruction register-to-stack move-bundle fixture based on the
  existing scalar compare/trunc prepared module.
- Added accepted coverage proving RV64 emits `sw t0, 16(sp)` before the
  prepared instruction.
- Added fail-closed coverage for adjacent unsupported shapes:
  wrong move reason (`consumer_stack_to_stack`), source home resolving to stack
  instead of GPR, wrong bundle phase, and synthetic destination stack offset.

The implementation intentionally does not add stack-to-stack, out-of-SSA,
before-return, or select-publication materialization.

## Residual Route

Step 3 should handle the residual `consumer_stack_to_stack` before-instruction
family as a separate packet. That packet needs an explicit temporary-register
policy for load-then-store and must continue to use prepared homes instead of
case-log/source-shape inference.

## Proof

Focused pre-proof run:

```sh
cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed.

Delegated proof command:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
