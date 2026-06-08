Status: Active
Source Idea Path: ideas/open/125_prepared_pending_store_global_publication_producer_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Shared Prepared Producer Contract

# Current Packet

## Just Finished

Plan Step 1 - Shared Prepared Producer Contract: added an optional shared
source-producer lookup input to
`plan_pending_prepared_store_global_publications`, populated pending
store-global candidate `store_source.source_producer_*` facts from unique
same-block producers, and made the producer-authority path fail closed for
missing or ambiguous producers. Updated
`tests/backend/mir/backend_store_source_publication_plan_test.cpp` to cover
producer kind/index publication plus missing/ambiguous producer rejection.

## Suggested Next

Wire the AArch64 pending store-global planning call site to pass prepared
source-producer lookups, then consume the shared producer identity instead of
rediscovering producer instructions locally.

## Watchouts

- The new fail-closed behavior is active when a non-null
  `PreparedEdgePublicationSourceProducerLookups` is passed. Existing callers
  without that authority keep current behavior until their packet wires the
  lookup in.
- Ambiguous producer entries are represented by an `Unknown` producer in the
  shared lookup map and are rejected by pending store-global planning.
- A candidate producer must be in the same block, before the store, within the
  block bounds, and match the stored value's named result/type.

## Proof

Passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Proof log: `test_after.log` (`100% tests passed, 0 tests failed out of 179`).
