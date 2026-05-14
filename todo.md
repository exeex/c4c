Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate Variadic Entry Facts In Prepared State

# Current Packet

## Just Finished

Step 3, Populate Variadic Entry Facts In Prepared State, completed the prepared
population slice for AAPCS64 variadic function-entry carriers.

Implementation notes:
- `populate_variadic_entry_plans` now populates AAPCS64 register-save-area,
  overflow-area, `va_list` layout, signed initial offset, saved-register-count,
  slot-size, and helper-use facts from prepared target profile plus existing BIR
  parameter ABI metadata and `llvm.va_*` calls.
- Functions without observed variadic helper use keep the Step 2 structural
  carrier but do not receive fabricated save-area or `va_list` requirements.
- Missing frame/storage facts that prepared state does not yet allocate are
  recorded on the carrier as `missing_required_facts` instead of being invented
  in AArch64 lowering.
- `PreparedCallPlan::variadic_fpr_arg_register_count` remains call-boundary
  metadata only; no AArch64 target lowering or printer consumption was added.

## Suggested Next

Start Step 4 by extending prepared dump and focused test coverage for the
variadic entry carrier family beyond the current AAPCS64 `va_start` proof,
including integer, FP, aggregate, and `va_copy` observations where the current
IR exposes them.

## Watchouts

- The carrier records required-but-missing prepared storage facts such as
  `register_save_area.slot_id`, `register_save_area.stack_offset_bytes`, and
  `overflow_area.base_stack_offset_bytes`; later packets should either allocate
  these in prepared/frame state or keep downstream AArch64 paths fail-closed.
- Helper scratch-resource counts remain unknown because this packet did not add
  a prepared scratch-policy authority.
- No AArch64 target lowering/printer files were touched; target consumption
  still belongs to the later fail-closed packet.

## Proof

Ran the delegated proof command:

`(cmake --build build --target backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and three-test
CTest run.
