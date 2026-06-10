Status: Active
Source Idea Path: ideas/open/163_bir_edge_join_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Edge And Join-Source Surfaces

# Current Packet

## Just Finished

Completed Step 1 inventory for
`ideas/open/163_bir_edge_join_source_annotation_schema.md`.

Prepared Route 5 oracle surfaces identified:

- Edge publication records and lookup APIs:
  `PreparedEdgePublication`, `PreparedEdgePublicationLookups`,
  `PreparedEdgePublicationKey`, `prepared_edge_publication_key(...)`,
  `make_prepared_edge_publication_lookups(...)`,
  `find_indexed_prepared_edge_publications(...)`, and
  `find_unique_indexed_prepared_edge_publication(...)`.
- Edge-copy source helper APIs:
  `PreparedEdgeCopySourceFacts`,
  `prepare_edge_copy_source_facts(...)`,
  `find_unique_indexed_block_entry_parallel_copy_edge_publication(...)`, and
  `prepare_block_entry_parallel_copy_edge_source_facts(...)`.
- Edge-copy status categories to preserve as oracles:
  available, missing prepared lookups, missing predecessor label, missing
  successor label, missing destination value, missing publication, ambiguous
  publication, publication unavailable, edge mismatch, unsupported move, move
  edge mismatch, publication/move mismatch, missing source value, missing source
  home, missing source producer, missing source memory access, and incomplete
  source memory access.
- Prepared source producer vocabulary:
  unknown, immediate, load-local, load-global, cast, binary, and select
  materialization via `PreparedEdgePublicationSourceProducerKind` plus
  `PreparedEdgePublicationSourceProducerLookups` and
  `make_prepared_edge_publication_source_producer_lookups(...)`.
- Prepared source memory access status oracle:
  unavailable, available, missing prepared memory access, and incomplete
  prepared memory access via
  `PreparedEdgePublicationSourceMemoryAccessStatus`.
- Current-block join-source helper APIs:
  `PreparedCurrentBlockJoinParallelCopySourceFact`,
  `PreparedCurrentBlockJoinParallelCopySourceFacts`,
  `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`, and
  `prepare_current_block_join_parallel_copy_source_facts(...)`.
- Current-block join-source top-level status oracle:
  available, missing names, missing value locations, missing edge publication
  lookups, missing block, and missing successor label.

Existing BIR/MIR Route 5 query surfaces identified:

- `mir::BirCfgEdgePublicationSourceRequest`,
  `mir::BirCfgEdgePublicationSourceIdentity`, and
  `mir::find_bir_cfg_edge_publication_source_identity(...)`.
- `mir::BirCfgEdgePublicationSourceStatus`: available, missing predecessor
  label, missing successor label, missing destination value, missing
  publication, missing source value, and missing source producer.
- `mir::BirCurrentBlockJoinSourceRequest`,
  `mir::BirCurrentBlockJoinSourceFact`,
  `mir::BirCurrentBlockJoinSourceIdentity`, and
  `mir::find_bir_current_block_join_source_identity(...)`.
- `mir::BirCurrentBlockJoinSourceStatus`: available, missing block, missing
  successor label, missing publication, and missing source producer.

Concrete Route 5 BIR schema targets for Step 2:

- Add target-neutral status/scope/source-kind/value-role vocabularies in BIR,
  likely `Route5EdgePublicationStatus`, `Route5PublicationSourceKind`,
  `Route5PublicationValueRole`, and a join/block status that can represent
  available, explicit no-source, memory-source available, missing predecessor,
  missing successor, missing destination, missing publication, missing source
  value, missing source producer, missing source memory access, incomplete
  source memory access, and no-match.
- Add edge annotation record(s) for predecessor/successor/destination/source
  identity:
  predecessor block label/id, successor block label/id, destination value
  identity/name/id/type, source value identity/name/id/kind/type, source
  producer kind, source producer block label/id, source producer instruction
  pointer/index, optional source memory identity, and explicit no-source marker.
- Add block/join-source annotation record(s) for current-block join incoming
  source identity:
  successor block label/id, predecessor label/id, destination PHI instruction
  pointer/index, destination value identity/name/type, incoming source value
  identity/name/kind/type, source producer kind and instruction index when
  present, incoming-expression value links, and source-value links.
- Reuse existing Route 1 value/source identity and Route 3 memory access
  identity where possible. Do not introduce a duplicate producer schema or
  target memory/address schema.
- Optional Step 3 index shape:
  `Route5EdgeJoinSourceIndex` or equivalent function-local index containing
  edge records keyed by predecessor label/id, successor label/id, destination
  value name/id/type and join-source records keyed by successor block label/id
  plus destination/source value names. The index must be rebuildable from Route
  5 BIR edge/block annotation records and must not become a prepared lookup
  container.

Oracle cases already available in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:

- Positive edge-copy source: named source on a predecessor/successor/destination
  tuple, immediate source, same-value source, stack source, and source producer
  facts.
- Positive memory-source edge: load-local source memory identity from the edge
  publication source memory access path.
- Positive join-source: current-block PHI incoming named source, immediate
  source, stack-source identity, incoming expression closure, and source value
  closure.
- Negative/fail-closed edge cases: missing prepared lookups, missing
  publication, unavailable publication, ambiguous publication, missing source
  value, missing source home, move edge mismatch, publication/move mismatch,
  invalid predecessor, invalid successor, wrong destination, and wrong
  predecessor/successor/destination tuple.
- Negative/fail-closed join cases: missing edge publication lookups, no PHIs,
  wrong successor label, missing named source producer, and unsupported
  prepared move. Step 2 should encode only the semantic BIR side and leave
  unsupported move policy as a prepared oracle boundary.

## Suggested Next

Execute Step 2 by adding Route 5 BIR annotation records and construction
helpers only, with focused oracle assertions in
`backend_prepared_lookup_helper_test.cpp`. Do not switch MIR, target/codegen, or
prealloc production consumers in Step 2.

## Watchouts

- Keep Route 5 annotations target-neutral and semantic: edge and block records
  own CFG/value/source identity, not parallel-copy execution policy.
- Do not import move bundle order, parallel-copy step order, cycle temporary
  routing, execution site/block placement, phase or carrier policy, coalescing
  or redundancy flags, destination registers, storage-sharing checks,
  `PreparedEdgePublication`, or prepared move records.
- Do not import target/prealloc storage details as schema: destination homes,
  destination storage kind, destination registers, source/destination storage
  sharing, register views, stack-source extension policy, and aggregate copy
  authority remain prepared/target policy.
- Explicit no-source and memory-source records must be representable as
  semantic identities; absence alone should not stand in for unavailable or
  no-source when an edge/join identity exists.
- Step 2 should compare BIR records against prepared and current MIR query
  oracles, but should not migrate `mir::find_bir_cfg_edge_publication_source_identity(...)`
  or `mir::find_bir_current_block_join_source_identity(...)`.

## Proof

Inventory-only Step 1 proof: no build required.

Recommended narrow Step 2 proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.
