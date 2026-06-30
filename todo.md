Status: Active
Source Idea Path: ideas/open/459_rv64_select_edge_suppression_placement_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Suppression Placement Consumer Evidence

# Current Packet

## Just Finished

Closed idea 458 as complete for producer/prepared
`predecessor_edge_consumed_suppression` placement authority. Step 4 classified
remaining RV64 consumption as separate from producer metadata.

## Suggested Next

Execute Step 1 for idea 459. Re-read:

- `ideas/open/459_rv64_select_edge_suppression_placement_consumer.md`
- `ideas/closed/458_select_edge_source_producer_move_bundle_placement_authority.md`
- `build/agent_state/458_step4_residual_disposition/disposition.md`
- `build/agent_state/458_step3_select_edge_placement_metadata/summary.md`
- `build/agent_state/458_step2_placement_authority_contract/contract.md`

Create `build/agent_state/459_step1_suppression_placement_consumer_audit/` and
record a bucket table for the target bundle: placement record fields, edge
identity, source producer, select carrier, bundle site, moves, current RV64
event visibility, and first missing consumer fact. Do not edit implementation
in Step 1.

## Watchouts

- Consume only explicit `predecessor_edge_consumed_suppression` placement
  metadata.
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

Result: passed.
