Status: Active
Source Idea Path: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire Contract Through LIR, BIR, and Prepared Output

# Current Packet

## Just Finished

Completed Step 3 by wiring the prepared non-AAPCS64 variadic missing-fact
contract into the RV64 object-route admission diagnostic without adding
variadic lowering.

- RV64 object emission still rejects defined variadic functions at admission
  with the recognizable `unsupported_function_admission` prefix.
- When the prepared variadic entry plan has missing facts, the diagnostic now
  appends the stable `missing_required_facts` list from the prepared contract.
- If the plan or fact list is absent, the diagnostic stays useful with a
  generic missing-contract note.
- Added a focused synthetic RV64 object failure case proving the CLI diagnostic
  includes `target_abi.variadic_entry_state` and `target_abi.va_list_layout`.

## Suggested Next

Start the next packet by letting the supervisor decide whether Step 3 has enough
CLI/object-route coverage to move on, or whether a separate helper-consumption
diagnostic packet is needed once helper-bearing variadic sources reach the
intended consumer boundary.

## Watchouts

The focused CLI fixture intentionally avoids `va_start`/`va_arg`; a helper
source currently fails earlier in semantic `lir_to_bir` load-local lowering and
does not reach RV64 object admission. Do not weaken expectations or route helper
cases through testcase-specific shortcuts. This slice did not add RV64 variadic
lowering.

## Proof

Ran the delegated proof command exactly and preserved `test_after.log`.

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_cli_riscv64_unsupported_global_diagnostic_obj|backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic|backend_riscv_object_emission|backend_prepared_printer|backend_prepare_frame_stack_call_contract)$'; } > test_after.log 2>&1 || true
```

The build completed and the delegated CTest subset passed 6/6:
`backend_cli_riscv64_variadic_entry_missing_contract_obj`,
`backend_cli_riscv64_unsupported_global_diagnostic_obj`,
`backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic`,
`backend_riscv_object_emission`, `backend_prepared_printer`, and
`backend_prepare_frame_stack_call_contract`.

Proof log: `test_after.log`.
