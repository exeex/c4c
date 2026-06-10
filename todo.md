Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate The Selected Join-Source Consumer

# Current Packet

## Just Finished

Lifecycle repair after committed Step 3 (`2b3da38d8`): Step 4 should continue
with the selected AArch64 consumer but must not use the full
`backend_aarch64_instruction_dispatch` CTest as its acceptance proof.

Reason: the selected consumer,
`aarch64_codegen::build_current_block_join_prepared_query_routing(...)`, is
covered by
`current_block_join_query_routing_uses_bir_identity_with_prepared_fallback()`
inside `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, but
that function is later in `main()` after a known ambient failure:
`expected selected f64 global readback to feed call ABI move`. The same failure
was reproduced at pre-route commit `963f2b71f`, so it is not Route 5 evidence.

## Suggested Next

Execute Step 4 from `plan.md`: migrate the selected consumer,
`aarch64_codegen::build_current_block_join_prepared_query_routing(...)`, to
observe the migrated Route 5-backed MIR helper or a narrow facade while keeping
target/prealloc scheduling policy unchanged.

Proof route for Step 4:

- Add or extract a bounded isolated AArch64 proof harness that runs
  `current_block_join_query_routing_uses_bir_identity_with_prepared_fallback()`
  or equivalent selected-consumer coverage without first executing the
  unrelated red dispatch cases.
- Keep the existing monolithic `backend_aarch64_instruction_dispatch` coverage
  intact; do not delete, weaken, reorder, or expectation-rewrite the ambient
  failing case to make Step 4 pass.
- Delegate the Step 4 proof as:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test <new-isolated-aarch64-route5-join-routing-target> && ctest --test-dir build -R '^(backend_prepared_lookup_helper|<new-isolated-aarch64-route5-join-routing-test>)$' --output-on-failure) > test_after.log 2>&1
```

Replace the placeholder target/test names with the actual narrow harness names
chosen by the executor.

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

## Proof

No tests run for this lifecycle-only proof-route repair.

Last committed Step 3 proof was:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Result: passed, 1/1 test green.

Log path: `test_after.log`.
