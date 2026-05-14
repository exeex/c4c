Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated AArch64 call-plan record construction for `CallArgumentRecord`, `CallResultRecord`, and `CallPreservedValueRecord` to prefer structured register placement before legacy register spelling.

`module.cpp` now resolves call argument destination, call result source/destination, and call-preserved register string fields through `PreparedRegisterPlacement` when present, falling back to legacy names only when placement is absent or cannot convert. `backend_aarch64_prepared_frame_control_test` now proves placement selects the AArch64 target register when the legacy argument register name is absent and when result/preserved legacy names are mismatched.

## Suggested Next

Next coherent packet: migrate any remaining AArch64 prepared-to-MIR register consumers still reading move or ABI-binding register-name fields directly, using the same placement-first conversion pattern and focused mismatch tests.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only legacy register names.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records and selected operands still retain `occupied_registers` from prepared occupied-name text; this packet only moves the target physical register/reference decision to structured placement.
- `CallArgumentRecord` has no source-register string field today, so this packet only changes the existing argument destination register surface.

## Proof

Ran delegated focused AArch64 placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|prepared_register_conversion|prepared_operand_identity)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 3/3 selected AArch64 backend tests passed. Proof log: `test_after.log`.
