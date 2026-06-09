Status: Active
Source Idea Path: ideas/open/143_stack_layout_id_lookup_helpers_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Helper Declarations, Definitions, And Consumers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/143_stack_layout_id_lookup_helpers_owner.md`.

## Suggested Next

Execute Step 1 from `plan.md`: locate `find_frame_slot_by_id` and
`find_stack_object_by_id`, map their declarations, definitions, and consumers,
inspect stack-layout implementation-file conventions, and record the chosen
implementation owner here before editing code.

## Watchouts

- This active idea is an ownership move only; do not change stack layout,
  frame-address, addressing, memory retargeting, comparison, or AArch64
  wrapper behavior.
- Do not use this route for broad residual include cleanup; idea 149 owns that
  after narrower owner moves land.
- Keep `prepared_lookups.hpp` includes where a consumer still needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or another
  residual prepared lookup API.
- Do not make stack id lookup target-specific.

## Proof

Activation only; no build proof required yet.
