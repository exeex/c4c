# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove prepared-oracle equivalence for edge sources

## Just Finished

Step 3 added focused prepared-oracle equivalence coverage for the
target-neutral BIR CFG edge publication/source identity query in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Completed test work item:

- Added a reusable prepared-vs-BIR helper over
  `find_bir_cfg_edge_publication_source_identity` keyed by predecessor,
  successor, and destination identity.
- Extended the producer-facts fixture so prepared binary/select edge
  publications have matching BIR PHIs, keeping the oracle comparison semantic.
- Covered available load-local, cast, binary, and select edge sources against
  prepared edge-copy source facts.
- Verified optional load-local source memory identity through
  `BirMemoryAccessIdentity` fields while intentionally excluding prepared target
  addressing policy such as base-plus-offset usability.
- Covered missing destination publication, unavailable named source producer,
  and negative predecessor/successor/destination key paths without switching
  consumers or editing implementation code.

## Suggested Next

Proposed Step 4 packet: add the remaining join-source coverage over current
block/join publication behavior before any consumer switch. Keep the proof
oracle-based and focused on source identity, not move-routing or target
addressing policy.

## Watchouts

- Step 3 stayed test-only and did not switch MIR/AArch64 consumers.
- The prepared oracle can classify a named edge source as non-materializable
  when no source producer exists; the BIR surface reports that same semantic
  condition as `MissingSourceProducer`.
- `BirCfgEdgePublicationSourceIdentity` remains a BIR semantic source-identity
  surface. Homes, move records, destination register names, coalescing facts,
  execution-site/phase/carrier facts, and parallel-copy scheduling remain out of
  scope.

## Proof

Ran the delegated proof:

`cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the passing
`backend_prepared_lookup_helper` CTest run.
