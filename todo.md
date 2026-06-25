Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit RV64 Vararg Target Requirements

# Current Packet

## Just Finished

Step 1 audit completed. The delegated proof reproduced the current RV64 object
admission state for the two representative cases:

- `src/20030914-2.c` fails at RV64 variadic function admission with
  `missing_required_facts=[target_abi.variadic_entry_state,
  target_abi.va_list_layout]`.
- `src/920908-1.c` fails at the same gate with
  `missing_required_facts=[target_abi.variadic_entry_state,
  target_abi.va_list_layout, helper_resources.scratch_register_count,
  helper_resources.scratch_stack_bytes,
  helper_operand_homes.va_start.destination_va_list,
  helper_operand_homes.va_start.destination_va_list_address,
  helper_operand_homes.va_arg_aggregate.source_va_list,
  helper_operand_homes.va_arg_aggregate.aggregate_destination_payload,
  helper_operand_homes.va_arg_aggregate.aggregate_access_plan]`.

Fact ownership map:

- `target_abi.variadic_entry_state`: RV64 branch in
  `populate_variadic_entry_plans`, parallel to
  `populate_aapcs64_variadic_entry_abi_facts`; it should publish RV64 vararg
  entry/register-save/overflow state or replace the generic missing fact with a
  narrower RV64 target diagnostic.
- `target_abi.va_list_layout`: same RV64 target ABI publication boundary;
  this is the first required publication before helper plans can be trusted.
- `helper_resources.scratch_register_count`: RV64 helper resource authority,
  parallel to `populate_aapcs64_variadic_entry_helper_resource_authority`, with
  RV64-specific scratch needs per `va_start`/`va_arg_aggregate`.
- `helper_resources.scratch_stack_bytes`: same RV64 helper resource authority.
- `helper_operand_homes.va_start.destination_va_list`: RV64 helper operand-home
  authority, parallel to
  `populate_aapcs64_variadic_entry_helper_operand_home_authority`; source home
  lookup can reuse prepared value locations, but target-specific materialization
  must not use AAPCS64 slot names/layout assumptions.
- `helper_operand_homes.va_start.destination_va_list_address`: same RV64
  `va_start` operand-home authority; likely needs an RV64-owned destination
  address materializer if `va_list` is stack-backed.
- `helper_operand_homes.va_arg_aggregate.source_va_list`: RV64 aggregate
  `va_arg` operand-home authority, using prepared value locations.
- `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`: RV64
  aggregate `va_arg` operand-home authority, using prepared value locations.
- `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`: RV64 aggregate
  `va_arg` access-plan builder; should be separate from AAPCS64 HFA/register
  save area assumptions and can initially diagnose unsupported aggregate classes
  precisely.

## Suggested Next

Start Step 2 with the narrow publication boundary in
`src/backend/prealloc/variadic_entry_plans.cpp`: add an RV64-specific branch in
`populate_variadic_entry_plans` that handles only
`target_abi.variadic_entry_state` and `target_abi.va_list_layout` for variadic
functions, leaving helper resources and operand homes as explicit missing facts
when helpers are present. Focused tests should cover prepared-printer or
prepare-frame-stack-call-contract output for RV64/no-helper and RV64/helper
cases, plus the existing
`backend_cli_riscv64_variadic_entry_missing_contract_obj` admission diagnostic.

## Watchouts

- The current RV64 admission text is emitted in
  `rv64_variadic_function_admission_diagnostic`; object emission is the
  reporting consumer, not the owner of the missing facts.
- Today all non-AAPCS64 targets flow through
  `publish_missing_non_aapcs64_variadic_entry_contract`, so Step 2 should avoid
  changing generic non-AAPCS64 behavior except for the explicit RV64 target
  branch.
- Do not clone AAPCS64 register-save/HFA rules into RV64. The first slice
  should publish or precisely diagnose RV64 entry/layout facts only; aggregate
  `va_arg` access planning belongs to a later packet after helper resources and
  `va_start` homes are owned.
- `src/20030914-2.c` is the cleaner representative for the first boundary
  because it currently exposes only `target_abi.*` missing facts. `src/920908-1.c`
  should still be checked to confirm the target ABI facts disappear while the
  helper facts remain.

## Proof

Ran the delegated command exactly and preserved `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_riscv_object_emission|backend_prepared_printer|backend_prepare_frame_stack_call_contract)$' && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-target-audit-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-target-audit-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

Build and the four focused CTest targets passed. The representative RV64
allowlist scan still reports both cases as failing with the current expected
variadic object-admission diagnostics recorded above.
