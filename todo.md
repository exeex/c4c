Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Semantic String Provenance From MIR Records

# Current Packet

## Just Finished

Step 5 continued by demoting the remaining delegated AArch64 module register record surfaces for move, ABI binding, call ABI, spill/reload, parallel-copy target move, and target-register occupancy paths.

`CallArgumentRecord`, `CallResultRecord`, `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, `ParallelCopyStepRecord`, and `TargetRegisterRecord` now expose typed `RegisterReference` register and occupancy fields for the delegated Step 5 surfaces. Legacy string/display fields remain as compatibility surfaces and are populated from typed `RegisterReference` display spellings when conversion succeeds.

Focused frame-control tests now assert typed identity for representative call argument/result, value-location and regalloc moves, ABI binding, parallel-copy target move, spill/reload scratch, and target-register occupancy paths.

## Suggested Next

Next coherent packet: audit the remaining AArch64 module display-only register spelling records outside this packet, especially callee-save/clobbered/preserved-value register records, and decide whether Step 5 should add typed `RegisterReference` compatibility fields there or leave them as non-semantic display metadata.

## Watchouts

- This packet intentionally left legacy string fields in the module records as compatibility/display surfaces; typed `RegisterReference` fields are now the authoritative register identity for the touched surfaces.
- `clang-format` is not installed in this environment, so touched C++ files were not auto-formatted beyond manual style matching.
- Placement-backed scalar casts validate result storage placement, but `ScalarCastRecord` does not expose a result register field; selected cast instruction records still carry result identity through `result_value_id`/`result_value_name`.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- `review/step4_placement_review.md` was pre-existing and intentionally left untouched.

## Proof

Delegated Step 5 proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|prepared_module_identity|target_instruction_records|prepared_operand_identity|prepared_scalar_alu_records|prepared_scalar_cast_records)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 6/6 selected AArch64 backend tests passed. Proof log: `test_after.log`.
