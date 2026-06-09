Status: Active
Source Idea Path: ideas/open/147_comparison_prealloc_fact_owner.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate And Record Proof

# Current Packet

## Just Finished

Step 4 - Validate And Record Proof completed.

Validated the completed comparison owner move with the supervisor-delegated
build plus backend CTest subset.

Final comparison owner status: fused-compare operand producer facts,
materialized-condition producer facts, and their lookup helpers are owned by
`src/backend/prealloc/comparison.hpp` with definitions in
`src/backend/prealloc/comparison.cpp`. The prepared lookup owner no longer
provides a compatibility include for those declarations, while consumers that
still dereference `PreparedFunctionLookups` remain on
`src/backend/prealloc/prepared_lookups.hpp`.

## Suggested Next

Supervisor should decide whether to call plan-owner for lifecycle
close/deactivation of the active plan, or run any milestone-level validation
before commit.

## Watchouts

- Full CTest was not required by Step 4 execution because this packet made no
  implementation edits and the completed owner move did not report shared
  branch/comparison semantic changes. Supervisor still owns any broader
  milestone validation decision.
- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`, return-chain
  helpers, edge-publication lookups, and
  `PreparedSameBlockLoadLocalStoredValueSource`; those remain separate from the
  comparison owner.
- `src/backend/mir/aarch64/codegen/comparison.cpp` still needs
  `prepared_lookups.hpp` for `PreparedFunctionLookups` member access.

## Proof

Ran the delegated proof command for Step 4:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Build completed with no work to do and the backend CTest subset
passed 179/179 tests.

Regression guard passed with 179/179 backend tests before and after, no new
failures.

Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
