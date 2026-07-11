# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Establish and consume source-semantic named results

## Just Finished

- Step 2.2 migrated the bounded same-block select-producer common-MIR adapter
  to consume the existing ownership-correct `BirProducerResult` contract.
- `SameBlockSelectProducer` now preserves explicit `BirViewStatus`, produced
  value identity, block identity, and instruction index. Missing, malformed,
  wrong-kind, wrong-type, future, ambiguous, and internally mismatched evidence
  fails closed without route fallback or recursive select interpretation.
- The focused shared-producer contract covers the positive select identity and
  bounded negative statuses while preserving existing target-carrier
  compatibility.

## Suggested Next

- Inventory the next bounded Route 1 common-MIR adapter family and select one
  semantic owner for migration; keep publication and indexed edge/join helpers
  out of a mechanical rename packet.

## Watchouts

- `SameBlockSelectProducer::operator bool` remains pointer-based because target
  prepared adapters also use this shared carrier and were explicitly outside
  this packet; direct common-MIR query results nevertheless always carry and
  test explicit status/value/block/index identity.
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
  focused shared select-producer status/identity contract and route-authority
  guard; `test_after.log` is the canonical proof log.
