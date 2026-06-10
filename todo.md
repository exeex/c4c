Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Route 5 Oracle Coverage

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: inventoried residual Route 5 current-block
join-source users and selected the first migration target.

Selected residual helper:
- `mir::find_bir_current_block_join_source_identity(...)` in
  `src/backend/mir/query.cpp`, declared in `src/backend/mir/query.hpp`.

Selected consumer:
- `aarch64_codegen::build_current_block_join_prepared_query_routing(...)` in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`. It already observes
  the MIR helper and falls back to prepared current-block join-source facts, so
  it is the narrow consumer for Step 4 after the MIR helper migration.

Target files for implementation:
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp` only if the existing request/result/status shape
  cannot preserve Route 5 fail-closed statuses.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- Tests in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` and
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.

Out-of-scope or rejected consumers:
- `prepared_query_current_block_join_parallel_copy_source(...)` remains
  prepared-policy fallback for target/prealloc parallel-copy source routing.
- `prepare::prepare_current_block_join_parallel_copy_source_facts(...)` and
  `PreparedCurrentBlockJoinParallelCopySource*` remain prepared helper/oracle
  surfaces until after migrated consumers no longer require them.
- AArch64 edge-copy emission paths in `dispatch.cpp`,
  `dispatch_edge_copies.cpp`, `memory.cpp`, `alu.cpp`, `calls.cpp`, and
  `comparison.cpp` remain out of scope because they consume prepared
  publication, move, storage, register, source-producer, memory-addressing, or
  target emission policy rather than only Route 5 semantic join-source
  identity.
- `PreparedEdgePublicationLookups` and
  `PreparedEdgePublicationSourceProducerLookups` remain public because
  prepared publication planning, source-producer materialization, memory
  addressing, x86/RISC-V tests, and AArch64 target policy still consume them.

Coverage candidates before implementation:
- Positive current-block join-source identity: existing prepared/BIR/Route 5
  agreement around named, immediate, and stack-like source records in
  `backend_prepared_lookup_helper`.
- No-source/fail-closed: Route 5 edge publication `NoSource` and current-block
  missing destination/source cases where the selected helper can preserve
  closed status without weakening prepared fallback.
- Memory-source: Route 5 edge publication records with `LoadLocal` producer and
  `source_memory_identity_available`; keep this as oracle coverage and do not
  pull memory-addressing policy into the current-block join consumer.
- Missing predecessor / missing successor: Route 5 edge publication
  `MissingPredecessor` and MIR current-block `MissingSuccessorLabel` coverage.
- Selected consumer: extend
  `current_block_join_query_routing_uses_bir_identity_with_prepared_fallback()`
  to prove the dispatch routing path observes Route 5-backed MIR identity while
  retaining prepared fallback for absent BIR identity.

## Suggested Next

Execute Step 2 from `plan.md`: add or confirm Route 5 oracle coverage for the
selected helper before changing the MIR helper implementation.

Use the green Route 5 oracle/helper subset for Step 2:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Do not include `backend_aarch64_instruction_dispatch` in the Step 2 proof
baseline. The supervisor already ran the originally proposed combined proof
into `test_before.log`; it failed 1/2 because
`backend_aarch64_instruction_dispatch` still has the known ambient failure
`expected selected f64 global readback to feed call ABI move`, while
`backend_prepared_lookup_helper` passed. That AArch64 failure was previously
proven to predate Route 4 at commit `963f2b71f`, so it is not a Step 2 oracle
prerequisite.

Before Step 4 or close-level validation, the supervisor should either provide a
green isolated consumer proof for
`current_block_join_query_routing_uses_bir_identity_with_prepared_fallback()`,
select a different green Route 5-observing consumer, or split the ambient
AArch64 dispatch failure into a separate blocker idea if the whole dispatch
CTest remains required.

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

## Proof

No build or tests were required for this analysis-only packet. Read-only
inventory used `rg`, `ctest --test-dir build -N`, `c4c-clang-tool`, and
`c4c-clang-tool-ccdb`.

Proposed future proof command:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

`test_after.log` was not touched in this packet.

The previously proposed combined command with
`backend_aarch64_instruction_dispatch_test` is red on current HEAD before
Route 5 Step 2 begins because of the ambient AArch64 dispatch failure noted
above. Treat it as a later consumer-proof decision point, not the Step 2
baseline.
