Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Project address-materialization lookups separately

# Current Packet

## Just Finished

Step 1 from `plan.md`: selected `address_materializations` as the next
aggregate field group for idea 175.

Selection basis: after the completed `call_plans` packet, traversal still only
reads `prepared_lookups.address_materializations` to populate the existing
`FunctionLoweringContext::address_materialization_lookups` pointer. The narrow
builder `prepare::make_prepared_address_materialization_lookups(...)` already
exists, so this follows the same target-local projection pattern as the
completed `call_plans` group.

## Suggested Next

Executor packet: perform Step 2 by building a local
`PreparedAddressMaterializationLookups` projection in AArch64 traversal and
assigning `function_context.address_materialization_lookups` from that
projection instead of from `prepared_lookups.address_materializations`.

Suggested narrow proof command for the supervisor to confirm or adjust:

```sh
cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure
```

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal for unselected fields.
- Do not change `move_bundles`, `value_homes`, `memory_accesses`,
  `edge_publications`, `edge_publication_source_producers`, or `return_chains`
  in this packet.
- Do not migrate target addressing/layout payloads into BIR.
- Do not remove `prepared_lookups.hpp` exposure while traversal and other
  AArch64 consumers still need unselected aggregate fields.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

Lifecycle-only reset. No code validation was run by plan-owner.
