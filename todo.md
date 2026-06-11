Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm selected value-home group

# Current Packet

## Just Finished

Lifecycle reset after Step 4 of the address-materialization runbook. The
completed group was `address_materializations`; production traversal no longer
reads `prepared_lookups.address_materializations`, and the selected proof in
`test_after.log` was green.

## Suggested Next

Execute Step 1 from `plan.md`: confirm `value_homes` remains the next safe
field group, then proceed to Step 2 if the narrow
`prepare::make_prepared_value_home_lookups(...)` projection still matches the
current traversal shape.

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

No proof run by plan-owner for this lifecycle-only reset. The next executor
packet should run the supervisor-selected compile and value-location/AArch64
lowering subset after code changes.
