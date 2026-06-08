Status: Active
Source Idea Path: ideas/open/125_prepared_pending_store_global_publication_producer_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Shared Prepared Producer Contract

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

Narrow follow-up before closure: tighten
`plan_pending_prepared_store_global_publications` so shared planning fails
closed when the caller lacks source-producer lookup authority. Prefer removing
the public/default `source_producers == nullptr` producer-less mode if no real
caller needs it; otherwise make the absent-authority path explicitly produce no
pending store-global publication candidates. Keep this scoped to Plan Step 1's
shared prepared producer contract and add focused shared-prepared proof for the
null/no-authority case.

## Watchouts

- Code review `review/idea125_code_review.md` judged the route on track and
  matching source idea, with one medium watch item: shared planner behavior
  still permits producer-less candidates when `source_producers == nullptr`.
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
