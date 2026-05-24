Status: Active
Source Idea Path: ideas/open/prepared-move-publication-indexing-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Name and Prove the x86 Consumption Path

# Current Packet

## Just Finished

Step 4 in `plan.md` is complete: named and proved the x86 consumption path for
the shared Prepared lookup helper.

The x86 surface is `c4c::backend::x86::consume_prepared_function_lookups(...)`,
which resolves a function's published `PreparedControlFlowFunction` and builds
`prepare::PreparedFunctionLookups` through
`prepare::make_prepared_function_lookups(...)`. `ConsumedPlans` now owns that
optional shared lookup bundle and exposes `shared_function_lookups()` plus
`shared_call_plan_lookups()` for family-specific consumption. Existing x86
call-plan consumers now route through
`prepare::find_indexed_prepared_call_plan(...)`, preserving the old prepared
call-plan scan as the helper's fallback when indexed lookups are unavailable.

## Suggested Next

Proceed to lifecycle review or the next supervisor-selected packet for this
idea. The Step 4 x86 slice did not require copying AArch64 context objects or
target policy into x86.

## Watchouts

The x86 shared lookup path currently consumes the call-plan family through
`ConsumedPlans`; address-materialization, move-bundle, and value-home family
helpers are exposed by `shared_function_lookups()` for future integration, but
this packet intentionally kept code changes to the smallest proven x86
structural path.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. The proof rebuilt the x86 consumption-path change and ran 149
matching backend tests with 0 failures. Proof log: `test_after.log`.
