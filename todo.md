Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Variadic Entry State and `va_list` Layout

# Current Packet

## Just Finished

Step 2 completed the narrow RV64 target ABI publication boundary in
`populate_variadic_entry_plans`: RV64 variadic functions now publish concrete
entry/layout facts for an overflow-area-backed `va_list` without publishing
helper resources or helper operand homes. The RV64 layout is explicit as an
8-byte, 8-byte-aligned `va_list` with one `overflow_arg_area` pointer field.
All RV64 ABI spellings in this repo (`lp64`, `lp64f`, and `lp64d`) use the new
branch.

Focused tests now assert that RV64/no-helper plans no longer carry generic
`target_abi.variadic_entry_state` or `target_abi.va_list_layout` missing facts,
while RV64/helper plans keep the helper-resource and helper-operand-home
missing facts explicit. The RV64 object diagnostic was adjusted only because
the previous fallback could no longer truthfully consume the now-complete
prepared target ABI facts.

Representative outcomes:

- `src/20030914-2.c` no longer reports generic target ABI missing facts; it now
  reaches `RV64 object variadic function lowering is not implemented`.
- `src/920908-1.c` no longer reports generic target ABI missing facts and still
  reports `helper_resources.scratch_register_count`,
  `helper_resources.scratch_stack_bytes`,
  `helper_operand_homes.va_start.destination_va_list`,
  `helper_operand_homes.va_start.destination_va_list_address`,
  `helper_operand_homes.va_arg_aggregate.source_va_list`,
  `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`, and
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.

## Suggested Next

Start the next packet on RV64 helper resource authority for variadic helpers:
publish RV64-owned `helper_resources.scratch_register_count` and
`helper_resources.scratch_stack_bytes` for the current helper set without
attempting `va_start` or `va_arg` operand-home materialization yet.

## Watchouts

Keep the next packet semantic and target-wide: do not make the representative
GCC cases pass by expectation changes or testcase-shaped shortcuts. Helper
operand homes should remain missing until their own packet owns RV64
destination/source homes and aggregate access plans. `src/20030914-2.c` is now
past the target ABI publication gate and blocked by missing RV64 object
variadic function lowering, not missing prepared layout facts.

## Proof

Ran the delegated command exactly and preserved `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_riscv_object_emission)$' && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-entry-layout-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-entry-layout-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

Build and the four focused CTest targets passed. The two representative RV64
allowlist cases still fail as expected for later packets, with the residual
diagnostics recorded above.
