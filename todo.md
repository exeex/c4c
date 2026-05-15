Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Fail-Closed AArch64 Consumption Guards

# Current Packet

## Just Finished

Step 5, Add Fail-Closed AArch64 Consumption Guards, completed the focused
AArch64 consumption-guard slice for prepared variadic function-entry carriers.

Implementation notes:
- AArch64 dispatch now recognizes retained `llvm.va_start.p0`,
  `llvm.va_arg.*`, `llvm.va_arg.aggregate`, and `llvm.va_copy.p0.p0` helper
  calls as variadic entry helper consumers.
- Helper dispatch requires a `PreparedVariadicEntryPlanFunction` for the
  current function before consulting normal call-boundary lowering, and rejects
  incomplete carriers with explicit diagnostics naming the first missing
  prepared fact.
- The guard checks only prepared carrier presence/completeness. It does not
  reconstruct register-save-area, overflow-area, `va_list`, helper scratch, or
  call-boundary variadic ABI policy in AArch64 target lowering.
- AArch64 call records now preserve observed prepared variadic-entry provenance
  and helper kind, while marking helper machine-node lowering as
  `deferred_unsupported` until a later delegated consumption slice.
- Focused AArch64 dispatch, record, and printer tests cover missing carriers,
  incomplete carriers, retained record state, and printer fail-closed behavior.

## Suggested Next

Start Step 6 by validating whether the current prepared-carrier route is enough
to summarize remaining downstream unsupported `va_start`, `va_arg`, `va_copy`,
and machine-node consumption states for supervisor lifecycle review.

## Watchouts

- Current AAPCS64 prepared carriers still intentionally fail closed for helper
  lowering because prepared storage facts such as `register_save_area.slot_id`,
  `register_save_area.stack_offset_bytes`, `overflow_area.base_slot_id`,
  `overflow_area.base_stack_offset_bytes`, and helper scratch-resource counts
  are not allocated by the prepared/frame authority yet.
- The helper-kind carrier remains family-level for scalar `va_arg`, so
  `llvm.va_arg.i32` and `llvm.va_arg.f64` both consume the generic `va_arg`
  observation unless a later plan adds typed per-helper observations.
- A later machine-node consumption slice should consume these structured facts
  directly or remain blocked; it should not add target-local AAPCS64 ABI
  reconstruction.

## Proof

Ran the delegated proof command:

`(cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and six-test
CTest run.
