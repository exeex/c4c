Status: Active
Source Idea Path: ideas/open/125_prepared_pending_store_global_publication_producer_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: AArch64 Consumer Rewire

# Current Packet

## Just Finished

Plan Step 2 - AArch64 Consumer Rewire: rewired
`lower_pending_store_global_stack_value_publications` to pass prepared
source-producer lookups into
`plan_pending_prepared_store_global_publications`, validate the returned
`store_source.source_producer_*` identity, and call the existing stack
publication emitter using `plan.source_producer_instruction_index` instead of
rescanning prior BIR instructions by result name/type. Updated
`tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp` to
prove the pending store-global stack publication path emits through the
prepared producer fact and fails closed when that prepared authority is absent.

## Suggested Next

Supervisor should review Step 2 for acceptance and choose the next lifecycle
packet from the active plan.

## Watchouts

- The AArch64 pending store-global consumer no longer has a fallback producer
  scan; contexts without `prepared_lookups` now fail closed for this path.
- The planner still owns matching the producer fact to the store source; the
  AArch64 consumer only validates the returned identity is same-block,
  before-store, in-bounds, and instruction-backed before invoking the existing
  publication emitter.

## Proof

Passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Proof log: `test_after.log` (`100% tests passed, 0 tests failed out of 179`).
