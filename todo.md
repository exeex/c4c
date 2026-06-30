Status: Active
Source Idea Path: ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Stack-Slot Pointer Dependency Evidence

# Current Packet

## Just Finished

Closed idea 452 as complete for register/immediate select-edge
source-producer rematerialization. Step 4 evidence shows the remaining
`20010329-1` failure is the `%t18 -> %t22` edge, where `%t18` depends on
`%t17 = inttoptr i32 %t16 to ptr` and `%t17` has stack-slot pointer home
`slot_id=2 offset=16`. That shape was rejected from idea 452's selected packet
and now belongs to this narrower stack-slot pointer select-edge dependency
materialization idea.

## Suggested Next

Execute Step 1 for idea 453. Re-read:

- `ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md`
- `ideas/closed/452_select_edge_source_producer_rematerialization.md`
- `build/agent_state/452_step4_residual_disposition/disposition.md`
- `build/agent_state/452_step3_select_edge_rematerialization/summary.md`
- `build/agent_state/452_step2_rematerialization_contract/contract.md`
- `build/agent_state/452_step4_residual_disposition/20010329-1.prepared.out`
- `build/agent_state/452_step4_residual_disposition/20010329-1.object.err`

Create `build/agent_state/453_step1_stack_slot_pointer_dependency_audit/` and
record a bucket table for the edge, incoming value, compare producer, `%t17`
cast producer, stack-slot home, slot metadata, source immediate, edge
placement, target register, current prepared authority, and first missing
fact. Do not edit implementation in Step 1.

## Watchouts

- Do not copy `%t18` from its successor/join-block register home on the
  predecessor edge.
- Do not load or rematerialize `%t17` from a stack slot without explicit
  prepared authority for slot identity, width, freshness, clobber safety, edge
  placement, and target compatibility.
- Do not infer dependency materialization authority from raw `inttoptr`,
  stack-slot spelling, block order, filenames, function names, or one prepared
  dump layout.
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
