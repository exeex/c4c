Status: Active
Source Idea Path: ideas/open/364_rv64_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative and Baseline Validation

# Current Packet

## Just Finished

Completed `plan.md` Step 4 representative and baseline validation for the
focused RV64 `va_start` helper-lowering slice.

The focused build and backend subset remain green. The representative
`src/920908-1.c` scan still fails, but it now fails at the precise prepared
state boundary:

`unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`

This is later and more specific than the original idea-364 starting signal
(`RV64 object route does not yet lower va_start helper`). The supported object
emission path exists for explicit overflow-area base state; the representative
does not currently carry that producer/runtime state into the prepared module.

## Suggested Next

Hand back to the supervisor/plan-owner for lifecycle disposition of idea 364.
Executor recommendation: treat the focused object-emission helper-lowering
runbook as validated, with any work to make `src/920908-1.c` fully pass routed
as follow-up lifecycle work for prepared overflow-area base-state production or
runtime variadic-entry state materialization.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- This packet did not edit implementation, tests, `plan.md`, or
  `ideas/open/*.md`.
- Normal prepared RV64 modules may still lack `overflow_area.base_slot_id` and
  `base_stack_offset_bytes`; the representative confirms that as a
  producer/runtime-state gap rather than an object-emission fallback.
- The materialized object path is intentionally narrow: RV64 single-field
  `va_list`, supported overflow-arg-area field, prepared va_list address in a
  GPR, and signed-12-bit overflow-base/field offsets.
- Do not broaden Step 4 into scalar `va_arg`, `va_copy`, aggregate `va_arg`,
  external variadic calls, or general parameter-home coverage.
- Helper-free fact-complete RV64 variadic entry is intentionally rejected until
  there is an explicit supported variadic entry/runtime contract; do not treat
  a helper-free body that emits an object as plan progress.

## Proof

Proof command completed and is preserved in `test_after.log`.

Command:

`cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test c4cll -j && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj)$' --output-on-failure > test_after.log && printf 'src/920908-1.c\n' > /tmp/rv64-va-start-representative.txt && (CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-va-start-representative.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh >> test_after.log 2>&1 || true)`

Result: all four delegated tests passed:
`backend_riscv_object_emission`, `backend_prepare_frame_stack_call_contract`,
`backend_prepared_printer`, and
`backend_cli_riscv64_variadic_entry_missing_contract_obj`.

Representative result: `src/920908-1.c` failed as allowed by the packet, with
case log at
`build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`. The failure is the
missing prepared overflow-area initial base state diagnostic above, not the old
generic unlowered-helper boundary.
