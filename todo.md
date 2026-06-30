Status: Active
Source Idea Path: ideas/open/457_before_instruction_stack_to_register_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Before-Instruction Register-Destination Move Bundle

# Current Packet

## Just Finished

Closed idea 456 as complete for explicit populated
`rematerialize_cast_from_source status=available` dependency-operand
consumption and authorized stack-publication suppression. Step 7 classified
the remaining `20010329-1` failure as a separate before-instruction
move-bundle/materialization family, especially the later
`consumer_stack_to_register` move at `block_index=4 instruction_index=2`.

## Suggested Next

Execute Step 1 for idea 457. Re-read:

- `ideas/open/457_before_instruction_stack_to_register_move_materialization.md`
- `ideas/closed/456_rv64_select_edge_cast_dependency_consumer.md`
- `build/agent_state/456_step7_final_residual_disposition/disposition.md`
- `build/agent_state/456_step7_final_residual_disposition/20010329-1.prepared.out`
- `build/agent_state/456_step7_final_residual_disposition/20010329-1.object.err`

Create `build/agent_state/457_step1_before_instruction_register_destination_audit/`
and record a bucket table for each move in `block_index=4 instruction_index=2`:
source value, destination value, source home, destination home, reason,
carrier use, scratch/clobber risk, traversal event, and first missing
authority. Do not edit implementation in Step 1.

## Watchouts

- Do not reopen explicit cast-dependency authority consumption closed by idea
  456.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not infer materialization authority from raw BIR shape, filenames,
  function names, block names, testcase shape, value ids alone, or one
  prepared dump.
- Keep general stack-home branch consumer work routed to
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

Result: passed.
