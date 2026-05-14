Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Semantic String Provenance From MIR Records

# Current Packet

## Just Finished

Step 4 is complete after the placement-first AArch64 migration series.

The reviewer-requested remaining move-family consumers were handled after `review/step4_placement_review.md`: parallel-copy target moves and regalloc move records now resolve `destination_register_placement` before legacy `destination_register_name`, and scalar ALU/cast record helpers now convert `PreparedStoragePlanValue::register_placement` before consulting legacy register names. Compatibility fallbacks remain for older/manual prepared fixtures, but normal AArch64 lowering no longer depends on those strings for the completed Step 4 surfaces.

## Suggested Next

Next coherent packet: begin Step 5 by removing or demoting semantic string provenance from AArch64 MIR records. Start with fields such as `occupied_registers` and other `std::string_view` register-name record data, preserving only explicit display/provenance or compatibility bridges where diagnostics still need text.

## Watchouts

- Step 5 still owns semantic string-record cleanup; do not count remaining printer/display paths or compatibility fallbacks as Step 4 blockers unless they are used as normal lowering authority.
- Target register records, selected operands, and occupied register vectors still retain `occupied_registers` from prepared occupied-name text; Step 5 should replace or demote these rather than widening Step 4.
- Placement-backed scalar casts validate result storage placement, but `ScalarCastRecord` does not expose a result register field; selected cast instruction records still carry result identity through `result_value_id`/`result_value_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- `review/step4_placement_review.md` was pre-existing and intentionally left untouched.

## Proof

Plan-owner lifecycle decision only; no implementation validation run.

Most recent delegated focused AArch64 scalar placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_scalar_alu_records|prepared_scalar_cast_records|scalar_alu_records|scalar_cast_records|prepared_register_conversion)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 5/5 selected AArch64 backend tests passed. Proof log: `test_after.log`.
