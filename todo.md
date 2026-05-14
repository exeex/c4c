Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated the remaining reviewed AArch64 move-family direct destination-register consumers to placement-first resolution.

`module.cpp` now resolves `ParallelCopyStepRecord::target_destination_register` and regalloc-created `MoveRecord::destination_register` through `PreparedRegisterPlacement` when present, falling back to legacy names only when placement is absent or cannot convert. `backend_aarch64_prepared_frame_control_test` now proves a parallel-copy target destination uses a placement-only register with no legacy name, and a regalloc move destination uses placement when the legacy register name is mismatched.

## Suggested Next

Next coherent packet: supervisor review of the Step 4 slice against `review/step4_placement_review.md`, then decide whether the active plan needs another executor packet or lifecycle handling.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only legacy register names.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records, selected operands, and occupied register vectors still retain `occupied_registers` from prepared occupied-name text; this packet only moves the exposed destination register spelling to structured placement.
- `review/step4_placement_review.md` was pre-existing and intentionally left untouched.

## Proof

Ran delegated focused AArch64 placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|prepared_module_identity|prepared_scalar_alu_records|prepared_register_conversion|prepared_operand_identity)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 5/5 selected AArch64 backend tests passed. Proof log: `test_after.log`.
