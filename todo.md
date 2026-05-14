Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated the AArch64 prepared scalar ALU/cast record register helpers to placement-first resolution.

`records.cpp` now lets `make_prepared_register_operand` convert `PreparedStoragePlanValue::register_placement` before consulting legacy register names, while preserving the old home/storage register-name equality fallback when placement is absent. The prepared scalar ALU operand/result gates and prepared scalar cast operand/result gates now accept placement-backed register storage with absent or mismatched legacy names. Focused ALU and cast record tests cover placement-only and mismatched-legacy storage registers.

## Suggested Next

Next coherent packet: supervisor review of the Step 4 placement slices against `review/step4_placement_review.md`, then decide whether the active plan needs another executor packet or lifecycle handling.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only legacy register names.
- Placement-backed scalar casts currently validate result storage placement but `ScalarCastRecord` does not expose a result register field; selected cast instruction records still carry result identity through `result_value_id`/`result_value_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records, selected operands, and occupied register vectors still retain `occupied_registers` from prepared occupied-name text; this packet only moves the exposed destination register spelling to structured placement.
- `review/step4_placement_review.md` was pre-existing and intentionally left untouched.

## Proof

Ran delegated focused AArch64 scalar placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_scalar_alu_records|prepared_scalar_cast_records|scalar_alu_records|scalar_cast_records|prepared_register_conversion)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 5/5 selected AArch64 backend tests passed. Proof log: `test_after.log`.
