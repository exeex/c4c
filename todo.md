Status: Active
Source Idea Path: ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Helper Scratch Resource Authority

# Current Packet

## Just Finished

Step 2 completed: AAPCS64 variadic entry plans now publish shared helper
scratch-resource authority for recognized helper families. `va_start` and
`va_copy` require one scratch register, scalar and aggregate `va_arg` require
two scratch registers, and all currently require zero scratch stack bytes.
Focused prepared printer, frame, liveness, target-record, printer, and AArch64
dispatch tests cover populated scratch facts and missing scratch-register /
scratch-stack diagnostics without adding selected helper machine-node lowering.

## Suggested Next

Execute Step 3 from `plan.md`: add helper operand-home authority for `va_start`
destination `va_list` pointers, scalar `va_arg` result/source homes, aggregate
`va_arg` destination/source homes, and `va_copy` source/destination homes.

## Watchouts

- Do not implement selected `va_start`, `va_arg`, aggregate `va_arg`, or
  `va_copy` machine-node lowering in this prerequisite.
- Scratch-resource authority is now a shared maximum across recognized helpers
  in the entry plan, not a physical register allocation.
- Preserve fail-closed diagnostics for missing prepared storage and scratch
  facts while adding operand-home authority.

## Proof

Supervisor-selected proof passed and was written to `test_after.log`:
`cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'`.
