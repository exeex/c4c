Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fold Thin Prepared Lookup Wrappers

# Current Packet

## Just Finished

Step 2 - Fold Thin Prepared Lookup Wrappers completed a narrow mechanical
sub-slice in `dispatch_edge_copies.cpp`: replaced all remaining
`find_value_home(context, ...)` wrapper uses there with direct
`prepare::find_indexed_prepared_value_home` owner API calls. The
`move.from_value_id` and `move.to_value_id` sites now use the
`PreparedValueId` overload shape with `context.function.value_home_lookups`
and `context.function.value_locations`; the resolved-name site now uses the
`ValueNameId` overload shape with `context.function.regalloc`.

`dispatch_edge_copies.cpp` now has no remaining `find_value_home(context, ...)`
wrapper call sites.

## Suggested Next

Continue Step 2 with supervisor-selected acceptance review or the next bounded
wrapper fold-back outside `dispatch_edge_copies.cpp` if assigned.

## Watchouts

This packet did not touch `dispatch_lookup.hpp`/`.cpp`, and the
`find_value_home` wrappers remain available for non-owned call sites. The
folded name-id site preserves unresolved-name and missing-home behavior by
keeping the existing `prepared_named_value_id` gating and null-home check; the
prepared-id edge-copy sites preserve the existing null-home checks and do not
add raw BIR scans or new AArch64 semantic lookup policy.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
