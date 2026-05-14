Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move AArch64 Physical Register Mapping Into Target MIR

# Current Packet

## Just Finished

Step 4 migrated AArch64 `MoveRecord` and `AbiBindingRecord` destination register construction to prefer structured register placement before legacy register spelling.

`module.cpp` now resolves move and ABI-binding destination register string fields through `PreparedRegisterPlacement` when present, falling back to legacy names only when placement is absent or cannot convert. `backend_aarch64_prepared_frame_control_test` now proves a move destination uses placement when the legacy register name is mismatched, and an ABI binding exposes the placement-derived destination register when the legacy register name is absent.

## Suggested Next

Next coherent packet: inspect remaining AArch64 prepared-to-MIR register-name consumers outside moved call/move/ABI records and migrate any still-owned destination/source surfaces to structured placement-first resolution.

## Watchouts

- This packet intentionally did not remove legacy parsing; older/manual prepared fixtures can still provide only legacy register names.
- AArch64 `CallArgument` and `CallResult` placement mapping is bounded to slot indexes 0..7. Return mapping currently accepts slot 0 only.
- Target register records, selected operands, and occupied register vectors still retain `occupied_registers` from prepared occupied-name text; this packet only moves the exposed destination register spelling to structured placement.
- `clang-format` was not available in this environment, so formatting was kept manual.

## Proof

Ran delegated focused AArch64 placement/register-record proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_aarch64_(prepared_frame_control|prepared_scalar_alu_records|prepared_register_conversion|prepared_operand_identity)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 4/4 selected AArch64 backend tests passed. Proof log: `test_after.log`.
