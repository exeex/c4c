# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Continue remaining prepared-authority families — Route 5 edge/join packet only

## Just Finished

- Step 3.3 Route 5 edge/join packet replaced current-block join request/index
  discovery in the common MIR query with the complete prepared direct-edge
  source view, removed the common request and dead join reconstruction helpers,
  and preserved fail-closed unavailable and mixed-successor handling.
- Adapted both bounded AArch64 fixtures and the prepared MIR core comparator
  contract to prove prepared authority without Route 5 fallback.
- Migrated the stale aggregate join-source matrix to construct a complete
  `PreparedMirCoreView` from its existing semantic components; positive source
  rows and unsupported/missing authority rows now query typed prepared views.

## Suggested Next

- Select the next separately bounded Step 3.3 prepared-authority family; keep
  placement, publication, home, move, freshness, frame, call-plan, and control
  lookup work outside this completed edge/join packet.

## Watchouts

- The authorized Route 1, Route 4, and Route 5 inventory ratchets are applied
  at 5, 14, and 33 hits. The public-header guard now requires zero route-owned
  payload types and the implementation self-checks remain unchanged.
- `backend_prepared_lookup_helper_test` still does not compile because of
  unrelated pre-existing missing Route 1 locals in
  `verify_prepared_same_block_scalar_source_facts()` and a `string_view::clear()`
  call in the store-source metadata fixture. Its join-source migration produces
  no remaining compiler diagnostics.

## Proof

- Exact expanded proof is green 4/4: `cmake --build --preset default --target
  backend_common_mir_query_route_authority_guard_test
  backend_aarch64_current_block_join_routing_test
  backend_prepared_mir_core_comparator_test
  backend_aarch64_instruction_dispatch_test && ctest --test-dir build
  --output-on-failure -R
  '^(backend_common_mir_query_route_authority_guard|backend_aarch64_current_block_join_routing|backend_prepared_mir_core_comparator|backend_aarch64_instruction_dispatch)$'
  > test_after.log 2>&1`; `test_after.log` is canonical and sufficient for this
  bounded packet.
