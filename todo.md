# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate placement and executable-authority queries

## Just Finished

- Step 3's public `find_bir_cfg_edge_publication_source_identity` family now
  consumes a complete `PreparedEdgeCopySourceFacts` result plus prepared names.
- Removed its Route 5 function/index reconstruction path while preserving
  producer kind/index, value type, memory identity, and unique edge/destination
  binding; incomplete, ambiguous, missing, and mismatched prepared evidence
  fails closed.
- Returned value identities retain prepared name/type/immediate semantics
  without borrowing pointers from temporary prepared-facts payloads.

## Suggested Next

- Continue Step 3 with the next bounded placement/executable-authority family
  selected by the supervisor.

## Watchouts

- The legacy request record remains for diagnostic/test builders, but the
  public identity query no longer accepts it; do not restore a request-pointer
  overload.
- Prepared producer identity intentionally uses semantic kind/index/value
  identity rather than reconstructing raw enclosing `bir::Inst*` pointers.
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
