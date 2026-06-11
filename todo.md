Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove the selected route subset and decide the next lifecycle move

# Current Packet

## Just Finished

Step 4 from `plan.md`: reran the selected `call_plans` projection proof after
the Step 2 projection split and Step 3 dependency check. The selected subset is
green.

Selected field group result: `call_plans` now uses the narrow
`FunctionLoweringContext::call_plan_lookups` projection built by
`prepare::make_prepared_call_plan_lookups(...)` in traversal. The production
`prepared_lookups.call_plans` aggregate field read was removed.

Step 3 found no additional include/dependency contraction safe yet because
`traversal.cpp` still builds and retains `PreparedFunctionLookups` for
unselected fields: `address_materializations`, `move_bundles`, `value_homes`,
and downstream aggregate `prepared_lookups` consumers.

## Suggested Next

Plan-owner lifecycle decision: either continue this runbook by selecting the
next aggregate field group, or retire/split if the remaining fields need a
fresh route review.

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

Supervisor-selected Step 4 proof passed and was captured in `test_after.log`:

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' --output-on-failure
```

Result: `backend_aarch64_instruction_dispatch` and
`backend_aarch64_call_boundary_owner` both passed. The proof log is
`test_after.log`.
