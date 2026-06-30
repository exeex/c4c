Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Cast Dependency Consumer Evidence

# Current Packet

## Just Finished

Closed idea 455 as complete for dependency-operand authority population and
prepared printing. The representative `%t17` dependency now has a populated
`policy=rematerialize_cast_from_source status=available` record, while the
parallel stack-load route remains fail-closed as `missing_stack_freshness`.

## Suggested Next

Execute Step 1 for idea 456. Re-read:

- `ideas/open/456_rv64_select_edge_cast_dependency_consumer.md`
- `ideas/closed/455_dependency_operand_authority_population.md`
- `build/agent_state/455_step4_residual_disposition/disposition.md`
- `build/agent_state/455_step3_dependency_operand_population/summary.md`
- `build/agent_state/455_step3_dependency_operand_population/20010329-1.prepared.out`

Create `build/agent_state/456_step1_cast_dependency_consumer_audit/` and
record a bucket table for edge identity, source producer, operand role,
dependency value, cast producer, cast source, source home, selected
policy/status, stack-load fail-closed row, and current RV64 move-bundle
failure. Do not edit implementation in Step 1.

## Watchouts

- Consume only populated `rematerialize_cast_from_source status=available`
  dependency-operand authority.
- Do not infer from raw `inttoptr`, stack homes, object metadata, filenames,
  function names, block names, or one prepared dump.
- Do not consume `load_from_stack_slot` while the row is
  `missing_stack_freshness`.
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
