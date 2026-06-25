Status: Active
Source Idea Path: ideas/open/364_rv64_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared `va_start` Helper Boundary

# Current Packet

## Just Finished

Audited `plan.md` Step 1 for the prepared RV64 `va_start` helper boundary.
Inspection commands/results:

- `sed -n '1,240p' build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`
  found the representative failure at
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_start helper`.
- `rg -n "unsupported_variadic_helper_lowering|va_start|Variadic|Prepared.*Helper|helper" src/backend/mir/riscv/codegen/object_emission.cpp`
  located the target-side rejection path in
  `diagnose_first_unsupported_prepared_variadic_helper()` and
  `diagnose_unsupported_prepared_variadic_helper_fragment()`.
- `rg -n "va_start|Variadic|Prepared.*Helper|helper|VaStart|va_list|overflow" src/backend/prealloc/variadic_entry_plans.cpp src/backend/prealloc/variadic.hpp tests/backend/mir/backend_riscv_object_emission_test.cpp tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp tests/backend/bir/backend_prepared_printer_test.cpp`
  located the prepared producer, helper completeness predicates, and focused
  tests for the same helper facts.

Boundary map:

- Producer: `populate_rv64_variadic_entry_abi_facts()` publishes RV64
  `va_list` as one 8-byte `overflow_arg_area` field at offset 0 and marks the
  overflow area required/aligned to 8.
- Producer: `populate_rv64_variadic_entry_helper_resource_authority()` records
  helper resources for required helpers; `va_start` currently reserves 3
  scratch registers and 0 scratch stack bytes.
- Producer: `populate_rv64_variadic_entry_va_start_operand_home_authority()`
  recognizes `llvm.va_start.p0`, records helper kind `VaStart`, and publishes
  `destination_va_list` from the call operand plus
  `destination_va_list_address` from
  `materialize_va_start_destination_home()`.
- Helper completeness: `has_complete_prepared_variadic_va_start_operand_homes()`
  requires only `helper == VaStart`, `destination_va_list`, and
  `destination_va_list_address`.
- Printer/debug output: `src/backend/prealloc/prepared_printer/variadic.cpp`
  prints these as `helper_operand kind=va_start ... dst_va_list=...`
  and `dst_va_list_addr=...`; focused printer/frame tests assert RV64 has
  complete `va_start` homes and no missing
  `helper_operand_homes.va_start.*` facts.
- Consumer: `prepared_function_to_object_function()` rejects variadic
  functions before normal instruction lowering when
  `diagnose_first_unsupported_prepared_variadic_helper()` finds any prepared
  helper. For complete `VaStart` homes it falls through to
  `rv64_variadic_helper_unsupported_diagnostic(VaStart)`, so the first
  implementation boundary is replacing this complete-`VaStart` reject with a
  target-side fragment/lowering path.
- Runtime-state gap: RV64 currently has layout and destination storage homes
  but no `overflow_area.base_slot_id` or `base_stack_offset_bytes` authority
  comparable to AAPCS64 storage attachment. The first implementation packet
  should therefore either define/materialize the RV64 overflow-area initial
  pointer state or emit a more precise unsupported diagnostic for complete
  `VaStart` homes whose overflow base is not explicit.

First helper class: complete prepared RV64 `VaStart` is the first supportable
class only if the next packet makes the overflow-area initial pointer explicit.
Without that runtime state, the first exact unsupported class is complete
`VaStart` operand homes with missing explicit RV64 overflow-area base state.

## Suggested Next

Implement Step 2 as a narrow contract packet: add focused RV64 object-emission
coverage that distinguishes complete `VaStart` homes with explicit
overflow-area initial state from complete `VaStart` homes that still lack that
state, then update the RV64 helper diagnostic/lowering boundary without
renaming the existing failure as progress.

Suggested proof command:

`cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure`

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen scalar `va_arg`, `va_copy`, external variadic call lowering, or
  general parameter-home expansion as part of this packet.
- Progress must consume prepared helper facts or emit precise diagnostics; a
  helper rename or expectation rewrite is not enough.
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

## Proof

No build required; this packet edited only `todo.md`.
Local proof run: `git diff --check -- todo.md`.
