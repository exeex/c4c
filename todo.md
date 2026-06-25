Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize `va_start` Helper Resources and Operand Homes

# Current Packet

## Just Finished

Step 3 completed the resource-authority subpacket for RV64 variadic helper
plans. `populate_variadic_entry_plans` now publishes RV64-owned helper scratch
resources for the current helper family (`va_start`, scalar `va_arg`,
aggregate `va_arg`, and `va_copy`) without materializing any helper operand
homes or access plans. RV64 helper plans now carry
`helper_resources.scratch_register_count=3` and
`helper_resources.scratch_stack_bytes=0`, and the helper-resource missing facts
are removed.

Focused prepared-printer and frame-stack-call tests now assert concrete RV64
helper resources while preserving the intended missing facts for
`va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` operand homes
and access plans. AAPCS64 behavior and generic non-RV64/non-AAPCS64 behavior
were left unchanged.

Representative outcomes:

- `src/20030914-2.c` still reaches `RV64 object variadic function lowering is
  not implemented`, with no helper-resource or operand-home missing facts in
  that no-helper representative case.
- `src/920908-1.c` no longer reports `helper_resources.*` missing facts. It
  still reports `helper_operand_homes.va_start.destination_va_list`,
  `helper_operand_homes.va_start.destination_va_list_address`,
  `helper_operand_homes.va_arg_aggregate.source_va_list`,
  `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`, and
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.

## Suggested Next

Start the next packet on RV64 `va_start` operand-home materialization:
publish `helper_operand_homes.va_start.destination_va_list` and
`helper_operand_homes.va_start.destination_va_list_address` from prepared state
without attempting scalar or aggregate `va_arg` access plans yet.

## Watchouts

Keep the next packet semantic and target-wide: do not make the representative
GCC cases pass by expectation changes or testcase-shaped shortcuts. The RV64
resource count is deliberately helper-family authority only; helper operand
homes remain missing until the packet that owns each destination/source home
and aggregate/scalar access plan. `src/920908-1.c` is now blocked by
`va_start` and aggregate `va_arg` operand facts, not by helper resources.

## Proof

Ran the delegated command exactly and preserved `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_riscv_object_emission)$' && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-helper-resource-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-helper-resource-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

Build and the four focused CTest targets passed. The two representative RV64
allowlist cases still fail as expected for later packets, with the residual
diagnostics recorded above. `test_after.log` is the preserved proof log.
