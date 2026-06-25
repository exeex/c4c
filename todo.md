Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize `va_start` Helper Resources and Operand Homes

# Current Packet

## Just Finished

Step 3 completed the `va_start` operand-home subpacket for RV64 variadic helper
plans. `populate_variadic_entry_plans` now publishes RV64 `va_start`
`destination_va_list` and `destination_va_list_address` helper operand homes
from prepared value locations and target-owned va_list layout facts, using an
RV64-tagged stack slot for the materialized destination address. The existing
AAPCS64 `va_start` behavior now uses the same generic materialization helper
with its own ABI tag.

Focused prepared-printer and frame-stack-call tests now assert one complete
RV64 `va_start` helper operand-home carrier while preserving the intended
missing facts for scalar `va_arg`, aggregate `va_arg`, and `va_copy` operand
homes/access plans. AAPCS64 behavior and generic non-RV64/non-AAPCS64 behavior
were left unchanged.

Representative outcomes:

- `src/20030914-2.c` still reaches `RV64 object variadic function lowering is
  not implemented`, with no helper-resource or operand-home missing facts in
  that no-helper representative case.
- `src/920908-1.c` no longer reports `helper_resources.*` missing facts or
  `helper_operand_homes.va_start.*` missing facts. It still reports
  `helper_operand_homes.va_arg_aggregate.source_va_list`,
  `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`, and
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.

## Suggested Next

Start the next packet on RV64 aggregate `va_arg` operand-home/access-plan
materialization for overflow-only aggregate cases: publish
`helper_operand_homes.va_arg_aggregate.source_va_list`,
`helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`, and the
RV64 aggregate access plan without attempting scalar `va_arg` or `va_copy` yet.

## Watchouts

Keep the next packet semantic and target-wide: do not make the representative
GCC cases pass by expectation changes or testcase-shaped shortcuts. The RV64
`va_start` address home uses RV64-tagged stack objects and should not inherit
AAPCS64 register-save/HFA semantics. `src/920908-1.c` is now blocked by
aggregate `va_arg` operand/access facts, not by helper resources or `va_start`.
Scalar `va_arg` and `va_copy` missing facts remain explicit in the focused
helper-family tests for later packets.

## Proof

Ran the delegated command exactly and preserved `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_riscv_object_emission)$' && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-va-start-homes-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-va-start-homes-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

Build and the four focused CTest targets passed. The two representative RV64
allowlist cases still fail as expected for later packets, with the residual
diagnostics recorded above. `test_after.log` is the preserved proof log.
