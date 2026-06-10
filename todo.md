# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add and prove current-block join-source identity

## Just Finished

Step 4 added BIR current-block join-source semantic identity support in
`src/backend/mir/query.hpp` and `src/backend/mir/query.cpp`, then proved it in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` against
`PreparedCurrentBlockJoinParallelCopySourceFact(s)`.

Completed work item:

- Added `find_bir_current_block_join_source_identity` over successor-block PHIs,
  returning predecessor/successor labels, destination/source values,
  same-block named source producer identity, incoming-expression identities, and
  source-value identities.
- Kept BIR identity free of prepared routing conveniences: no destination
  register names, homes, stack-source policy flags, immediate-source move
  details, storage-sharing fields, move bundles, or prepared move records.
- Extended the focused current-block join test with PHI-backed named,
  immediate, and stack-source paths, oracle comparison against prepared
  semantic facts, and negative missing-PHI, successor mismatch, and missing
  named source producer paths.
- Did not switch MIR/AArch64 consumers or edit prealloc/codegen routing.

## Suggested Next

Proposed Step 5 packet: switch the first narrow MIR/AArch64 consumer from the
prepared current-block join-source helper to the new BIR semantic identity only
where the consumer needs source/destination expression identity. Keep prepared
move routing, storage policy, homes, and register/stack decisions on the
existing prepared path until a dedicated routing replacement exists.

## Watchouts

- Step 4 adds a BIR semantic query but leaves all consumers on the existing
  prepared helper.
- `BirCurrentBlockJoinSourceIdentity` intentionally reports PHI source identity
  and same-block source producers only. It does not replace prepared move
  resolution or target storage/routing policy.
- Step 5 should avoid treating the new BIR query as an authority for register
  names, source/destination homes, stack-source routing, move bundles, or
  immediate move materialization details.

## Proof

Ran the delegated proof:

`cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the passing
`backend_prepared_lookup_helper` CTest run.
