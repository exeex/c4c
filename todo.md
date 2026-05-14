Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated AArch64 return-selected scalar operand conversion for storage-plan registers to prefer `PreparedStoragePlanValue::register_placement` before legacy storage register spelling.

`module.cpp` now lets selected scalar operands convert register-encoded storage from structured placement even when storage `register_name` is absent or mismatched, while preserving the old value-home/register-name equality fallback for placement-less fixtures. `backend_aarch64_prepared_scalar_alu_records_test` adds a return-selected scalar module fixture that proves lhs/rhs operands come from placement-selected `x1`/`x2`, not legacy storage spelling.

## Suggested Next

Next coherent packet: migrate the remaining AArch64 prepared-to-MIR register consumers that still read move, call, preserved, or spill/reload register-name fields directly, using the same placement-first conversion pattern and focused mismatch tests.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only `register_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records and selected operands still retain `occupied_registers` from prepared occupied-name text; this packet only moves the target physical register/reference decision to structured placement.
- Keep future migration slices semantic; do not replace string assertions with target-name-shaped special cases.

## Proof

Ran delegated focused AArch64 placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_scalar_alu_records|scalar_alu_records|prepared_operand_identity|prepared_register_conversion)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 4/4 selected AArch64 backend tests passed. Proof log: `test_after.log`.
