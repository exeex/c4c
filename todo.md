# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate source-semantic common queries

## Just Finished

- Step 2 migrated the bounded same-block binary/select producer, named producer
  identity, scalar producer, and integer-constant common-query family to
  `BirProducerView`. Before-index and stable producer/value identities are
  preserved; missing names, type/label mismatches, future producers,
  incomplete results, and ambiguous producer results fail closed.
- Integer folding now walks operands through the named producer view with the
  original depth bound and arithmetic semantics, without rebuilding or
  querying a Route 1 index in common MIR. The exact Route 1 guard inventory was
  ratcheted from 33 to 23 hits.

## Suggested Next

- Step 2: migrate the bounded Route 2 select-chain common-query family to its
  named BIR select/dependency view, preserving root/dependency identities and
  explicit unavailable/incomplete/ambiguous behavior. Ratchet the Route 2
  guard inventory only for the completed family.

## Watchouts

- Twenty-three Route 1 spellings remain in other bounded memory/publication/
  edge-join adapters; they are not same-block producer authority and should
  migrate with their owning families rather than being mechanically renamed.
- Route 5 remains the only public-header breach and the largest family; leave
  its indexed edge/join migration for a later bounded packet.
- Keep target materializer migration in ideas 708-710 and stack-destination
  authority work in idea 707.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`: 330/330 backend
  tests passed, including the focused producer-query and route-authority guard
  contracts. Proof log: `test_after.log`.
