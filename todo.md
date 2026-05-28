Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fold Thin Prepared Lookup Wrappers

# Current Packet

## Just Finished

Step 2 - Fold Thin Prepared Lookup Wrappers completed the assigned
`memory.cpp` sub-slice: replaced all remaining
`find_value_home(context, <ValueNameId>)` wrapper uses there with direct
`prepare::find_indexed_prepared_value_home` owner API calls using
`context.function.value_home_lookups`, `context.function.regalloc`, and
`context.function.value_locations`.

`memory.cpp` now has no remaining `find_value_home(context, ...)` wrapper
call sites.

## Suggested Next

Continue Step 2 with supervisor-selected acceptance review or the next bounded
wrapper fold-back outside `memory.cpp` if assigned.

## Watchouts

This packet did not touch `dispatch_lookup.hpp`/`.cpp`, and the
`find_value_home` wrappers remain available for non-owned call sites. The
folded `memory.cpp` sites preserve unresolved-name gating and missing-home
behavior by keeping the existing `prepared_named_value_id`, `value_locations`,
optional-name, and null-home checks where they already existed; no raw BIR
scans or new AArch64 semantic lookup policy were introduced.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
