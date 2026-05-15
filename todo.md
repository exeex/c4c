Status: Active
Source Idea Path: ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Attach Register-Save And Overflow Storage Authority

# Current Packet

## Just Finished

Step 1 completed: AAPCS64 variadic entry plans now attach prepared
register-save-area slot id/stack offset and overflow-area base slot id/stack
offset storage authority when helper facts require them. Focused prepared
printer, frame, liveness, and AArch64 dispatch tests cover populated storage
facts and incomplete overflow-base diagnostics without adding selected helper
machine-node lowering.

## Suggested Next

Execute Step 2 from `plan.md`: decide or delegate the remaining prepared helper
resource authority needed before selected `va_start`/`va_arg` machine-node
consumption can proceed.

## Watchouts

- Do not implement selected `va_start`, `va_arg`, aggregate `va_arg`, or
  `va_copy` machine-node lowering in this prerequisite.
- The delegated dispatch proof for a complete helper fixture still needs
  preexisting scratch-resource facts; this packet intentionally did not add
  helper scratch policy.
- Preserve fail-closed diagnostics for missing prepared storage facts.

## Proof

Supervisor-selected proof passed and was written to `test_after.log`:
`cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'`.
