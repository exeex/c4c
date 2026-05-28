Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fold Thin Prepared Lookup Wrappers

# Current Packet

## Just Finished

Step 2 - Fold Thin Prepared Lookup Wrappers completed the assigned
`calls.cpp` sub-slice: replaced all remaining
`find_value_home(context, ...)` wrapper uses there with direct
`prepare::find_indexed_prepared_value_home` owner API calls.

`calls.cpp` now has no remaining `find_value_home(context, ...)` wrapper call
sites. Prepared value-id lookups use `context.function.value_home_lookups` plus
`context.function.value_locations`; value-name lookups also pass
`context.function.regalloc`.

## Suggested Next

Continue Step 2 with supervisor-selected acceptance review or the next bounded
wrapper fold-back outside `calls.cpp` if assigned.

## Watchouts

This packet did not touch `dispatch_lookup.hpp`/`.cpp`, and the
`find_value_home` wrappers remain available for non-owned call sites. The
folded `calls.cpp` sites preserve the existing unresolved-name,
optional-value-id, `value_locations`, and missing-home branches; no raw BIR
scans or new AArch64 semantic lookup policy were introduced. `clang-format` is
not installed in this environment, so no formatter pass was run.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
