Status: Active
Source Idea Path: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the Shared Vararg Contract

# Current Packet

## Just Finished

Completed Step 2 by publishing explicit missing required facts for
non-AAPCS64 prepared variadic entry plans instead of leaving empty placeholder
plans.

- Non-AAPCS64 variadic entry plans now record missing
  `target_abi.variadic_entry_state` and `target_abi.va_list_layout` facts.
- When recognized helper kinds are present, the same plan records missing
  helper scratch facts and helper-specific operand/access-plan facts for
  `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`.
- AAPCS64 still follows the existing concrete publication path for register-save
  area, overflow area, `va_list_layout`, helper resources, helper operand homes,
  and scalar/aggregate access plans.
- Focused prepared-printer and frame/stack/call contract tests now prove the
  non-AAPCS64 missing-fact dump/structured contract and continue to prove
  AAPCS64 concrete helper/access-plan publication.

## Suggested Next

Start the next packet by wiring the non-AAPCS64 prepared missing-fact contract
into the relevant consumer/admission diagnostic path, so unsupported variadic
entry and helper consumption failures report the prepared missing facts rather
than a generic route-level absence.

## Watchouts

The new facts are intentionally target-independent ABI-boundary names; do not
replace them with RV64 testcase names or object-emission shortcuts. The current
slice does not add RV64 lowering, does not classify `va_end`, and leaves
non-AAPCS64 helper operand homes/access plans empty until a target publishes
concrete semantics.

## Proof

Ran the delegated proof command exactly and preserved `test_after.log`.

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$'; } > test_after.log 2>&1 || true
```

The build completed and the delegated CTest subset passed 4/4:
`backend_prepare_frame_stack_call_contract`, `backend_prepared_printer`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`, and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

Proof log: `test_after.log`.
