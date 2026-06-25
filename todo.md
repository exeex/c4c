Status: Active
Source Idea Path: ideas/open/365_rv64_va_start_overflow_base_state_production.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Overflow Base State in Prepared Facts

# Current Packet

## Just Finished

Completed the validation/classification subpacket for `plan.md` Step 3 for
`ideas/open/365_rv64_va_start_overflow_base_state_production.md`.

- Rebuilt `backend_prepare_frame_stack_call_contract_test`,
  `backend_prepared_printer_test`, `backend_riscv_object_emission_test`, and
  `c4cll`.
- Reran the focused prepared-contract/object-emission subset; all three focused
  tests passed.
- Reran the representative RV64 allowlist case `src/920908-1.c`; it still
  fails, but no longer with the missing overflow-area base-state diagnostic.
- Representative classification: normal preparation now reaches RV64 object
  emission with the overflow-area base state present. The remaining failure is a
  later helper/lowering boundary:
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires
  destination va_list address in a prepared GPR home`.

## Suggested Next

Step 4/lifecycle direction: treat the overflow-base-state producer as validated
for helper-bearing preparation and decide whether to close this focused idea or
split a separate initiative for the later RV64 `va_start` destination-address
prepared-GPR-home boundary. A next execution packet should target that boundary
only if the supervisor/plan owner keeps it inside the active lifecycle scope.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-364 object-emission helper lowering except for narrow
  consumer adjustments required by a finalized producer contract.
- Do not broaden into external variadic call lowering, general parameter-home
  expansion, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- `base_stack_offset_bytes` remains a `std::size_t`; Step 2 defines the RV64
  producer convention as a non-negative prepared frame-slot offset from the
  stable stack base. The existing RV64 consumer still enforces signed 12-bit
  immediate materializability separately.
- The representative residual is not the old
  `requires prepared overflow-area initial base state` diagnostic. It is a
  separate prepared destination-address/home requirement and should not be
  reported as overflow-base-state producer failure.

## Proof

Supervisor-selected proof completed and is preserved in `test_after.log`.

Command run exactly:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test c4cll && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log && printf 'src/920908-1.c\n' > /tmp/rv64-overflow-base-representative.txt && (CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-overflow-base-representative.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh >> test_after.log 2>&1 || true)
```

Result: build succeeded; `backend_prepare_frame_stack_call_contract`,
`backend_prepared_printer`, and `backend_riscv_object_emission` all passed.
The representative scan appended one expected failure for `src/920908-1.c`;
details are in
`build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`.
