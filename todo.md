Status: Active
Source Idea Path: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Confirm RV64 Boundary Without Target-Layer Overfit

# Current Packet

## Just Finished

Completed Step 4 by running the delegated RV64 boundary proof and classifying
the two idea 360 representatives without implementation or expectation changes.

- `src/920908-1.c` reaches RV64 object admission, not an earlier
  frontend/LIR/BIR boundary. It fails with
  `unsupported_function_admission: variadic functions are not supported by the
  RV64 object route` and reports prepared missing facts:
  `target_abi.variadic_entry_state`, `target_abi.va_list_layout`,
  `helper_resources.scratch_register_count`,
  `helper_resources.scratch_stack_bytes`,
  `helper_operand_homes.va_start.destination_va_list`,
  `helper_operand_homes.va_start.destination_va_list_address`,
  `helper_operand_homes.va_arg_aggregate.source_va_list`,
  `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`, and
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.
- `src/20030914-2.c` reaches RV64 object admission, not an earlier
  frontend/LIR/BIR boundary. It now reports the Step 3 prepared missing facts
  through admission: `target_abi.variadic_entry_state` and
  `target_abi.va_list_layout`.

## Suggested Next

Next packet should be target-hook planning rather than testcase repair:
define the RV64 target ABI hook/materialization boundary for variadic entry
state and `va_list` layout, with a separate decision on helper resource and
aggregate `va_arg` operand-home facts before any object lowering is attempted.

## Watchouts

Both representatives now prove prepared missing-fact admission rather than the
earlier helper/contract failure noted after Step 3. `src/20030914-2.c` is the
clean target-ABI-only representative. `src/920908-1.c` also reaches admission,
but carries helper resource and aggregate `va_arg` operand-home missing facts;
do not collapse those into named-case matching or a partial RV64 lowering
shortcut.

## Proof

Ran the delegated proof command exactly and preserved `test_after.log`.

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic|backend_prepared_printer|backend_prepare_frame_stack_call_contract)$' && printf 'src/920908-1.c\nsrc/20030914-2.c\n' > /tmp/rv64-vararg-boundary-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-boundary-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

The build completed and the delegated CTest subset passed 4/4:
`backend_cli_riscv64_variadic_entry_missing_contract_obj`,
`backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic`,
`backend_prepared_printer`, and
`backend_prepare_frame_stack_call_contract`.

The delegated two-case RV64 allowlist run completed with expected
classification failures: total=2 passed=0 failed=2. Case logs:
`build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log` and
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`.

Proof log: `test_after.log`.
