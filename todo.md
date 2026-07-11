# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the blocked-family baseline and authority inventory

## Just Finished

- Step 1 established the blocked-family baseline and authority inventory without
  implementation or expectation changes. Current lifecycle HEAD is `eeb6d01df`;
  accepted implementation boundary `d253152e0` remains the starting contract,
  and `backend_aarch64_current_block_join_routing_test.cpp` is unchanged between
  those revisions.
- The rejected facts map to exactly one focused probe each:
  - transfer result and edge destination ->
    `backend_prealloc_join_transfer_destination_consistency_test.cpp`
  - publication source, routed operands, and immediate destination ->
    `backend_prealloc_current_block_routed_operand_authority_test.cpp`
  - predecessor and parallel-edge facts ->
    `backend_prealloc_current_block_all_edge_invariance_test.cpp`
- AST-backed inventory confirms the prepared authority boundary is
  `PreparedCurrentBlockJoinRoutingFact` stored by
  `PreparedFunctionLookups::current_block_join_routing_facts`, with
  `query_prepared_current_block_join_routing_consumption` as the public
  owner-attached query. The fact carries predecessor/successor, destination,
  source, routed-value, role, and publication-origin fields; the consumption
  query currently validates applicable facts and reports edge count.
- All three proposed focused `.cpp` files are absent, and none has an
  executable, `add_test`, or CTest registration in
  `tests/backend/bir/CMakeLists.txt`.

## Suggested Next

- Execute the first bounded Step 2 packet: add and register only
  `backend_prealloc_join_transfer_destination_consistency_test.cpp`, proving
  matching and mismatching transfer-result/edge-destination/publication-
  destination triples without routed-operand or AArch64 changes.

## Watchouts

- Accepted HEAD `d253152e0` contains Step 6.1 owner storage; do not reopen it.
- Ideas 716, 713, and 705 remain open and blocked.
- Keep the AArch64 integration test as integration proof, not discovery.
- Keep each rejected fact in its assigned probe; in particular, do not let
  Step 2 define publication-source or routed-operand authority.
- The existing query groups applicability by routed identity and role, then
  checks destination/source/predecessor/origin facts. Focused probes must test
  semantic contracts rather than mirror that implementation shape.

## Proof

- Read-only Step 1 proof: canonical `test_before.log` records the accepted
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` baseline as
  `100% tests passed, 0 tests failed out of 317` (317/317).
- Confirmed with `git diff d253152e0..HEAD --
  tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp` (empty),
  AST-backed symbol/type/caller/callee queries, and registration/file searches.
- Per the delegated docs-only proof contract, no tests were rerun and
  `test_after.log` was not written.
