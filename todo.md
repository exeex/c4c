Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fold Thin Prepared Lookup Wrappers

# Current Packet

## Just Finished

Step 2 - Fold Thin Prepared Lookup Wrappers completed a narrow mechanical
sub-slice in `dispatch_producers.cpp`: replaced every
`find_value_home(context, <PreparedValueId>)` use with direct
`prepare::find_indexed_prepared_value_home` owner API calls using
`context.function.value_home_lookups` and `context.function.value_locations`.

The remaining `find_value_home` calls in `dispatch_producers.cpp` all use
resolved `ValueNameId` values and were left unchanged for this packet.

## Suggested Next

Continue Step 2 with another bounded wrapper fold-back, such as converting the
remaining `find_value_home(context, <ValueNameId>)` producer-file call sites to
the direct prepared lookup owner API if the supervisor assigns that scope.

## Watchouts

This packet did not touch `dispatch_lookup.hpp`/`.cpp`, and the
`find_value_home` wrappers remain available for non-owned call sites.
Preserve the two owner API overload shapes: prepared ids use
`value_home_lookups` plus `value_locations`; name ids also require `regalloc`.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
