Status: Active
Source Idea Path: ideas/open/142_value_home_move_bundle_lookup_ownership.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Decide ABI, Result-Lane, And Publication Helper Ownership

# Current Packet

## Just Finished

Step 4 moved the remaining value-location-owned ABI/result-lane/current-block
entry publication declarations to value-location ownership.

`PreparedAfterCallResultLaneBinding`,
`prepared_after_call_result_lane_position_key`,
`prepared_before_return_abi_move_source_bank_key`,
`find_indexed_prepared_before_call_argument_move`,
`find_indexed_prepared_after_call_result_lane_binding`, and
`find_prepared_before_return_abi_move_by_source_and_destination_bank` now live
publicly in `src/backend/prealloc/value_locations.hpp` because they are lookup
keys/accessors over `PreparedMoveBundleLookups` and `PreparedValueLocationFunction`
move-bundle data.

`PreparedCurrentBlockEntryPublicationStatus`,
`prepared_current_block_entry_publication_status_name`,
`PreparedCurrentBlockEntryPublicationQueryInputs`,
`PreparedCurrentBlockEntryPublication`, and both
`find_prepared_current_block_entry_publication` overload declarations also now
live publicly in `src/backend/prealloc/value_locations.hpp` because they query
value homes plus block-entry publication collection. The current-block join
parallel-copy routing declarations remain in `src/backend/prealloc/prepared_lookups.hpp`
as out-of-scope routing state.

Return-chain declarations/helpers and target-local policy declarations were not
touched. `src/backend/prealloc/prepared_lookups.cpp` definitions remain in
place; this packet only changed declaration ownership.

## Suggested Next

Execute Step 5 from `plan.md`: update direct consumers to include the
value-location owner where they need only moved declarations, while preserving
`prepared_lookups.hpp` includes for aggregate or residual lookup users.

## Watchouts

- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`, return-chain
  lookups, call-argument/comparison/load-local producer queries, and
  current-block join-routing declarations.
- The moved declarations are still available to existing prepared-lookup
  consumers through the residual header's `value_locations.hpp` include, so no
  direct consumer include edits were required in this packet.
- `PreparedMoveBundleLookups` still keeps out-of-line special members in
  `prepared_lookups.cpp`; that remains compatible now that the result-lane
  binding type is complete in `value_locations.hpp`.

## Proof

Ran `cmake --build --preset default` from the repo root: passed.

Ran `ctest --test-dir build -R '^backend_' --output-on-failure > test_after.log 2>&1`
from the repo root: passed, `100% tests passed, 0 tests failed out of 179`.

Proof log: `test_after.log`.
