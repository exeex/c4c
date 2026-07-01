Status: Active
Source Idea Path: ideas/open/501_rv64_before_instruction_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Register-To-Stack Consumer Moves

# Current Packet

## Just Finished

Step 2 implemented RV64 materialization for coherent prepared
before-instruction `consumer_register_to_stack` move bundles only. Durable
evidence is recorded under
`build/agent_state/501_step2_register_to_stack_moves/summary.md`.

`fragment_for_prepared_move_bundle` now admits stack-slot destinations only for
the exact Step 2 shape: `BeforeInstruction` bundle, authority `None`, move op
kind `Move`, destination kind `Value`, destination storage `StackSlot`, no
cycle-temp source, no synthetic destination stack offset, no source immediate,
source home resolving to a GPR, destination home resolving to a scalar stack
slot, and reason `consumer_register_to_stack`. Emission uses the existing
`append_rv64_store_register_to_stack` helper. Register-destination handling is
left on the existing path.

Focused RV64 object-emission coverage now proves an accepted
register-to-stack before-instruction bundle emits `sw t0, 16(sp)` before the
prepared instruction and keeps adjacent shapes fail-closed for wrong reason,
stack source, wrong phase, and synthetic destination offset.

## Suggested Next

Execute Step 3 by splitting or implementing the residual
`consumer_stack_to_stack` before-instruction family as a separate packet. That
packet needs an explicit temporary-register load-then-store policy and must keep
out-of-SSA, before-return, and select-publication handling out of scope.

## Watchouts

- Do not implement out-of-SSA, before-return, or select-publication move
  handling under idea 501.
- Do not infer move authority, destination storage, source storage, or consumer
  ordering from testcase names, raw BIR shape, case-log text, final homes, or
  object output.
- Stack-to-stack is still intentionally fail-closed for this packet.
- Source and destination routes must continue to come from `move.from_value_id`,
  `move.to_value_id`, prepared value homes, and stack layout.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 2 implementation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
