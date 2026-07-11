# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate placement and executable-authority queries

## Just Finished

- Step 3's first bounded placement family now makes
  `PreparedMirFunctionView::current_block_direct_edge_publication_sources`
  consume only prepared names, value homes, edge-publication lookups, control
  flow, and move/freshness authority.
- Removed the common-MIR caller's Route 5/BIR join-source discovery and evidence
  agreement fallback. Missing prepared join-transfer authority fails closed
  even when the raw BIR route remains discoverable.

## Suggested Next

- Migrate the public `find_bir_cfg_edge_publication_source_identity` family once
  a prepared view/result carries its required producer kind/index, value type,
  and memory-access identity fields with unique edge/destination binding.

## Watchouts

- `PreparedMirDirectEdgePublicationSourceView` currently carries placement and
  freshness authority but not the producer/type/memory fields required to
  migrate `find_bir_cfg_edge_publication_source_identity`; do not invent a
  partial request-pointer adapter or discard those contracts.
- Remaining Route 3 helpers in common MIR serve the separate Route 5
  publication/edge identity family; do not mechanically migrate them or lower
  the guard inventory until their owning family is migrated.
- Preserve missing, incomplete, ambiguous, unsupported, and mismatched
  fail-closed behavior at the common-MIR boundary.
- Eighteen Route 1 spellings remain in other bounded memory/publication/
  edge-join adapters; they are not same-block producer authority and should
  migrate with their owning families rather than being mechanically renamed.
- Route 5 remains the only public-header breach and the largest family; leave
  its indexed edge/join migration for a later bounded packet.
- Keep target materializer migration in ideas 708-710 and stack-destination
  authority work in idea 707.

## Proof

- Passed the supervisor-selected exact acceptance proof (331/331 backend tests):
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`
- Canonical proof log: `test_after.log`.
