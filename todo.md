Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Contract selected direct dependency exposure

# Current Packet

## Just Finished

Step 3 from `plan.md`: inspected AArch64 traversal for dependency exposure
that became unnecessary after the address-materialization projection split.

No implementation contraction was safe in this packet. `traversal.cpp` still
needs `PreparedFunctionLookups` because `lower_prepared_functions(...)` builds
the aggregate with `prepare::make_prepared_function_lookups(...)`, assigns
`function_context.prepared_lookups`, and exposes unselected aggregate fields via
`function_context.move_bundle_lookups = &prepared_lookups.move_bundles` and
`function_context.value_home_lookups = &prepared_lookups.value_homes`. The
address-materialization consumer now uses the separate
`function_context.address_materialization_lookups` projection, but the remaining
aggregate dependencies prevent removing the aggregate construction or its header
dependency from traversal.

## Suggested Next

Executor packet: perform Step 4 by handing the remaining
`PreparedFunctionLookups` aggregate exposure to the next lifecycle route, with
the Step 3 evidence that traversal itself has no address-materialization-only
dependency left to contract safely.

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal while unselected fields
  still require `prepared_lookups`, `move_bundles`, or `value_homes`.
- `traversal.cpp` no longer reads `prepared_lookups.address_materializations`;
  further contraction would need a broader plan for the remaining aggregate
  fields, not an address-materialization-only edit.
- Do not change `memory_accesses`, `edge_publications`,
  `edge_publication_source_producers`, or `return_chains` without an explicit
  packet that owns those dependencies.
- Do not migrate target addressing/layout payloads into BIR.
- Do not remove `prepared_lookups.hpp` exposure while traversal and other
  AArch64 consumers still need unselected aggregate fields.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

Passed. Proof log: `test_after.log`.

```sh
(cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```
