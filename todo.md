Status: Active
Source Idea Path: ideas/open/452_select_edge_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Edge Rematerialization Evidence

# Current Packet

## Just Finished

Closed idea 450 as blocked by a split after Step 4 classified
`20010329-1` at `unsupported_move_bundle_target_shape`. The direct
select-result branch facts are present, but the predecessor-edge move for
`%t22` cannot soundly copy incoming `%t18` because `%t18` is produced in the
successor/join block. A durable source idea now owns the required
select-edge source-producer dependency rematerialization route.

## Suggested Next

Execute Step 1 for idea 452. Re-read:

- `ideas/open/452_select_edge_source_producer_rematerialization.md`
- `ideas/closed/450_select_result_branch_publication.md`
- `build/agent_state/450_step4_residual_disposition/disposition.md`
- `build/agent_state/450_step3_select_result_branch_publication/blocker.md`
- `build/agent_state/450_step4_residual_disposition/20010329-1.prepared.out`
- `build/agent_state/450_step4_residual_disposition/20010329-1.object.err`

Create `build/agent_state/452_step1_select_edge_rematerialization_audit/` and
record a bucket table for each candidate edge, incoming value, producer
instruction, dependency operand, value home, edge availability, prepared
source-producer fact, and first missing authority. Do not edit implementation
in Step 1.

## Watchouts

- Do not copy `%t18` on the predecessor edge unless edge availability is
  proven.
- Do not infer source-producer dependencies from raw BIR shape, block order,
  filenames, function names, or one prepared dump layout.
- Keep stack-home branch operand materialization routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md` unless Step 2
  explicitly accepts a narrow overlap as part of edge rematerialization.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Keep `20000622-1` out of the first packet while its first owner is
  instruction-side lowering.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle activation validation:

```sh
git diff --check
```

Result: passed.
