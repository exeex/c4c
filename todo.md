Status: Active
Source Idea Path: ideas/open/501_rv64_before_instruction_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Stack-To-Stack Consumer Moves

# Current Packet

## Just Finished

Step 3 implemented RV64 materialization for coherent prepared
before-instruction `consumer_stack_to_stack` move bundles only. Durable evidence
is recorded under
`build/agent_state/501_step3_stack_to_stack_moves/summary.md`.

`fragment_for_prepared_move_bundle` now admits stack-to-stack only under the
same prepared structural guards as Step 2 plus reason `consumer_stack_to_stack`.
The source and destination are derived from `move.from_value_id`,
`move.to_value_id`, prepared value homes, and stack layout. The destination must
resolve to a scalar stack slot; the source must resolve to a stack slot with the
same scalar size. RV64 uses an explicit load-then-store policy through an
unoccupied temporary GPR chosen from `t1`, `t2`, and `t3`; if all candidates are
published as prepared GPR homes, the shape remains fail-closed.

Focused RV64 object-emission coverage now proves an accepted stack-to-stack
before-instruction bundle emits `lw t1, 8(sp); sw t1, 16(sp)` before the
prepared instruction and keeps adjacent shapes fail-closed for missing source
stack facts, scalar-size mismatch, no available scratch GPR, and reason
confusion back into the register-to-stack path.

## Suggested Next

Execute Step 4 residual disposition for idea 501. Record that the two coherent
before-instruction consumer move families from idea 495 are now implemented and
route remaining move-bundle families to their separate follow-up ideas rather
than extending idea 501.

## Watchouts

- Do not implement out-of-SSA, before-return, or select-publication move
  handling under idea 501.
- Do not infer move authority, destination storage, source storage, or consumer
  ordering from testcase names, raw BIR shape, case-log text, final homes, or
  object output.
- Stack-to-stack is admitted only with an unoccupied scratch GPR; if prepared
  homes occupy `t1`, `t2`, and `t3`, it remains fail-closed.
- Source and destination routes must continue to come from `move.from_value_id`,
  `move.to_value_id`, prepared value homes, and stack layout.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 3 implementation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
