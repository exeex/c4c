Status: Active
Source Idea Path: ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Edge Placement Evidence

# Current Packet

## Just Finished

Closed idea 457 as complete for audit/contract/disposition. Step 4 classified
the remaining `20010329-1` blocker as missing producer/prepared placement
metadata for the before-instruction register-destination bundle at
`block_index=4 instruction_index=2`, not as a safe RV64 lowering packet.

## Suggested Next

Execute Step 1 for idea 458. Re-read:

- `ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md`
- `ideas/closed/457_before_instruction_stack_to_register_move_materialization.md`
- `build/agent_state/457_step4_residual_disposition/disposition.md`
- `build/agent_state/457_step3_register_destination_move_materialization/blocker.md`
- `build/agent_state/456_step7_final_residual_disposition/20010329-1.prepared.out`

Create `build/agent_state/458_step1_select_edge_placement_audit/` and record a
bucket table for the target bundle: phase, block/instruction coordinates,
moves, source producer, select carrier, edge transfer, source/destination
homes, existing source-producer facts, and first missing placement authority.
Do not edit implementation in Step 1.

## Watchouts

- Do not route to RV64 lowering before placement authority exists.
- Do not infer predecessor/successor identity or suppression authority from
  value ids, block indexes, instruction indexes, raw BIR shape, filenames,
  function names, or one prepared dump.
- Do not reopen idea 456 cast-dependency consumption.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
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

Result: passed.
