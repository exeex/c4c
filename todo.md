Status: Active
Source Idea Path: ideas/open/142_value_home_move_bundle_lookup_ownership.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Value-Home Lookup APIs To Value-Location Ownership

# Current Packet

## Just Finished

Step 2 moved the public value-home lookup API declarations to value-location
ownership.

`PreparedValueHomeLookups` and `make_prepared_value_home_lookups` now live in
`src/backend/prealloc/value_locations.hpp` beside the existing inline indexed
value-home/id helpers. `src/backend/prealloc/prepared_lookups.hpp` no longer
declares those value-home lookup APIs directly, but its existing
`value_locations.hpp` include preserves include compatibility for current
prepared-lookup consumers.

`PreparedFunctionLookups` aggregate wiring and the
`make_prepared_value_home_lookups` definition in
`src/backend/prealloc/prepared_lookups.cpp` were left behaviorally unchanged.

## Suggested Next

Execute Step 3 from `plan.md`: move the move-bundle lookup API declarations to
value-location ownership while keeping aggregate construction and after-call
lane binding publication intact.

## Watchouts

- This slice only moved value-home lookup public declarations; do not mix
  current-block entry-publication or move-bundle ownership into its commit.
- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`, return-chain
  lookups, and residual prepared lookup APIs, and it still includes
  `value_locations.hpp` for compatibility.
- `value_locations.hpp` now includes `<unordered_map>` for
  `PreparedValueHomeLookups`.
- No direct consumers needed explicit include changes during this packet.

## Proof

Ran `cmake --build --preset default` from the repo root: passed.

Ran `ctest --test-dir build -R '^backend_' --output-on-failure > test_after.log 2>&1`
from the repo root: passed, `100% tests passed, 0 tests failed out of 179`.

Proof log: `test_after.log`.
