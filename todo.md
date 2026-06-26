Status: Active
Source Idea Path: ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Helper-Contract Coverage

# Current Packet

## Just Finished

Step 2 added focused helper-contract coverage for the RV64 overflow-area
aggregate `va_arg` boundary.

- Added `tests/backend/case/riscv64_variadic_aggregate_overflow_helper_contract.c`
  as a compact two-aggregate `va_arg` fixture that publishes one register-backed
  destination payload and one stack-slot destination payload.
- Added `backend_dump_riscv64_variadic_aggregate_overflow_helper_contract` to
  pin the prepared-printer facts: RV64 one-field `overflow_arg_area` va-list
  layout, `va_start`/`va_arg_aggregate` helper resources, source `va_list` in
  `s1`, explicit overflow access-plan sizes/strides, and current
  `destination_payload=%value/{register,stack_slot}` spellings.
- Added a forbidden prepared-dump check for concrete payload write-address
  spellings such as `destination_payload_slot`, `destination_payload_object`,
  and `payload_write_address`, so the test documents that the current contract
  has not yet gained the direct payload-address fact needed for RV64 lowering.
- Added `backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj`
  to prove the real CLI object route remains fail-closed with
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower
  va_arg_aggregate helper`.
- Extended `backend_riscv_object_emission_test` with synthetic prepared-module
  coverage for both boundaries: complete current aggregate helper facts still
  reject without lowering, and an aggregate access plan missing
  `destination_payload_home` rejects as incomplete prepared helper operand
  homes.

## Suggested Next

Proceed to the next supervisor-selected packet for the direct RV64 lowering
contract: add the missing prepared destination-payload concrete write-address
fact before attempting object-route aggregate `va_arg` emission.

## Watchouts

- Do not reopen shared LIR/BIR vararg semantics from idea 360.
- Do not reopen `va_start` destination-address or overflow-area setup from
  ideas 365 and 366.
- Do not infer aggregate layout, helper resources, or `va_list` state from
  source syntax, testcase names, raw BIR text, or diagnostics.
- Preserve fail-closed diagnostics for unsupported aggregate helper shapes.
- The new dump coverage intentionally proves the current boundary, not the final
  lowering contract. RV64 object emission should still not derive the payload
  write object by joining aggregate value names, address-selection side
  channels, and addressing records ad hoc.
- The focused fixture uses a 9-byte aggregate so both register-backed and
  stack-slot destination payload spellings appear in a compact prepared dump.

## Proof

Delegated proof passed and was written to `test_after.log`:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- CTest reported `100% tests passed, 0 tests failed out of 326`.
- The log includes the new/updated coverage:
  `backend_riscv_object_emission`,
  `backend_dump_riscv64_variadic_aggregate_overflow_helper_contract`, and
  `backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj`.
