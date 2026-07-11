# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Establish and consume source-semantic named results

## Just Finished

- Step 2.2 migrated the bounded same-block store-local source family to the
  `BirSameBlockStoreLocalSourceResult` contract in `bir/query.*` and made the
  common-MIR adapter preserve its explicit `BirViewStatus`.
- The named result preserves the exact load/store instructions, memory/value
  identities, and stable local-slot identity. Missing, incomplete, ambiguous,
  type-mismatched, overlapping-range, and identity-mismatched evidence fail
  closed without route/index/raw-record reconstruction in the adapter.
- Focused BIR/common contracts cover success and negative status propagation.
  Retiring the now-unused legacy conversion helper ratcheted Route 1 from 18
  to 17 and Route 3 from 22 to 19 for only this family.

## Suggested Next

- Execute the next bounded Step 2 family selected by the supervisor, leaving
  publication and prepared-placement families with their owning packets.

## Watchouts

- Preserve the accepted distinction between `CompleteStopped` and
  `CompleteNoDependency`; common MIR relies only on the BIR result's complete
  contract and must not reconstruct operand traversal.
- Route 2 is now zero. Do not reintroduce route vocabulary or hidden recursive
  select dependency interpretation in later common-query packets.
- Remaining Route 3 helpers in common MIR serve publication identity; do not
  mechanically migrate them.
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

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`
- The supervisor-selected exact backend proof passed 331/331, including the
  focused named BIR/common store-local source contract and route-authority
  guard; `test_after.log` is the canonical proof log.
