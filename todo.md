Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated AArch64 spill/reload scratch conversion to prefer `PreparedSpillReloadOp::register_placement` before legacy register spelling.

`module.cpp` now builds spill/reload target-register records and machine-node scratch operands from structured placement when present, while preserving the old `register_name` fallback for placement-less fixtures. `backend_aarch64_prepared_frame_control_test` now gives spill/reload ops mismatched legacy names and proves the target register record plus selected machine scratch operand still resolve through placement to `x20`.

## Suggested Next

Next coherent packet: migrate the remaining AArch64 prepared-to-MIR register consumers that still read move, call, preserved, or ABI-binding register-name fields directly, using the same placement-first conversion pattern and focused mismatch tests.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only `register_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records and selected operands still retain `occupied_registers` from prepared occupied-name text; this packet only moves the target physical register/reference decision to structured placement.
- Keep future migration slices semantic; do not replace string assertions with target-name-shaped special cases.

## Proof

Ran delegated focused AArch64 placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|memory_operand_contract|prepared_register_conversion|prepared_operand_identity)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 4/4 selected AArch64 backend tests passed. Proof log: `test_after.log`.
