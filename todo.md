Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm selected value-home group

# Current Packet

## Just Finished

Step 1 from `plan.md`: confirmed `value_homes` remains the selected safe field
group. Scan evidence:

- `src/backend/mir/aarch64/codegen/traversal.cpp` still reads
  `prepared_lookups.value_homes` only to assign
  `FunctionLoweringContext::value_home_lookups`.
- `prepare::make_prepared_value_home_lookups(...)` is a narrow builder over
  `PreparedValueLocationFunction::value_homes`; it only fills `homes_by_id` and
  `value_ids`.
- `move_bundles` remains out of scope for the next packet because equivalent
  construction is not just value-home lookup wiring:
  `make_prepared_function_lookups(...)` builds move-bundle lookups separately
  and then calls `publish_prepared_after_call_result_lane_bindings(...)`.

## Suggested Next

Execute Step 2 from `plan.md`: add the local
`prepare::make_prepared_value_home_lookups(...)` construction in traversal and
wire `FunctionLoweringContext::value_home_lookups` to the standalone lookup
object, without changing `PreparedFunctionLookups::move_bundles` or other
aggregate fields.

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

No build proof required for this confirmation-only packet; scan evidence is
recorded above.

Step 2 proof command:

```sh
(cmake --build build --target backend_aarch64_operand_resolution_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_operand_resolution|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```
