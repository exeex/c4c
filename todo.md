Status: Active
Source Idea Path: ideas/open/364_rv64_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the First RV64 `va_start` Lowering Contract

# Current Packet

## Just Finished

Completed a corrective `plan.md` Step 2 contract fix after the hook full-suite
baseline candidate rejected commit `8278eed3`: the CLI expected-failure case
`backend_cli_riscv64_variadic_entry_missing_contract_obj` exposed that
fact-complete helper-free RV64 variadic functions could bypass helper
diagnostics and fall through to normal object emission.

RV64 object emission now fails closed when a prepared variadic entry plan has
complete required facts but no prepared helper boundary, with:

`unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract`

Focused coverage now distinguishes all Step 2 rejection classes:

- missing variadic entry plan
- missing required variadic entry facts
- fact-complete helper-free variadic entry with no explicit supported runtime
  contract
- complete `VaStart` missing overflow-area initial base state
- complete `VaStart` with base state still stopped at the lowering boundary

## Suggested Next

Implement `plan.md` Step 3 as a narrow materialization packet for the prepared
`VaStart` path that has complete operand homes and explicit overflow-area base
state. The packet should store the initial overflow pointer into the RV64
`va_list` overflow-arg-area field and preserve the precise missing-base-state
diagnostic for incomplete runtime state.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen scalar `va_arg`, `va_copy`, external variadic call lowering, or
  general parameter-home expansion as part of this packet.
- Do not infer `va_start` semantics from the BIR call shape in the RV64 target
  route. The target route should consume `PreparedVariadicEntryHelperOperandHomes`
  and entry-plan runtime facts.
- `destination_va_list` can be a current value home while
  `destination_va_list_address` is a materialized stack-slot home for the
  addressable `va_list` object. The object route must be explicit about which
  home it stores into.
- RV64 `va_list` layout is only the overflow pointer. Materializing
  `va_start` requires a pointer to the first unnamed stack argument, not just
  the destination `va_list` slot.
- This packet did not change prepared producers, so producer-owned RV64 facts
  still need to supply explicit overflow-area base state before the materialized
  path can become reachable from normal prepared modules.
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
