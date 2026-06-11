Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Contract includes and direct aggregate dependency for the selected group

# Current Packet

## Just Finished

Step 3 from `plan.md`: inspected
`src/backend/mir/aarch64/codegen/traversal.cpp` for includes or direct
aggregate dependencies made unnecessary by the Step 2 `call_plans` projection
split.

No implementation contraction is currently safe. `traversal.cpp` still builds a
local `PreparedFunctionLookups` via `prepare::make_prepared_function_lookups`
and keeps it live for unselected fields:
`address_materializations`, `move_bundles`, `value_homes`, and the aggregate
`prepared_lookups` pointer consumed downstream.

## Suggested Next

Proceed to Step 4 for the selected `call_plans` group: update or verify the
matching call-plan consumer path to rely on `FunctionLoweringContext`'s narrow
`call_plan_lookups` projection without widening into other lookup groups.

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal until a later packet
  splits the remaining unselected fields: `address_materializations`,
  `move_bundles`, `value_homes`, and aggregate `prepared_lookups` consumers.
- Do not migrate or contract `return_chains` through this runbook.
- Do not create a renamed aggregate or generic BIR lowering-plan facade.
- Do not edit `ideas/open/175_prepared_function_lookups_aggregate_privacy.md`
  for routine execution notes.
- Do not select `return_chains`; that group remains rejected for this runbook.
- Treat the AArch64 fixture aggregate-field assignments as test harness
  compatibility unless the supervisor explicitly delegates test cleanup.
- The direct `../../../prealloc/calls.hpp` include remains justified by
  `prepare::make_prepared_call_plan_lookups`; removing it would rely on the
  transitive `prepared_lookups.hpp` include through `traversal.hpp`.

## Proof

Supervisor-selected proof passed and was captured in `test_after.log`:

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' --output-on-failure
```

Result: rebuilt both delegated test binaries and ran
`backend_aarch64_instruction_dispatch` plus `backend_aarch64_call_boundary_owner`;
both tests passed. The proof log is `test_after.log`.
