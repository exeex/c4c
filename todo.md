Status: Active
Source Idea Path: ideas/open/142_value_home_move_bundle_lookup_ownership.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Core Move-Bundle Lookup APIs To Value-Location Ownership

# Current Packet

## Just Finished

Step 3 moved the public core move-bundle lookup API declarations to
value-location ownership.

`PreparedMoveBundleLookups`, `prepared_move_bundle_position_key`,
`make_prepared_move_bundle_lookups`, and `find_indexed_prepared_move_bundle`
now live publicly in `src/backend/prealloc/value_locations.hpp`.
`src/backend/prealloc/prepared_lookups.hpp` no longer declares those core
move-bundle lookup APIs directly, while its existing `value_locations.hpp`
include preserves include compatibility for current prepared-lookup consumers.

The after-call result-lane binding declaration remains in
`src/backend/prealloc/prepared_lookups.hpp`. `PreparedMoveBundleLookups` uses
out-of-line special members defined in
`src/backend/prealloc/prepared_lookups.cpp` so `value_locations.hpp` can own
the lookup type without moving result-lane binding ownership.

## Suggested Next

Execute Step 4 from `plan.md`: make the remaining ABI helper, after-call
result-lane binding, current-block entry publication, and related residual
prepared lookup ownership decisions without changing test expectations.

## Watchouts

- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`, return-chain
  lookups, ABI helper declarations, after-call result-lane binding
  declarations, and current-block entry publication declarations.
- `PreparedMoveBundleLookups` still contains after-call/result-lane lookup
  fields; only the result-lane binding declaration itself was left in
  `prepared_lookups.hpp` for Step 4.
- The self-contained include probe for `value_locations.hpp` required
  out-of-line `PreparedMoveBundleLookups` special members because the
  result-lane binding type is intentionally forward-declared there.
- No direct consumers needed explicit include changes during this packet.

## Proof

Compiled a self-contained `value_locations.hpp` include probe that instantiates
`PreparedMoveBundleLookups`: passed.

Ran `cmake --build --preset default` from the repo root: passed.

Ran `ctest --test-dir build -R '^backend_' --output-on-failure > test_after.log 2>&1`
from the repo root: passed, `100% tests passed, 0 tests failed out of 179`.

Proof log: `test_after.log`.
