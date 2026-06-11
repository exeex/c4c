Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Project value-home lookups separately

# Current Packet

## Just Finished

Step 2 from `plan.md`: projected value-home lookups separately in
`src/backend/mir/aarch64/codegen/traversal.cpp`.

- Added a local `prepared_value_home_lookups` built with
  `prepare::make_prepared_value_home_lookups(function_context.value_locations)`.
- Wired `FunctionLoweringContext::value_home_lookups` to that standalone
  projection.
- Left `prepared_lookups` and `move_bundle_lookups` in place for unselected
  aggregate fields.

## Suggested Next

Execute Step 3 from `plan.md`: inspect whether the value-home projection split
made any selected direct aggregate dependency or include unnecessary. Leave
`move_bundles` alone until its after-call result lane binding publication path
is explicitly owned.

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal while unselected fields
  still require `prepared_lookups` or `move_bundles`.
- Do not select `move_bundles` inside this packet; aggregate-equivalent
  move-bundle construction also publishes after-call result lane bindings.
- Do not change `memory_accesses`, `edge_publications`,
  `edge_publication_source_producers`, or `return_chains` without an explicit
  packet that owns those dependencies.
- Do not migrate target value-location, storage-plan, register-allocation, or
  move-record policy into BIR.
- Do not remove `prepared_lookups.hpp` exposure while traversal and other
  AArch64 consumers still need unselected aggregate fields.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

Delegated Step 2 proof passed. Proof log: `test_after.log`.

```sh
(cmake --build build --target backend_aarch64_operand_resolution_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_operand_resolution|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```
