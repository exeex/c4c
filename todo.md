# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Adapt common MIR to named source-semantic results

## Just Finished

- Step 2.1 established `BirSelectDependencyResult` and the BIR-owned
  `find_bir_select_dependency` API. BIR now owns recursive Select/Cast/Binary/
  LoadGlobal interpretation with explicit complete-direct-global,
  complete-no-dependency, unavailable, incomplete, ambiguous, and mismatched
  statuses plus stable block, root-value, dependency-value, and instruction
  indices.
- Added focused BIR contract proof for direct-global and no-dependency success,
  all required negative statuses, stable identities, and before-index behavior.

## Suggested Next

- Step 2.2: replace the rejected common-MIR recursive select-chain walk with a
  narrow adapter over `find_bir_select_dependency`, preserving the named
  result's status and stable identities before reconsidering the Route 2 guard.

## Watchouts

- Remove `find_select_chain_dependency` from common MIR in Step 2.2; common MIR
  must not duplicate or reinterpret the BIR-owned recursion now established.
- Do not accept the uncommitted Route 2 guard reduction from 29 to zero; restore
  the prior guard inventory until ownership-correct consumption is proved.
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
- The supervisor-selected backend proof passed; `test_after.log` is the
  canonical proof log.
