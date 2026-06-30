Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define RV64 Cast Dependency Consumer Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 456. Contract artifact:
`build/agent_state/456_step2_cast_dependency_consumer_contract/contract.md`.

Accepted RV64 consumer shape:

- prepared move bundle is an out-of-SSA/block-entry edge move with matching
  predecessor/successor, incoming source, and destination value identity;
- destination has an available register publication/home;
- source producer is a prepared binary compare available for select-edge
  rematerialization;
- dependency operand role and dependency value id/name/type match the compare
  operand;
- dependency-operand authority record has
  `policy=rematerialize_cast_from_source` and `status=available`;
- cast producer, dependency value, cast source, and source home all match the
  prepared authority record;
- cast source home is a GPR-compatible register or rematerializable immediate;
- RV64 materializes the cast dependency before rematerializing the compare into
  the edge destination register.

Rejected shapes:

| Shape | Reason |
| --- | --- |
| Missing dependency-operand authority | Raw BIR and homes are not authority |
| Non-available authority | No safe consumer contract |
| `load_from_stack_slot`, including `missing_stack_freshness` | Freshness/clobber policy is not owned here |
| Raw `inttoptr` or stack object metadata alone | Producer authority must be explicit |
| Successor/join-block result copy such as `%t18` | Source result is not available on predecessor edge |
| Mismatched edge/source/destination/producer/operand/cast/source facts | Cannot bind consumer to the prepared record |
| Unsupported cast kind, width, aggregate/vector/F128, or non-GPR source home | Outside first RV64 scalar packet |

Materialization order:

1. Match the prepared edge move bundle and destination publication.
2. Match the source-producer compare and dependency operand role.
3. Match the available `rematerialize_cast_from_source` authority record.
4. Materialize the cast source from its prepared home.
5. Materialize the pointer cast dependency as scalar GPR movement, not a stack
   load.
6. Rematerialize the compare using that dependency operand.
7. Publish the compare result into the prepared edge destination register.

## Suggested Next

Step 3: `Implement Or Route First Consumer Packet`.

Implement the narrow RV64 consumer for explicit
`rematerialize_cast_from_source status=available` dependency-operand authority,
or record the exact blocker if the existing RV64 select-edge materializer lacks
a safe scratch/register path. The packet should use the prepared authority
record as the only source of permission and keep stack-load, raw `inttoptr`,
and successor-copy paths fail-closed.

Suggested owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp` only if helper
  coverage adjustment is needed
- `todo.md`
- `test_after.log`
- `build/agent_state/456_step3_cast_dependency_consumer/*`

Proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Consume only populated `rematerialize_cast_from_source status=available`
  dependency-operand authority.
- Do not infer from raw `inttoptr`, stack homes, object metadata, filenames,
  function names, block names, or one prepared dump.
- Do not consume `load_from_stack_slot` while the row is
  `missing_stack_freshness`.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
