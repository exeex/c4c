Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add or expose the narrow projection for the selected group

# Current Packet

## Just Finished

Step 2 from `plan.md`: replaced the production `prepared_lookups.call_plans`
field read in `src/backend/mir/aarch64/codegen/traversal.cpp` with a local
`PreparedCallPlanLookups` projection built by
`prepare::make_prepared_call_plan_lookups(prepared, function_context.call_plans,
prepared_function)`.

`FunctionLoweringContext::call_plan_lookups` now points at that narrow local
projection. `PreparedFunctionLookups` remains available in the traversal loop
for the unselected fields.

## Suggested Next

Implement Step 3 include/dependency contraction for the selected `call_plans`
group: inspect whether `src/backend/mir/aarch64/codegen/traversal.cpp` still
needs the current aggregate-lookup include surface directly, then contract only
the includes/dependencies made unnecessary by the Step 2 projection split.

## Watchouts

- Keep `PreparedFunctionLookups` available for the unselected fields still read
  in traversal: `address_materializations`, `move_bundles`, and `value_homes`.
- Do not migrate or contract `return_chains` through this runbook.
- Do not create a renamed aggregate or generic BIR lowering-plan facade.
- Do not edit `ideas/open/175_prepared_function_lookups_aggregate_privacy.md`
  for routine execution notes.
- Do not select `return_chains`; that group remains rejected for this runbook.
- Treat the AArch64 fixture aggregate-field assignments as test harness
  compatibility unless the supervisor explicitly delegates test cleanup.

## Proof

Supervisor-selected proof passed and was captured in `test_after.log`:

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' --output-on-failure
```

Result: rebuilt `traversal.cpp`, linked both delegated test binaries, and ran
`backend_aarch64_instruction_dispatch` plus `backend_aarch64_call_boundary_owner`;
both tests passed.
