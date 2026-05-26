Status: Active
Source Idea Path: ideas/open/21_x86_prepared_edge_publication_consumer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the x86 Shared-Publication Consumer Helper

# Current Packet

## Just Finished

Completed Step 2 by adding `x86::prepared::consume_edge_publication_move_intent`.
The helper consumes `ConsumedPlans::shared_function_lookups()` and reads
`PreparedFunctionLookups::edge_publications` through
`prepare::find_unique_indexed_prepared_edge_publication`, returning an x86 move
intent for the prepared `entry -> join` / `PreparedValueId{5}` edge without
rediscovering edge semantics locally.

## Suggested Next

Wire Step 3 by making the x86 lowering edge-copy path call the new
`consume_edge_publication_move_intent` helper for block-entry publications,
while keeping the shared prepared lookup as the only edge-publication authority.

## Watchouts

- The helper currently supports the proven stack-slot source to register
  destination form and reports missing shared lookup, missing publication,
  unsupported source home, or unsupported destination home explicitly.
- Step 3 should reuse the helper output rather than reconstructing predecessor,
  successor, destination value, source home, or destination home locally.
- Do not move target-local x86 operand spelling into shared prepare.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
