# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Continue remaining prepared-authority families — Route 5 dead conversion-helper cleanup packet only

## Just Finished

- Step 3.3 Route 5 dead conversion-helper cleanup removed the unreachable
  edge-publication record/status/source reconstruction family now that the
  live adapter directly consumes `PreparedEdgeCopySourceFacts`.
- Ratcheted exact common-query inventories for Route 5 from 33 to zero and for
  incidental Route 1/Route 3 spellings removed with that family from 5/19 to
  2/18, without weakening the guard's exact-count or self-check semantics.

## Suggested Next

- Select the next separately bounded Step 3.3 prepared-authority family after
  the Route 5 dead-helper cleanup.

## Watchouts

- The authorized Route 1, Route 3, Route 4, and Route 5 inventory ratchets are
  now 2, 18, 0, and 0 hits. The guard also requires zero route analysis entry points in the
  common implementation while retaining its remaining-route self-checks.
- `backend_prepared_lookup_helper_test` still does not compile because of
  unrelated pre-existing missing Route 1 locals in
  `verify_prepared_same_block_scalar_source_facts()` and a `string_view::clear()`
  call in the store-source metadata fixture. Its join-source migration produces
  no remaining compiler diagnostics.

## Proof

- Exact delegated proof built both targets. The guard passes; the frame/call
  contract retains the known baseline `call_plans no longer publish the direct
  call` failure. Command: `cmake --build --preset default --target
  backend_common_mir_query_route_authority_guard_test
  backend_prepare_frame_stack_call_contract_test && ctest --test-dir build
  --output-on-failure -R
  '^(backend_common_mir_query_route_authority_guard|backend_prepare_frame_stack_call_contract)$'
  > test_after.log 2>&1`; canonical log: `test_after.log`.
