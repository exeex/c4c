Status: Active
Source Idea Path: ideas/open/174_route_index_facade_contraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Validation

# Current Packet

## Just Finished

Step 4 contracted the redundant public direct Route 4 block-entry publication
lookup surface by removing `route4_find_block_entry_publication` from
`bir.hpp` and `bir.cpp`.

The remaining focused oracle coverage now checks the typed
`route4_validate_block_entry_publication_reference` result and the
`route_index_validate_block_entry_publication_reference` facade result directly.
The typed `Route4PublicationAvailabilityIndex`, block-entry records, prepared
helpers, current-block lookup path, and AArch64 publication paths remain
unchanged because remaining consumers and tests still require them.

## Suggested Next

Delegate Step 5 acceptance validation for the Route 4 block-entry facade
contraction slice, including any broader backend validation the supervisor
decides is required because public BIR header surface changed.

## Watchouts

- `rg` and AST caller checks found no source consumers for
  `route4_find_block_entry_publication`; remaining direct uses were the focused
  BIR tests that could assert through the validator instead.
- Do not remove `route4_find_current_block_publication`: `src/backend/mir/query.cpp`
  still uses it for the nearby current-block path.
- Do not remove typed Route 4 index records or prepared block-entry helpers:
  tests and backend publication paths still use them as semantic owners or
  prepared emission-readiness oracles.

## Proof

Proof passed for Step 4:
`(cmake --build build --target backend_prepared_lookup_helper_test backend_prealloc_block_entry_publications_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' --output-on-failure) > test_after.log 2>&1`.
Canonical proof log: `test_after.log`.
