Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated AArch64 prepared operand/register-record construction for regalloc assigned registers, spill-authority scratch registers, and storage-plan registers to prefer structured placement conversion before legacy register spelling.

`module.cpp` now builds those target register records from `PreparedPhysicalRegisterAssignment::placement` or `PreparedStoragePlanValue::register_placement` when present, preserving legacy spelling as fallback only. `backend_aarch64_prepared_operand_identity_test` now uses deliberately mismatched legacy register names to prove placement selects `x19`, `x20`, and reserved scratch `x9`.

## Suggested Next

Next coherent packet: migrate the remaining AArch64 prepared-to-MIR register consumers that still read move, call, preserved, or spill/reload register-name fields directly, using the same placement-first conversion pattern and focused mismatch tests.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only `register_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records still retain `occupied_registers` from prepared occupied-name text; this packet only moves the target physical register/reference decision to structured placement for assigned, spill-authority, and storage-plan register records.
- Keep future migration slices semantic; do not replace string assertions with target-name-shaped special cases.

## Proof

Ran delegated focused AArch64 placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_register_conversion|prepared_operand_identity|prepared_scalar_alu_records|scalar_alu_records|prepared_frame_control)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 5/5 selected AArch64 backend tests passed. Proof log: `test_after.log`.
