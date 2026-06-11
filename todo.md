Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Contract selected direct dependency exposure

# Current Packet

## Just Finished

Step 3 from `plan.md`: inspected direct dependency exposure in
`src/backend/mir/aarch64/codegen/traversal.cpp` after the value-home projection
split.

- No further implementation contraction is safe in this packet.
- `traversal.cpp` no longer reads `prepared_lookups.value_homes`; it constructs
  `prepared_value_home_lookups` directly with
  `prepare::make_prepared_value_home_lookups(function_context.value_locations)`.
- `traversal.cpp` still must construct `PreparedFunctionLookups` because it
  assigns `function_context.prepared_lookups = &prepared_lookups` for
  unselected aggregate consumers and
  `function_context.move_bundle_lookups = &prepared_lookups.move_bundles`.
- The visible include exposure is still indirect through
  `src/backend/mir/aarch64/module/module.hpp`, which defines
  `FunctionLoweringContext` with `PreparedFunctionLookups`,
  `PreparedMoveBundleLookups`, and `PreparedValueHomeLookups` pointers.

## Suggested Next

Execute Step 4 from `plan.md`: continue with the next owned contraction or
review packet selected by the supervisor.

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

Delegated Step 3 proof passed. Proof log: `test_after.log`.

```sh
(cmake --build build --target backend_aarch64_operand_resolution_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_operand_resolution|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```
