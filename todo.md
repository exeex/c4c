# Current Packet

Status: Active
Source Idea Path: ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement independent typed CFG identity resolution

## Just Finished

- Completed Plan Step 2 independent typed CFG identity resolution.
- Replaced the prepared-fact-backed public overload with a
  `BirCfgEdgePublicationSourceRequest` consumer. It validates block ids/text and
  destination pointer/name/type constraints, resolves the exact leading phi and
  predecessor incoming through Route 5, and publishes BIR-owned destination,
  incoming, producer-instruction, and memory-access pointers and identities.
- Extended typed MIR failure status for mismatched requests, ambiguous
  publications, and missing/incomplete source memory; unavailable Route 5
  records remain unavailable rather than being collapsed to success.
- Generalized Route 5 memory-source attachment to both load-local and
  load-global through the exact Route 3 producer instruction, and reject
  duplicate matching destination phis or predecessor incomings without using
  row order.
- Updated the focused helper so prepared facts and independently established
  BIR identities are compared instead of adapting prepared facts twice.
- Corrected the Step 2 review blockers: Route 5 now performs a full leading-phi
  uniqueness pass before selecting a destination, valid incoming label IDs
  exclusively govern identity when both sides provide them, and the MIR query
  distinguishes duplicate-publication ambiguity from destination key/type
  mismatch.
- Added direct public-query negatives proving duplicate exact destination phi
  rejection, conflicting incoming-ID rejection despite matching text, and
  mismatch-versus-ambiguity status separation.
- Added the explicit Route 5 `AmbiguousPublication` status and use it for both
  duplicate exact destination phis and duplicate matching predecessor
  incomings. The MIR public query preserves that status, and a direct negative
  now proves duplicate incoming ambiguity separately from key/type mismatch.

## Suggested Next

- Investigate the first remaining focused failure:
  `store-source producer metadata should publish for complete prepared agreement`.
  It occurs after all CFG edge-publication source identity assertions now pass
  and is outside the Step 2 query/Route 5 edge slice.

## Watchouts

- The focused test remains red only at the later store-source prepared-agreement
  assertion. Do not widen this packet into prepared store-source publication.
- Route 5 source/destination pointer fields refer to the authoritative values
  owned inside the phi instruction, not the request's compatibility value.

## Proof

- Ran the exact delegated command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' 2>&1 | tee test_after.log`.
- Build passed. The focused test advanced past the Step 2 edge identity checks
  (including duplicate destination, duplicate incoming, conflicting-ID, and
  typed mismatch negatives) and failed at `store-source producer metadata
  should publish for complete prepared agreement`. Proof log: `test_after.log`.
