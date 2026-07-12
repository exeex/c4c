# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair typed join-source identity propagation

## Just Finished

- Step 2 localized the later focused failure at
  `backend_prepared_lookup_helper_test.cpp:6421`. The earliest incorrect fact is
  at the test/query boundary: `prepared_and_bir_cfg_edge_publication_source_identity_match`
  constructs and accepts a `BirCfgEdgePublicationSourceRequest`, but never uses
  it. Instead it calls `find_bir_cfg_edge_publication_source_identity(names,
  prepared)` after deriving `prepared` from the prepared edge-publication
  lookup, so the purported BIR CFG side is another transcription of the same
  prepared oracle rather than independently resolved BIR CFG identity.
- The owning boundary is the legacy `BirCfgEdgePublicationSourceRequest` /
  `find_bir_cfg_edge_publication_source_identity` API in `src/backend/mir/query.*`
  plus its test agreement helper. The request type is explicitly retained only
  for source compatibility and the public query does not consume it.
- Classification: **out of idea 717 scope / separate initiative**. Idea 717 owns
  `PreparedMirDirectEdgePublicationSourceQuery` and
  `find_bir_current_block_join_source_identity`; this failure is in the older
  CFG edge-publication identity adapter and predates the Step 2 typed
  join-source propagation repair.

## Suggested Next

- Route the CFG identity boundary into a separate idea/plan packet. Its semantic
  repair rule should require the BIR side to resolve and validate the supplied
  predecessor block, successor block, destination value, and producer/memory
  identity independently, then compare that typed result with prepared facts;
  it must not claim BIR/prepared agreement by adapting prepared facts twice.

## Watchouts

- Do not repair this by weakening expectations or by continuing to ignore the
  request: that would preserve a circular oracle. Also do not fold this legacy
  CFG adapter work into `find_bir_current_block_join_source_identity`; the two
  APIs carry different authority shapes and lifecycle ownership.
- The first failing row is load-local only because it is the first call site.
  Nearby cast, binary, select, missing-destination, and unavailable-source rows
  use the same non-independent adapter boundary, so a semantic repair must cover
  the family rather than special-case load-local.

## Proof

- No build or test was run, per the delegated read-only localization packet.
  The existing focused failure baseline and `test_after.log` were preserved.
- Used AST-backed definition/callee tracing for the test helper and
  `find_bir_cfg_edge_publication_source_identity`, plus focused source/history
  inspection to identify the owning API boundary.
