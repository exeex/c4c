Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 started the AArch64 target mapping migration by adding a target-layer conversion path from `PreparedRegisterPlacement` to `aarch64::abi::RegisterReference`.

`aarch64::abi::convert_prepared_register` now maps structured call-argument, call-result, caller-saved, callee-saved, and reserved-scratch placements to AArch64 target register records, validates bank/class/view metadata, and fails closed on out-of-range slots. Prepared physical assignments and saved-register carriers prefer structured placement when present before falling back to legacy spelling.

A narrow AArch64 return-ABI lowering consumer now prefers `PreparedMoveResolution::destination_register_placement` for function return registers, with the legacy spelling fallback preserved for older/manual fixtures that only carry `x0`-style text.

## Suggested Next

Next coherent packet: migrate another AArch64 prepared-to-MIR consumer away from parsing prepared register names, likely storage-plan operand conversion or spill/reload scratch conversion, and add a focused test that proves placement-first behavior with mismatched legacy spelling.

## Watchouts

- This packet intentionally did not remove legacy parsing; it establishes the AArch64 placement-first mapping path and updates one return-ABI consumer.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Prepared-printer summary/detail text includes extra `placement=` or `spill_slot=` tokens; future string checks should assert those structured tokens for migrated records instead of relying only on `reg=` or `slot_id=`.
- Keep future migration slices semantic; do not replace string assertions with target-name-shaped special cases.

## Proof

Ran focused AArch64 conversion and prepared-return proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_register_conversion|prepared_scalar_alu_records|scalar_alu_records|prepared_frame_control|prepared_operand_identity)$" --output-on-failure' > test_after.log 2>&1`

Result: passed.

Ran broader AArch64 validation, now preserved in `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_aarch64_" --output-on-failure' > test_after.log 2>&1`

Result: passed; 24/24 AArch64 backend tests passed.
