Status: Active
Source Idea Path: ideas/open/501_rv64_before_instruction_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Before-Instruction Prepared Move Surfaces

# Current Packet

## Just Finished

Step 1 inspected the before-instruction prepared move-bundle publication and
RV64 consumption surfaces for idea 501. Durable evidence is recorded under
`build/agent_state/501_step1_before_instruction_move_surfaces/`:
`representative_rows.tsv` and `summary.md`.

The focused family remains 328 coherent before-instruction consumer moves:
278 `consumer_register_to_stack` rows and 50 `consumer_stack_to_stack` rows.
`PreparedMoveResolution` plus `PreparedMoveBundle` publish the needed
function/phase/authority/block/instruction coordinate, source value id,
destination value id, destination storage, op kind, and reason. RV64 currently
rejects these rows because `fragment_for_prepared_move_bundle` only accepts
register destinations, but it already has the source-home lookup,
destination-home lookup path, stack-slot offset helper, scalar size helper, and
`append_rv64_store_register_to_stack` emission helper needed for a narrow
register-to-stack packet.

## Suggested Next

Execute Step 2 by implementing the narrow
`consumer_register_to_stack` before-instruction materialization path only. Guard
on a `BeforeInstruction` bundle with authority `None`, move op kind `Move`,
stack-slot destination storage, source home resolving to a GPR, destination home
resolving to a scalar stack slot, and exact prepared coordinates. Leave
`consumer_stack_to_stack` for a separate temporary-register/load-then-store
packet.

## Watchouts

- Do not implement out-of-SSA, before-return, or select-publication move
  handling under idea 501.
- Do not infer move authority, destination storage, source storage, or consumer
  ordering from testcase names, raw BIR shape, case-log text, final homes, or
  object output.
- Source storage is not printed in current case logs; Step 2 must derive the
  source route from `move.from_value_id` and prepared value homes.
- Destination stack offset must come from the destination prepared value home
  and stack layout, not from the testcase or case-log shape.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 1 evidence-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
