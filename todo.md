Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove the selected address-materialization subset and decide next

# Current Packet

## Just Finished

Step 4 from `plan.md`: reran the selected address-materialization projection
proof after the Step 2 projection split and Step 3 dependency check. The
selected subset is green.

Selected field group result: `address_materializations` now uses the narrow
`FunctionLoweringContext::address_materialization_lookups` projection built by
`prepare::make_prepared_address_materialization_lookups(...)` in traversal. The
production `prepared_lookups.address_materializations` aggregate field read was
removed.

Step 3 found no additional include/dependency contraction safe yet because
`traversal.cpp` still builds and retains `PreparedFunctionLookups` for
unselected fields: `move_bundles`, `value_homes`, and downstream aggregate
`prepared_lookups` consumers.

## Suggested Next

Plan-owner lifecycle decision: either continue this runbook by selecting the
next aggregate field group, or retire/split if the remaining fields need a
fresh route review.

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

Supervisor-selected Step 4 proof passed and was captured in `test_after.log`.

```sh
(cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_aarch64_prepared_memory_operand_records` and
`backend_aarch64_instruction_dispatch` both passed.
