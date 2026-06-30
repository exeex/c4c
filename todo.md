Status: Active
Source Idea Path: ideas/open/459_rv64_select_edge_suppression_placement_consumer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define RV64 Suppression Consumer Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 459. RV64 suppression is accepted only for
an explicit available `predecessor_edge_consumed_suppression` placement record
that matches the same function, before-instruction move-bundle site, binary
source producer, select carrier, predecessor/successor edge tuple,
incoming/destination values, and register-destination target moves.

Accepted suppression conditions:

- collect `PreparedSelectEdgeSourceProducerPlacementRecords` for the prepared
  function;
- match `status=available` and
  `kind=predecessor_edge_consumed_suppression`;
- match function identity, phase, block index, instruction index, binary
  source-producer identity/result, select-materialization carrier, edge
  predecessor/successor labels, incoming value, destination value, and target
  move source/destination value ids;
- require `BeforeInstruction` `authority=none` target bundles whose moves are
  value moves with register destination storage and whose destinations match
  the source-producer result;
- suppress as an empty RV64 fragment only after that explicit match.

Event visibility requirement:

- the object route must observe before-instruction register-destination bundles
  only through a narrow placement-record-backed visibility path;
- absence of a matching available placement record remains fail-closed and must
  not become generic before-instruction register move lowering.

Artifact:
`build/agent_state/459_step2_suppression_consumer_contract/contract.md`.

## Suggested Next

Step 3: Implement Or Route First Suppression Consumer Packet.

Owned files/tests for the selected packet:

- `src/backend/prealloc/prepared_object_traversal.hpp`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/prealloc/publication_plans.hpp` and
  `src/backend/prealloc/publication_plans.cpp` only if a focused lookup helper
  is needed
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/459_step3_suppression_consumer/**`

Implement the narrow route that collects placement records, exposes the
authorized register-destination before-instruction bundle to RV64 object
emission, suppresses only the matching available record, and keeps missing,
mismatched, generic, stale stack-load, and raw-inferred cases rejected.

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Consume only explicit `predecessor_edge_consumed_suppression` placement
  metadata.
- RV64 currently has two consumer gaps that Step 3 must handle together:
  placement records are not collected in object emission, and prepared
  traversal does not expose the target register-destination before-instruction
  bundle.
- Do not infer suppression from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen idea 456 cast-dependency consumption.
- Keep generic stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Lifecycle activation validation:

```sh
git diff --check
```

Result: passed for Step 2 contract.
