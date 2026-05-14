Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Semantic String Provenance From MIR Records

# Current Packet

## Just Finished

Step 5 continued by auditing the remaining delegated AArch64 module callee-save, call-preserved, and call-clobbered register spelling surfaces.

`CalleeSaveRecord` now exposes typed `RegisterReference` identity and typed occupied-register identity for both frame callee saves and call clobber records. `CallPreservedValueRecord` now exposes typed register identity, contiguous width, and typed occupied-register identity for callee-saved preservation routes. The legacy string fields remain compatibility/display surfaces and are populated from typed `RegisterReference` display spellings when conversion succeeds.

Focused frame-control tests now assert typed identity for representative frame callee-save, call-preserved, and call-clobbered records, including stale legacy source strings where placement identity is authoritative.

## Suggested Next

Next coherent packet: have the supervisor review whether Step 5 is now exhausted for AArch64 module semantic register-string provenance, or delegate a final narrow scan for any display-only string surfaces outside the module record families already converted.

## Watchouts

- This packet intentionally left legacy string fields in the module records as compatibility/display surfaces; typed `RegisterReference` fields are now the authoritative register identity for the touched surfaces.
- No remaining display-only callee-save, call-preserved, or call-clobbered module register spelling surfaces were found in the owned record families; their register names and occupied-register strings are compatibility/display mirrors.
- `clang-format` is not installed in this environment, so touched C++ files were not auto-formatted beyond manual style matching.
- Placement-backed scalar casts validate result storage placement, but `ScalarCastRecord` does not expose a result register field; selected cast instruction records still carry result identity through `result_value_id`/`result_value_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- `review/step4_placement_review.md` was pre-existing and intentionally left untouched.

## Proof

Delegated Step 5 proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|prepared_module_identity|target_instruction_records|prepared_operand_identity|prepared_scalar_alu_records|prepared_scalar_cast_records)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 6/6 selected AArch64 backend tests passed. Proof log: `test_after.log`.
