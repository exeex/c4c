# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Establish and consume source-semantic named results

## Just Finished

- Step 2.2 migrated the bounded same-block global-load source query from Route 3
  index/raw-record reconstruction to `BirSameBlockGlobalLoadResult`.
- The BIR-owned result preserves stable instruction, load, result-value, and
  global-symbol identity and reports unavailable, incomplete, and ambiguous
  states explicitly; common MIR fails closed on every non-available result.
- Focused contracts cover the available identity and a type-mismatch negative;
  the common-query guard ratcheted Route 3 from 59 to 47.

## Suggested Next

- Execute the next bounded Step 2 family selected by the supervisor, leaving
  the adjacent Route 3 load-local/store-local families with their owning packet.

## Watchouts

- Preserve the accepted distinction between `CompleteStopped` and
  `CompleteNoDependency`; common MIR relies only on the BIR result's complete
  contract and must not reconstruct operand traversal.
- Route 2 is now zero. Do not reintroduce route vocabulary or hidden recursive
  select dependency interpretation in later common-query packets.
- Remaining Route 3 helpers in common MIR still serve adjacent load-local,
  store-local, and publication families; do not mechanically migrate them.
- Preserve missing, incomplete, ambiguous, unsupported, and mismatched
  fail-closed behavior at the common-MIR boundary.
- Twenty-three Route 1 spellings remain in other bounded memory/publication/
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
  common-query route guard, focused memory contracts, and unchanged target
  backend coverage; `test_after.log` is the canonical proof log.
