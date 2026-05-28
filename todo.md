Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fold Thin Prepared Lookup Wrappers

# Current Packet

## Just Finished

Step 2 - Fold Thin Prepared Lookup Wrappers completed a narrow mechanical
sub-slice in `dispatch.cpp`: replaced the remaining
`find_value_home(context, <ValueNameId>)` use sites with direct
`prepare::find_indexed_prepared_value_home` owner API calls using
`context.function.value_home_lookups`, `context.function.regalloc`, and
`context.function.value_locations`.

`dispatch.cpp` now has no remaining `find_value_home(context, ...)`
wrapper call sites.

## Suggested Next

Continue Step 2 with supervisor-selected acceptance review or the next bounded
wrapper fold-back outside `dispatch.cpp` if assigned.

## Watchouts

This packet did not touch `dispatch_lookup.hpp`/`.cpp`, and the
`find_value_home` wrappers remain available for non-owned call sites. The
folded name-id site preserves unresolved-name and missing-home behavior by
keeping the existing `prepared_named_value_id` gating and null-home check.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
