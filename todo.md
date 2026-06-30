Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Expose Authorized Before-Instruction Move Bundle To Object Route

# Current Packet

## Just Finished

Plan-owner reviewed the Step 5 blocker for idea 456 and kept the source idea
active. Blocker artifact:
`build/agent_state/456_step5_cast_dependency_stack_publication/blocker.md`.

Fresh `20010329-1` probes under
`build/agent_state/456_step5_cast_dependency_stack_publication/` still show:

```text
prepared_exit=0
object_exit=2
error: --codegen obj failed: RISC-V backend object route unsupported prepared module shape: unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

The relevant prepared facts are still coherent for this idea:

| Fact | Evidence |
| --- | --- |
| Stack publication bundle | `move_bundle phase=before_instruction authority=none block_index=4 instruction_index=1`, `from_value_id=8 to_value_id=9 destination_storage=stack_slot reason=consumer_register_to_stack` |
| Cast dependency authority | `dependency=%t17 dependency_value_id=9 policy=rematerialize_cast_from_source status=available`, `cast_source=%t16 cast_source_value_id=8 cast_source_home=rematerializable_immediate` |
| Rejected adjacent policy | `policy=load_from_stack_slot status=missing_stack_freshness` |

Blocker: the prepared dump prints the required `before_instruction` move-bundle
fact, but the current RV64 object traversal does not emit before-instruction
move-bundle events to `fragment_for_prepared_move_bundle`. A local
`object_emission.cpp` suppression helper for this exact stack publication is
therefore unreachable from the delegated consumer hook. The remaining packet
is same-family because it only exposes the event needed by the authorized
cast-dependency consumer route.

## Suggested Next

Execute Step 6: `Expose Authorized Before-Instruction Move Bundle To Object Route`.

Add or route the smallest prepared object traversal/consumer surface that lets
the RV64 object route see the exact `before_instruction` move bundle for an
authorized cast-dependency stack publication. Only after that event is exposed
should object emission suppress or safely consume the
`rematerialize_cast_from_source status=available` stack publication. Keep
generic stack moves, `load_from_stack_slot missing_stack_freshness`, raw
`inttoptr`, successor-copy, scratch-clobber, and unrelated move bundles
fail-closed.

Suggested owned files:

- `src/backend/prealloc/prepared_object_traversal.hpp`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/456_step6_before_instruction_move_bundle/*`

Suggested proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Full `20010329-1` object success remains unclaimed.
- The delegated Step 5 local object-emission hook is blocked because the needed
  before-instruction move bundle is not emitted as an object traversal event.
- The next packet should expose only the authorized cast-dependency
  `before_instruction` stack publication event; do not implement generic stack
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
