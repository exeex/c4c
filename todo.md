Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Suppress Authorized Cast-Dependency Stack Publication Bundle

# Current Packet

## Just Finished

Plan-owner reviewed Step 4 for idea 456 and kept the source idea active.
Supporting artifact:
`build/agent_state/456_step4_residual_disposition/disposition.md`.

The explicit cast-dependency compare consumer is implemented and covered, but
the representative `20010329-1` object route still fails at
`unsupported_move_bundle_target_shape` before the edge consumer can benefit
from that authority. The remaining same-family packet is the
`before_instruction` stack-slot publication for `%t17`:

```text
move_bundle phase=before_instruction authority=none block_index=4 instruction_index=1
move from_value_id=8 to_value_id=9 destination_storage=stack_slot reason=consumer_register_to_stack
```

The packet must use the populated `rematerialize_cast_from_source
status=available` dependency-operand authority and carrier-use proof for the
owned cast result. No lifecycle split is needed.

## Suggested Next

Execute Step 5: `Suppress Authorized Cast-Dependency Stack Publication Bundle`.

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

Lifecycle repair validation:

```sh
git diff --check
```

Result: passed.
