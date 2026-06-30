Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 456. Disposition artifact:
`build/agent_state/456_step4_residual_disposition/disposition.md`.

Fresh `20010329-1` probes under
`build/agent_state/456_step4_residual_disposition/` show:

| Row | Evidence | Disposition |
| --- | --- | --- |
| `%t17` cast dependency for `%t18 -> %t22` | `policy=rematerialize_cast_from_source status=available`, `operand_role=rhs`, `cast_source=%t16`, `cast_source_home=rematerializable_immediate` | Accepted by the Step 3 focused RV64 consumer route |
| `%t17` stack-load alternative | `policy=load_from_stack_slot status=missing_stack_freshness` | Still fail-closed; not owned by idea 456 |
| `%t17` stack-slot publication move | `move_bundle phase=before_instruction authority=none block_index=4 instruction_index=1`, `destination_storage=stack_slot`, `reason=consumer_register_to_stack` | Current first object-route blocker before the edge consumer can benefit from the explicit cast authority |

The prepared probe exits 0. The object probe exits 2 with:

```text
unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

Decision: idea 456 is not close-ready for the representative route yet. The
explicit cast-dependency compare consumer is implemented and covered, but one
precise remaining consumer packet exists for the same authority family: the
object route must suppress or safely consume the owned `before_instruction`
stack-slot publication for an authorized `rematerialize_cast_from_source`
dependency result when carrier-use proof says the cast result is needed only by
the select-edge source-producer compare.

## Suggested Next

Step 5: `Suppress Authorized Cast-Dependency Stack Publication Bundle`.

Implement or route the exact remaining RV64 consumer packet for the
`before_instruction` stack-slot publication of an authorized
`rematerialize_cast_from_source status=available` dependency result. The packet
should use the populated dependency-operand authority and carrier-use proof as
authority, preserve the existing scratch-clobber guard, and keep generic
stack-slot moves, `load_from_stack_slot missing_stack_freshness`, raw
`inttoptr`, successor-copy, and unrelated move bundles fail-closed.

Suggested owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/456_step5_cast_dependency_stack_publication/*`

Suggested proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Full `20010329-1` object success remains unclaimed.
- The next packet should target only the authorized cast-dependency
  `before_instruction` stack publication bundle; do not implement generic stack
  moves.
- Continue to consume only populated `rematerialize_cast_from_source
  status=available` dependency-operand authority.
- Preserve the scratch-clobber guard unless a later packet implements a
  register-safe materialization ordering or scratch allocator.
- Do not infer from raw `inttoptr`, stack homes, object metadata, filenames,
  function names, block names, or one prepared dump.
- Do not consume `load_from_stack_slot` while the row is
  `missing_stack_freshness`.
- Do not copy successor/join-block compare results on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, baseline logs, or `review/`.

## Proof

Step 4 proof:

```sh
git diff --check
```

Result: passed.
