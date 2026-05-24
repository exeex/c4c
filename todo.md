Status: Active
Source Idea Path: ideas/open/prepared-move-publication-indexing-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Harden Focused Behavioral Proof

# Current Packet

## Just Finished

Step 5 in `plan.md` is complete: added focused behavioral tests for the shared
Prepared lookup helper.

The new `backend_prepared_lookup_helper` BIR backend test builds linear and
diamond prepared-function fixtures through `prepare::make_prepared_function_lookups(...)`
and directly verifies indexed lookup behavior for call plans, address
materializations, move bundles, and value homes. The fixtures also prove
function-local filtering by checking that unrelated function data does not leak
through the shared lookup bundle.

## Suggested Next

Proceed to supervisor lifecycle review for this idea, since Step 5 behavioral
proof is now in place and the backend subset is green.

## Watchouts

This packet only added tests and CMake integration. It did not touch helper
implementation, lowering, or expectations.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. The proof rebuilt the new behavioral test and ran 150 matching
backend tests with 0 failures, including `backend_prepared_lookup_helper`.
Proof log: `test_after.log`.
