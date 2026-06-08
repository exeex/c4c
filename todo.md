Status: Active
Source Idea Path: ideas/open/125_prepared_pending_store_global_publication_producer_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Shared Prepared Producer Contract

# Current Packet

## Just Finished

Plan Step 1 - Shared Prepared Producer Contract: tightened
`plan_pending_prepared_store_global_publications` so pending store-global
candidates fail closed when no `PreparedEdgePublicationSourceProducerLookups`
authority is supplied. Removed the public default `nullptr` argument, made the
implementation return no candidates for absent authority before scanning, and
kept all pending candidates tied to a matching prepared producer fact. Updated
`tests/backend/mir/backend_store_source_publication_plan_test.cpp` to cover the
null/no-authority path while preserving producer-present, missing-producer,
ambiguous-producer, replay-suppression, and normal store-global planning
coverage.

## Suggested Next

Supervisor should review/commit this coherent Step 1 follow-up or route plan
owner/reviewer closure if the source idea is now satisfied.

## Watchouts

- Changed files: `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/publication_plans.cpp`, and
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp`.
- `plan_pending_prepared_store_global_publications` still accepts `nullptr`
  defensively at runtime, but callers must now pass the lookup argument
  explicitly and null authority produces no candidates.
- Untracked `review/idea125_code_review.md` was present before this packet and
  was not touched.

## Proof

Passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Proof log: `test_after.log` (`100% tests passed, 0 tests failed out of 179`).
