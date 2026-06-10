Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate The Selected Join-Source Consumer

# Current Packet

## Just Finished

Step 4 (`Migrate The Selected Join-Source Consumer`): migrated the selected
AArch64 consumer,
`aarch64_codegen::build_current_block_join_prepared_query_routing(...)`, so it
queries the Route 5-backed MIR current-block join-source identity first and
uses prepared current-block join facts only as the absent/no-PHI fallback.
Added the isolated
`backend_aarch64_current_block_join_routing` harness proving Route 5-available
routing without prepared value-location policy and no-PHI prepared fallback
routing without weakening or reordering `backend_aarch64_instruction_dispatch`.

## Suggested Next

Start Step 5 with a narrow scan of direct prepared current-block join-source
helper exposure and contract only the surface proven private by Steps 3-4.

## Watchouts

- Keep Route 5 limited to edge publication identity and current-block
  join-source semantic identity.
- Do not import move bundle order, parallel-copy scheduling, cycle temporary
  routing, execution site policy, carrier/phase policy, coalescing,
  redundancy, destination registers, storage-sharing checks, prepared move
  records, or AArch64 edge-copy emission policy into BIR.
- Do not hide prepared current-block join-source helpers until direct
  consumers have been re-scanned and migrated or proven out of scope.
- The migrated MIR helper currently uses direct Route 5 current-block join
  records because the existing MIR result is an aggregate list with no
  destination/source request fields. If Step 4 needs typed per-value lookup,
  prefer `Route5EdgeJoinSourceIndex` and
  `bir::route5_find_current_block_join_source(...)` in the consumer/facade
  rather than adding scheduling policy to BIR.
- The selected MIR helper has no predecessor field in its request shape.
  Predecessor absence is covered by Route 5 edge publication oracle/status
  coverage, while current-block join migration should preserve predecessor
  labels carried by PHI incoming records.
- Do not include `backend_aarch64_instruction_dispatch` in the next narrow proof
  unless the supervisor explicitly selects it. The known ambient dispatch
  failure is close-level ambient debt, not the Step 4 proof route, unless its
  failure mode changes to implicate Route 5.
- The isolated Step 4 harness must be a real selected-consumer proof, not a
  duplicate of the Step 2/3 MIR helper oracle. It should reach
  `aarch64_codegen::build_current_block_join_prepared_query_routing(...)` or an
  equivalent narrow AArch64 consumer entry point.
- Step 4 intentionally leaves target/prealloc scheduling policy in prepared
  fallback paths; the Route 5-first path only reads semantic current-block
  join-source identity.

## Proof

```bash
(cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing)$' --output-on-failure) > test_after.log 2>&1
```

Result: passed, 2/2 tests green.

Log path: `test_after.log`.
