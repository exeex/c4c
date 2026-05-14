Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Semantic String Provenance From MIR Records

# Current Packet

## Just Finished

Step 5 began by demoting AArch64 MIR register occupancy string vectors for the targeted codegen record surfaces.

`RegisterOperand` now carries typed `occupied_register_references` as the authoritative occupancy surface, with legacy `occupied_registers` populated from the converted `RegisterReference` display spelling instead of prepared storage strings. `SpillReloadInstructionRecord` now carries typed `occupied_scratch_register_references`, and spill/reload machine-node selection requires typed scratch occupancy; older bridges that only pass a scratch `RegisterReference` are normalized inside `make_spill_reload_instruction`.

Focused tests now assert scalar and spill/reload MIR occupancy through `RegisterReference` identity instead of semantic occupied-register strings.

## Suggested Next

Next coherent packet: continue Step 5 by widening typed occupancy into the adjacent AArch64 module/target-register record surfaces that still expose `occupied_registers` string vectors, especially move, ABI binding, spill/reload, and target-register records.

## Watchouts

- This packet intentionally left legacy `occupied_registers` and `occupied_scratch_registers` fields in codegen records as compatibility/display surfaces; typed `RegisterReference` vectors are now the selected-node authority for the touched surfaces.
- AArch64 module records still retain semantic string occupancy vectors, and the target-register record tests still assert those strings. Moving those to typed/display-separated fields likely requires touching `src/backend/mir/aarch64/module/*`, which was outside this packet's owned code files.
- Placement-backed scalar casts validate result storage placement, but `ScalarCastRecord` does not expose a result register field; selected cast instruction records still carry result identity through `result_value_id`/`result_value_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- `review/step4_placement_review.md` was pre-existing and intentionally left untouched.

## Proof

Delegated Step 5 proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|prepared_register_conversion|target_instruction_records|prepared_operand_identity|prepared_scalar_alu_records|prepared_scalar_cast_records)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 6/6 selected AArch64 backend tests passed. Proof log: `test_after.log`.
