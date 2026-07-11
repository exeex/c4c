# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Establish and consume source-semantic named results

## Just Finished

- Step 2.2 migrated the bounded same-block binary-producer common-MIR family
  to consume the existing ownership-correct `BirProducerResult` contract.
- `SameBlockBinaryProducer` now preserves explicit `BirViewStatus`, produced
  value identity, block identity, and instruction index. Missing, malformed,
  wrong-kind, wrong-type, ambiguous, and internally mismatched evidence fails
  closed without reconstructing producer semantics in common MIR.
- The focused shared-producer contract covers the positive identity and all
  bounded negative statuses. The route-authority inventory remains exact: this
  family already used only the named producer view and contained no route
  vocabulary to retire.

## Suggested Next

- Migrate the adjacent same-block select-producer adapter to preserve the
  existing named producer result's explicit status and identity, leaving
  select dependency traversal, publication, and prepared-placement families
  unchanged.

## Watchouts

- The next select-producer packet is only the direct producer adapter; do not
  widen it into select-chain dependency traversal or target materialization.
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
  focused shared binary-producer status/identity contract and route-authority
  guard; `test_after.log` is the canonical proof log.
