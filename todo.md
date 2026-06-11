Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Project address-materialization lookups separately

# Current Packet

## Just Finished

Step 2 from `plan.md`: projected address-materialization lookups separately in
AArch64 traversal.

`lower_prepared_functions(...)` now builds a local
`PreparedAddressMaterializationLookups` with
`prepare::make_prepared_address_materialization_lookups(prepared,
prepared_function.function_name)` and assigns
`function_context.address_materialization_lookups` from that projection.
`PreparedFunctionLookups` remains available for unselected aggregate fields, but
traversal no longer reads `prepared_lookups.address_materializations` directly.

## Suggested Next

Executor packet: perform Step 3 by contracting address-materialization
dependencies so consumers prefer
`FunctionLoweringContext::address_materialization_lookups` and only keep
aggregate access where another unselected field still requires it.

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal for unselected fields.
- Step 3 should verify whether any address-materialization consumer still
  reaches the same data through `prepared_lookups` before removing or narrowing
  dependencies.
- Do not change `move_bundles`, `value_homes`, `memory_accesses`,
  `edge_publications`, `edge_publication_source_producers`, or `return_chains`
  in this packet.
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
