Status: Active
Source Idea Path: ideas/open/455_dependency_operand_authority_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Authority Population Evidence

# Current Packet

## Just Finished

Closed idea 454 as complete for dependency-operand authority metadata
representation. Step 4 separated remaining work into concrete population and
printing for representative edge dependency operands before any RV64 consumer
route can be selected.

## Suggested Next

Execute Step 1 for idea 455. Re-read:

- `ideas/open/455_dependency_operand_authority_population.md`
- `ideas/closed/454_edge_dependency_operand_materialization_authority.md`
- `build/agent_state/454_step4_residual_disposition/disposition.md`
- `build/agent_state/454_step3_dependency_operand_authority_metadata/summary.md`
- `build/agent_state/454_step2_dependency_operand_authority_contract/contract.md`
- `build/agent_state/454_step1_dependency_operand_metadata_audit/audit.md`
- `build/agent_state/453_step1_stack_slot_pointer_dependency_audit/20010329-1.prepared.out`

Create `build/agent_state/455_step1_authority_population_audit/` and record a
bucket table for `%t17` and adjacent edge dependency operands: edge identity,
dependency operand identity, value home, object linkage, cast/source identity,
freshness/clobber facts, current planner inputs, prepared printing, and first
missing population fact. Do not edit implementation in Step 1.

## Watchouts

- Do not route to RV64 target lowering from metadata type existence alone.
- Do not treat `%t17` stack home plus object metadata as sufficient
  `load_from_stack_slot` authority.
- Do not treat raw `inttoptr` plus `%t16` immediate as sufficient
  `rematerialize_cast_from_source` authority.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle activation validation:

```sh
git diff --check
```

Result: passed.
