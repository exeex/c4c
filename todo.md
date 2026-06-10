Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Contract Only Proven-Private Prepared Surface

# Current Packet

## Just Finished

Step 5 (`Contract Only Proven-Private Prepared Surface`): re-scanned the
prepared current-block join-source helper exposure and found no surface proven
private enough for safe contraction in this packet. No implementation files were
changed.

Direct remaining consumers from the scan:

- `prepare::PreparedCurrentBlockJoinParallelCopySource*` types and
  `prepare::prepare_current_block_join_parallel_copy_source_facts(...)` remain
  production prealloc API in `src/backend/prealloc/publication_plans.hpp/.cpp`,
  are used by `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`, and are
  directly covered by
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- `aarch64_codegen::prepared_query_current_block_join_parallel_copy_source(...)`
  remains a production AArch64 codegen helper declared in
  `dispatch_producers.hpp`, defined in `dispatch_producers.cpp`, and consumed
  from `dispatch_value_materialization.cpp`.
- `aarch64_codegen::build_current_block_join_prepared_query_routing(...)`,
  `current_block_join_prepared_query_incoming_expression(...)`, and
  `current_block_join_prepared_query_source(...)` remain production dispatch
  routing helpers consumed by `dispatch.cpp` and directly exercised by
  `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp` and
  the existing `backend_aarch64_instruction_dispatch` helper path.

## Suggested Next

Step 6 or lifecycle close: have the supervisor decide whether the remaining
public prepared current-block join-source seams are acceptable close-level
coverage, or whether a follow-up plan-owner/reviewer pass should split a new
initiative for migrating the remaining production/test seams.

## Watchouts

- Keep Route 5 limited to edge publication identity and current-block
  join-source semantic identity.
- Do not import move bundle order, parallel-copy scheduling, cycle temporary
  routing, execution site policy, carrier/phase policy, coalescing,
  redundancy, destination registers, storage-sharing checks, prepared move
  records, or AArch64 edge-copy emission policy into BIR.
- Step 5 found no prepared current-block join-source helper that is both within
  this packet's owned files and proven private by direct `rg` evidence.
- Do not hide the remaining prepared current-block join-source helpers without
  first migrating or deliberately replacing their production/test consumers.
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
(rg -n 'PreparedCurrentBlockJoin|prepare_current_block_join|prepared_query_current_block_join|current_block_join_prepared_query' src tests -S && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing)$' --output-on-failure) > test_after.log 2>&1
```

Result: passed, scan completed and 2/2 tests green.

Log path: `test_after.log`.
