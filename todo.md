# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Adapt common MIR to the named memory-access result

## Just Finished

- Step 2.4 now adapts `find_bir_memory_access_identity` through the complete
  named `BirMemoryAccessResult` payload and one status-preserving copier.
- The adapter copies instruction, block, value, address-space, volatility,
  alignment, slot/link, and distinct local/global/string identities without
  BIR instruction look-through, base-name inference, or Route 3 discovery.
- Request/result kind, block, instruction-index, and instruction-pointer
  disagreement fails closed. The source authority guard prevents route or raw
  instruction reconstruction from returning to this bounded function.

## Suggested Next

- Supervisor review of the completed Step 2.4 adapter, then select the first
  bounded Step 3 placement-query migration packet if accepted.

## Watchouts

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
