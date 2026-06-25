Status: Active
Source Idea Path: ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the Prepared Destination-Address Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for
`ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md`.

Changed files:

- `src/backend/prealloc/variadic_entry_plans.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `todo.md`

Defined and implemented the RV64 prepared `va_start` destination-address
contract in the producer path. For RV64 `va_start`, the original pointer
operand home is now published as `destination_va_list_address` only when it is
a prepared register home, while `materialize_va_start_destination_home()`
continues to allocate the synthetic destination `va_list` stack storage and
publishes that stack slot as `destination_va_list`. The RV64 object-emission
consumer stayed unchanged and still requires an explicit prepared GPR home for
the helper store base.

Focused tests now assert the distinction directly: the prepared-contract test
requires `destination_va_list` to be a stack slot and
`destination_va_list_address` to be a register with a concrete register name;
the prepared-printer test checks the same distinction in dump output with
`dst_va_list=ap:stack_slot` and `dst_va_list_addr=ap:register:reg=`.

## Suggested Next

Step 3 should run the representative RV64 path or next narrow executor packet
that proves the producer-published GPR address is consumed by real prepared
object emission for `src/920908-1.c` without changing object-emission
diagnostics or allowlists.

```sh
cmake --build build --target c4c && scripts/run_case.sh rv64_gcc_c_torture_backend src/920908-1.c
```

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-365 overflow-area base-state production.
- Do not broaden into general parameter-home expansion, external variadic call
  lowering, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- This packet intentionally rejects non-register RV64 `va_start` pointer homes
  by leaving the address fact absent rather than weakening the object consumer.
- AAPCS64 still uses the shared `materialize_va_start_destination_home()` path
  as before; this packet only changes RV64 helper operand publication.
- `src/backend/mir/riscv/codegen/object_emission.cpp` was not touched.

## Proof

Proof command:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed; `test_after.log` contains 3/3 passing tests.
