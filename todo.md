Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Project move-bundle lookups separately

# Current Packet

## Just Finished

Step 1 confirmed `move_bundles` as the selected aggregate field group.
Production traversal still reads `prepared_lookups.move_bundles` only to set
`FunctionLoweringContext::move_bundle_lookups`.

`prepare::make_prepared_move_bundle_lookups(...)` builds the base move-bundle
indexes from value locations: bundle lookup by phase/block/instruction,
before-call argument moves by call ABI lane, and before-return ABI moves by
source value/register bank.

The standalone Step 2 projection must preserve the current
`publish_prepared_after_call_result_lane_bindings(...)` behavior. That
publication is now performed inside `make_prepared_function_lookups(...)` after
the base move-bundle lookup object is built, and
`module::find_prepared_after_call_result_lane_binding(...)` queries those
published records through `FunctionLoweringContext::move_bundle_lookups`.

## Suggested Next

Execute Step 2 from `plan.md`: build a standalone
`prepare::PreparedMoveBundleLookups` projection in traversal, including the
same after-call result lane binding publication currently attached by
`make_prepared_function_lookups(...)`, then point
`FunctionLoweringContext::move_bundle_lookups` at that local projection.

## Watchouts

- Keep `PreparedFunctionLookups` available in traversal while unselected fields
  still require aggregate consumers.
- Direct publication/source-producer, memory-access, and return-chain consumers
  remain outside this packet and still have live AArch64 aggregate reads.
- Preserve after-call result lane binding behavior when moving
  `FunctionLoweringContext::move_bundle_lookups` off the aggregate field.
- Do not migrate target value-location, storage-plan, register-allocation, call
  ABI, or move-record policy into BIR.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

No build or ctest was required for this inventory-only Step 1 packet. No proof
logs were created or modified.
