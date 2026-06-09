Status: Active
Source Idea Path: ideas/open/147_comparison_prealloc_fact_owner.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Establish The Comparison Owner

# Current Packet

## Just Finished

Step 2 - Establish The Comparison Owner completed.

Created `src/backend/prealloc/comparison.hpp` and
`src/backend/prealloc/comparison.cpp` as the shared prealloc comparison owner.
Moved these facts there:

- `PreparedFusedCompareOperandProducer`
- `PreparedFusedCompareOperandProducerFacts`
- `PreparedMaterializedConditionProducer`

Moved these shared helper declarations and definitions there:

- `find_prepared_fused_compare_operand_producer`
- `find_prepared_fused_compare_operand_producer_facts`
- `find_prepared_materialized_condition_producer`

`src/backend/prealloc/prepared_lookups.hpp/.cpp` no longer declare or define
the moved facts/helpers. `prepared_lookups.hpp` includes the new owner as a
temporary compatibility include for existing prepared-lookup consumers.

`src/backend/mir/aarch64/codegen/comparison.cpp` now includes
`../../../prealloc/comparison.hpp`; target-local compare instruction selection,
condition-code emission, and branch lowering behavior were left in place.

AST-backed checks confirmed the moved helper declarations in
`comparison.hpp`, the moved helper definitions in `comparison.cpp`, the
same-block helper callees in `select_chain_lookups.hpp`, and the AArch64
wrapper/consumer caller path.

## Suggested Next

Step 3 - Move Lookup Helpers And Consumers should audit remaining include sites
that get comparison facts/helpers through `prepared_lookups.hpp`, update direct
consumers to include `prealloc/comparison.hpp` where appropriate, and decide
whether the temporary compatibility include in `prepared_lookups.hpp` can be
removed.

## Watchouts

- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`,
  return-chain helpers, edge-publication lookups, and
  `PreparedSameBlockLoadLocalStoredValueSource`; do not move those with the
  comparison owner.
- `prepared_lookups.hpp` currently includes `comparison.hpp` only to keep
  existing prepared-lookup consumers compiling after the ownership move.
  Step 3 should avoid turning that into the long-term comparison access path if
  direct includes are more appropriate.
- `src/backend/prealloc/comparison.cpp` duplicates the small file-local
  `existing_prepared_value_name_id` helper because the existing
  `prepared_lookups.cpp` helper is anonymous-namespace state and still serves
  unrelated prepared lookup APIs.
- Do not move AArch64 compare instruction selection or condition-code emission
  into shared prealloc ownership.
- Do not change branch-condition semantics.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. The backend CTest subset passed 179/179 tests.

Regression guard passed with 179/179 backend tests before and after, no new
failures.

Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
