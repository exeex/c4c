Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm selected move-bundle group

# Current Packet

## Just Finished

Lifecycle reset after Step 4 completed the `value_homes` group. The selected
next field group is `move_bundles`.

## Suggested Next

Execute Step 1 from `plan.md`: confirm the selected move-bundle group remains
the narrowest safe continuation and that the standalone projection must
preserve after-call result lane binding publication.

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal while unselected fields
  still require aggregate consumers.
- Do not fold `memory_accesses`, `edge_publications`,
  `edge_publication_source_producers`, or `return_chains` into the
  move-bundle packet.
- Preserve after-call result lane binding behavior when moving
  `FunctionLoweringContext::move_bundle_lookups` off the aggregate field.
- Do not migrate target value-location, storage-plan, register-allocation, call
  ABI, or move-record policy into BIR.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

No proof has been run for the move-bundle packet yet. The previous
`value_homes` Step 4 proof is recorded in `test_after.log`.
