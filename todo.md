# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Audit ownership and run integration proof — final three-target acceptance packet

## Just Finished

- Step 6 final ownership audit confirmed zero `Route1`-`Route8`, route-index,
  or route-builder vocabulary in `src/backend/mir/query.hpp` and
  `src/backend/mir/query.cpp`.
- AST-backed declaration, definition, callee, and direct-caller checks confirmed
  source-semantic adapters consume named BIR producer/select/memory results,
  while current-block, block-entry, CFG-edge, and join-source adapters consume
  prepared publications or `PreparedMirDirectEdgePublicationSourceQuery`.
- All audited adapters retain explicit unavailable/status checks and identity
  validation. No common query rebuilds a route index, reconstructs prepared
  lookups, or falls back from missing prepared/named authority.
- Direct target paths remain ownership-correct: x86
  `append_prepared_compare_join_parallel_copy` consumes the prepared MIR edge
  intent; AArch64 publication/select callers consume named common adapters;
  RV64 branch stack loads consume prepared MIR freshness authority. Remaining
  target-local Route vocabulary is the already-deferred ideas 708-710
  materializer scope, not a common-query fallback.

## Suggested Next

- Ask the plan owner to close idea 706: Step 6 ownership and integration
  acceptance is complete against the accepted 24-failure baseline. Preserve
  target materializer cleanup for ideas 708-710 and the positive
  stack-destination authority gate for idea 707.

## Watchouts

- Do not treat target-local Route 2/3/6/7 compatibility and materializer code as
  idea 706 drift; the active plan explicitly assigns that cleanup to ideas
  708-710.
- The accepted baseline contains 24 runtime failures, including
  `backend_prepared_lookup_helper`, prepared layout/printer contracts, one x86
  handoff abort, RV64 dumps, and prepared-BIR CLI cases. The after run has the
  identical 24-name set: no added or removed failure.

## Proof

- Exact delegated command: `{ cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log
  2>&1`. The default build succeeds; 373/397 backend tests pass and 24 fail.
- Normalized comparison of the `The following tests FAILED` sections in
  `test_before.log` and `test_after.log`: 24 before, 24 after, empty symmetric
  difference. Canonical after log: `test_after.log`.
- Direct contracts are baseline-identical and green in both logs:
  `backend_common_mir_query_route_authority_guard`,
  `backend_x86_prepared_decoded_home_storage`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_riscv_prepared_edge_publication`.
