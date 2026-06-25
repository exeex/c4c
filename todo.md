Status: Active
Source Idea Path: ideas/open/364_rv64_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the First RV64 `va_start` Lowering Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the first RV64 `va_start` lowering contract.
The RV64 object-emission helper boundary now distinguishes:

- complete prepared `VaStart` operand homes that still lack explicit
  `overflow_area.base_slot_id` or `base_stack_offset_bytes`; these reject with
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`
- complete prepared `VaStart` operand homes with explicit overflow-area base
  state; these remain at the real lowering boundary,
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_start helper`

Focused RV64 object-emission coverage now builds both prepared-fact variants
directly in `make_prepared_variadic_va_start_module()` and checks both
diagnostics through the prepared object module and ELF writer paths.

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

## Proof

Proof passed and is preserved in `test_after.log`.

Command:

`cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log`

Result: all three focused tests passed:
`backend_riscv_object_emission`, `backend_prepare_frame_stack_call_contract`,
and `backend_prepared_printer`.
