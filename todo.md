Status: Active
Source Idea Path: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Milestone Validation and Handoff

# Current Packet

## Just Finished

Completed Step 5 milestone validation and handoff for idea 360 without
implementation, test, expectation, plan, or source-idea changes.

- The touched-layer CTest subset is green at 7/7, covering RV64 admission
  diagnostics plus BIR/prepared publication and printer surfaces.
- `src/20030914-2.c` reaches RV64 object admission with a structured
  target-ABI-only missing-fact outcome:
  `target_abi.variadic_entry_state` and `target_abi.va_list_layout`.
- `src/920908-1.c` reaches RV64 object admission with a structured outcome
  that includes the target ABI facts plus helper resource and aggregate
  `va_arg` operand-home facts:
  `target_abi.variadic_entry_state`, `target_abi.va_list_layout`,
  `helper_resources.scratch_register_count`,
  `helper_resources.scratch_stack_bytes`,
  `helper_operand_homes.va_start.destination_va_list`,
  `helper_operand_homes.va_start.destination_va_list_address`,
  `helper_operand_homes.va_arg_aggregate.source_va_list`,
  `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`, and
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.
- Remaining work is target-specific ABI hook/materialization work, not shared
  LIR/BIR/prepared contract completion.

## Suggested Next

Supervisor should send this active lifecycle state to plan-owner review for
close or follow-up routing if the shared-contract acceptance criteria are met.
Any next implementation packet should be a target ABI hook/materialization
initiative for RV64 variadic entry state, `va_list` layout, helper resources,
and aggregate `va_arg` operand homes; do not start that follow-up inside this
handoff packet.

## Watchouts

The representative RV64 allowlist still reports two failed cases, but both
failures are structured object-admission outcomes and not earlier
frontend/LIR/BIR/prepared contract loss. `src/20030914-2.c` is the clean
target-ABI-only representative. `src/920908-1.c` also reaches admission but
adds helper-resource and aggregate operand-home requirements; keep those
separate from the entry-state/`va_list` hook boundary. Do not treat the
allowlist failures as testcase repair candidates or resolve them with
named-case matching.

## Proof

Ran the delegated proof command exactly and preserved `test_after.log`.

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic|backend_prepared_printer|backend_prepare_frame_stack_call_contract|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' && printf 'src/920908-1.c\nsrc/20030914-2.c\n' > /tmp/rv64-vararg-milestone-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-milestone-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

The build completed and the delegated CTest subset passed 7/7:
`backend_cli_riscv64_variadic_entry_missing_contract_obj`,
`backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic`,
`backend_prepare_frame_stack_call_contract`, `backend_prepared_printer`,
`backend_cli_dump_bir_00204_stdarg_semantic_handoff`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`, and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

The delegated two-case RV64 allowlist run completed with expected
classification failures: total=2 passed=0 failed=2. Case logs:
`build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log` and
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`.

Proof log: `test_after.log`.
