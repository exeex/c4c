Status: Active
Source Idea Path: ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Helper Operand-Home Authority

# Current Packet

## Just Finished

Step 3 completed: AAPCS64 variadic entry plans now publish helper operand-home
authority for recognized helper calls. The shared carrier records `va_start`
destination `va_list` homes, scalar `va_arg` result/source homes, aggregate
`va_arg` destination payload/source homes, and `va_copy` destination/source
homes; AArch64 call records consume those prepared facts while selected helper
machine-node lowering remains deferred.

## Suggested Next

Execute the next supervisor-selected packet for idea 244, likely the follow-on
consumption/validation slice that uses the prepared helper operand-home carrier
without implementing idea 243 selected helper machine-node lowering.

## Watchouts

- Selected `va_start`, `va_arg`, aggregate `va_arg`, and `va_copy`
  machine-node lowering is still intentionally fail-closed/deferred.
- Helper operand-home records are producer-owned prepared facts; target codegen
  should not re-derive these homes from raw BIR operands.
- The aggregate `va_arg` carrier supports explicit destination-payload operand
  form and the existing result-backed compatibility shape.

## Proof

Supervisor-selected proof passed and was written to `test_after.log`:
`cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'`.
