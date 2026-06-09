Status: Active
Source Idea Path: ideas/open/147_comparison_prealloc_fact_owner.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Lookup Helpers And Consumers

# Current Packet

## Just Finished

Step 3 - Move Lookup Helpers And Consumers completed.

Removed the temporary compatibility include of `comparison.hpp` from
`src/backend/prealloc/prepared_lookups.hpp`.

Kept unrelated prepared lookup consumers on `prepared_lookups.hpp`.
`src/backend/mir/aarch64/codegen/comparison.cpp` already includes
`../../../prealloc/comparison.hpp` for the moved comparison facts/helpers and
still keeps `../../../prealloc/prepared_lookups.hpp` because it dereferences
`PreparedFunctionLookups` through `context.function.prepared_lookups`.

Added an explicit `src/backend/prealloc/comparison.hpp` include to
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, the remaining
direct consumer that calls the moved fused-compare helper while also testing
other prepared lookup APIs.

AST-backed checks confirmed AArch64 wrapper callees for
`find_prepared_fused_compare_operand_producer`,
`find_prepared_fused_compare_operand_producer_facts`, and
`find_prepared_materialized_condition_producer` resolve through
`src/backend/prealloc/comparison.hpp`. The focused prepared lookup helper test
callee check also resolves
`find_prepared_fused_compare_operand_producer_facts` through
`src/backend/prealloc/comparison.hpp`.

## Suggested Next

Step 4 - Validate And Record Proof should review the completed owner move and
decide whether the active plan is ready for lifecycle close/deactivation or
needs any supervisor-selected broader validation.

## Watchouts

- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`, return-chain
  helpers, edge-publication lookups, and
  `PreparedSameBlockLoadLocalStoredValueSource`; those remain separate from the
  comparison owner.
- `src/backend/mir/aarch64/codegen/comparison.cpp` still needs
  `prepared_lookups.hpp` for `PreparedFunctionLookups` member access; do not
  remove that include as part of comparison-owner cleanup unless the function
  context ownership changes.
- `src/backend/prealloc/comparison.cpp` still duplicates the small file-local
  `existing_prepared_value_name_id` helper because the existing
  `prepared_lookups.cpp` helper is anonymous-namespace state and still serves
  unrelated prepared lookup APIs.
- Do not move AArch64 compare instruction selection or condition-code emission
  into shared prealloc ownership.
- Do not change branch-condition semantics.

## Proof

Ran the delegated proof command for Step 3:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Build completed and the backend CTest subset passed 179/179
tests.

Regression guard passed with 179/179 backend tests before and after, no new
failures.

Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
