# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Continue remaining prepared-authority families — Route 4 current-block publication packet only

## Just Finished

- Step 3.3 Route 4 current-block publication packet replaced common MIR's
  block-copy/index/discovery reconstruction with direct consumption of the
  complete uniquely-bound prepared publication object.
- Removed the public reconstruction request and Route 4 conversion helpers;
  the adapter now rejects unavailable, incomplete, stale, mismatched-kind, and
  instruction/value-incoherent prepared payloads.
- Updated focused positive and fail-closed contracts and ratcheted the common
  query Route 4 inventory from 14 hits to zero.

## Suggested Next

- Select the next separately bounded Step 3.3 prepared-authority family.

## Watchouts

- The authorized Route 1, Route 4, and Route 5 inventory ratchets are now 5, 0,
  and 33 hits. The guard also requires zero route analysis entry points in the
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
