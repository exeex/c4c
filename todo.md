Status: Active
Source Idea Path: ideas/open/364_rv64_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize or Precisely Diagnose `va_start`

# Current Packet

## Just Finished

Completed `plan.md` Step 3 materialization for the first supportable prepared
RV64 `VaStart` path: complete operand homes plus explicit
`overflow_area.base_slot_id` and `base_stack_offset_bytes`.

RV64 object emission now consumes the prepared `VaStart` helper at the exact
block/instruction site, computes the prepared initial overflow pointer from
`sp + base_stack_offset_bytes`, and stores it through the prepared
`destination_va_list_address` GPR into the RV64 `va_list` overflow-arg-area
field. The focused explicit-state fixture now emits real object bytes:
`addi t1, sp, 64; sd t1, 0(a1); addi a0, zero, 0; ret`.

The existing precise rejection for complete operand homes without explicit
overflow-area base state is preserved:

`unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`

Helper-free variadic entry rejection and generic unsupported-helper diagnostics
remain outside the materialized path.

## Suggested Next

Run `plan.md` Step 4 representative and baseline validation. Suggested packet:
exercise the representative RV64 allowlist case `src/920908-1.c`, record
whether it now reaches a later boundary or succeeds, and decide whether any
remaining failure belongs to this idea or a separate open idea.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- This packet did not edit producer files. Normal prepared RV64 modules may
  still lack `overflow_area.base_slot_id` and `base_stack_offset_bytes`; that
  remains a producer/runtime-state gap rather than an object-emission fallback.
- The materialized object path is intentionally narrow: RV64 single-field
  `va_list`, supported overflow-arg-area field, prepared va_list address in a
  GPR, and signed-12-bit overflow-base/field offsets.
- Do not broaden Step 4 into scalar `va_arg`, `va_copy`, aggregate `va_arg`,
  external variadic calls, or general parameter-home coverage.
- Helper-free fact-complete RV64 variadic entry is intentionally rejected until
  there is an explicit supported variadic entry/runtime contract; do not treat
  a helper-free body that emits an object as plan progress.

## Proof

Proof passed and is preserved in `test_after.log`.

Command:

`cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test c4cll -j && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj)$' --output-on-failure > test_after.log`

Result: all four delegated tests passed:
`backend_riscv_object_emission`, `backend_prepare_frame_stack_call_contract`,
`backend_prepared_printer`, and
`backend_cli_riscv64_variadic_entry_missing_contract_obj`.
