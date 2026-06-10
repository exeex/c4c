Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The MIR Join-Source Helper

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: migrated
`mir::find_bir_current_block_join_source_identity(...)` in
`src/backend/mir/query.cpp` to build current-block join-source facts from Route
5 current-block join-source records.

The helper now maps Route 5 join statuses into the existing MIR status shape,
converts Route 5 destination/source identities into the existing MIR fact
fields, and derives named source-producer identity from the Route 5 record
metadata. The destination/source fact set no longer comes from the prior direct
PHI scan plus same-block producer query path. Incoming expression closure is
still populated for the existing MIR result, but it now uses the existing Route
1 producer index instead of the previous same-block query helper.

## Suggested Next

Execute Step 4 from `plan.md`: migrate the selected consumer,
`aarch64_codegen::build_current_block_join_prepared_query_routing(...)`, to
observe the migrated Route 5-backed MIR helper or a narrow facade while keeping
target/prealloc scheduling policy unchanged.

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
  failure is still a later consumer-proof decision point.

## Proof

Ran the exact delegated Step 3 proof:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed, 1/1 test green.

Log path: `test_after.log`.
