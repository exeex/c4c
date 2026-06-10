Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The MIR Join-Source Helper

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: confirmed and tightened Route 5 oracle
coverage for `mir::find_bir_current_block_join_source_identity(...)` before any
MIR helper implementation migration.

Coverage now asserted in `backend_prepared_lookup_helper`:
- available current-block joins for named, immediate, and stack-like source
  PHI records, with prepared/BIR agreement and direct Route 5 record/index
  checks
- no-source / no-publication current-block behavior through the no-PHI
  `MissingPublication` case
- missing successor block through MIR `MissingBlock` and direct Route 5
  `MissingSuccessor` records
- successor-label mismatch through MIR `MissingSuccessorLabel`
- fail-closed destination/source cases through Route 5 destination type
  mismatch, missing source value, missing destination, and missing source
  producer statuses, plus the matching MIR missing source producer category
- memory-source oracle coverage remains confirmed in the existing Route 5
  CFG-edge load-local coverage in the same test file; current-block join-source
  identity should not import memory-addressing policy

## Suggested Next

Execute Step 3 from `plan.md`: migrate
`mir::find_bir_current_block_join_source_identity(...)` in
`src/backend/mir/query.cpp` to source its current-block join facts from Route 5
records/index lookups while preserving the existing MIR request/result/status
shape when possible.

## Watchouts

- Keep Route 5 limited to edge publication identity and current-block
  join-source semantic identity.
- Do not import move bundle order, parallel-copy scheduling, cycle temporary
  routing, execution site policy, carrier/phase policy, coalescing,
  redundancy, destination registers, storage-sharing checks, prepared move
  records, or AArch64 edge-copy emission policy into BIR.
- Do not hide prepared current-block join-source helpers until direct
  consumers have been re-scanned and migrated or proven out of scope.
- Prefer `Route5EdgeJoinSourceIndex` and
  `bir::route5_find_current_block_join_source(...)` for typed destination/source
  lookups. If the existing MIR result still needs destination-name-only
  aggregate lists, use Route 5 join records directly from the index rather than
  reintroducing a broad BIR PHI scan.
- The selected MIR helper has no predecessor field in its request shape.
  Predecessor absence is covered by Route 5 edge publication oracle/status
  coverage, while current-block join migration should preserve predecessor
  labels carried by PHI incoming records.
- Do not include `backend_aarch64_instruction_dispatch` in the Step 3 narrow
  proof unless the supervisor explicitly selects it. The known ambient dispatch
  failure is still a later consumer-proof decision point.

## Proof

Ran the exact delegated Step 2 proof:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed, 1/1 test green.

Log path: `test_after.log`.
