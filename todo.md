# Current Packet

Status: Active
Source Idea Path: ideas/open/704_bir_semantic_handoff_views.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate the first common-MIR semantic entry points

## Just Finished

- Completed Step 5's bounded first migration by moving
  `find_same_block_producer_identity` and `find_same_block_scalar_producer`
  from Route1 queries to `BirProducerView`.
- Added common-MIR proof that both migrated entry points fail closed for an
  ambiguous producer result, while retaining existing unavailable, future,
  type-mismatch, and missing-name coverage.

## Suggested Next

- Execute Step 6's boundary audit and integration proof.

## Watchouts

- Route-backed helpers remain in `query.cpp` for deferred memory,
  publication, select-chain, and prepared-consumer migration; this packet did
  not expand or redirect those consumers.
- Requests without an explicit value type discover candidate source value
  types locally, but producer identity and ambiguity are still decided by the
  named producer view and any negative result fails closed.

## Proof

- Passed the supervisor-selected proof (308/308 backend tests):
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`.
- Focused common-MIR producer behavior passed as part of the suite:
  `backend_x86_shared_producer_query`.
- Canonical proof log: `test_after.log`.
