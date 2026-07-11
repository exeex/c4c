# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Establish and consume source-semantic named results

## Just Finished

- Step 2.2 Route 2 ownership correction is complete: the BIR select-dependency
  result now owns type-less named lookup plus root producer kind,
  materialization availability, value identity, and instruction identity.
- `find_select_chain_view_result` now only adapts a complete BIR-owned result;
  it no longer scans instructions, classifies producer kinds, rediscovers
  candidate roots, or derives materialization completeness.
- `SameBlockSelectProducer` truth now requires `Available` and complete value
  identity, and the prepared AArch64 compatibility construction populates that
  carrier coherently. Focused contracts cover positive identity, type-less
  lookup, and missing/incomplete/ambiguous/mismatched/unavailable failures.

## Suggested Next

- Supervisor review of the completed Step 2.2 correction, then select the next
  plan packet if the ownership audit accepts Route 2 retirement.

## Watchouts

- Preserve Step 2.1 select-arm short-circuit and operand-order semantics; this
  packet transfers ownership and must not broaden dependency discovery.
- Keep the carrier correction bounded. Do not migrate target materializers or
  introduce target policy while repairing explicit status semantics.
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

- Passed required acceptance proof (331/331 backend tests):
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`
- Canonical proof log: `test_after.log`.
