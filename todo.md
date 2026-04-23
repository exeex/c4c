Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Call Boundary Authority Completion
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed Plan Step 3 "Call Boundary Authority Completion" for idea 88 by
publishing explicit memory-return/sret authority in `PreparedCallPlan`,
printing it in prepared dumps, and tightening backend/prealloc tests so
downstream targets no longer need to recover aggregate-return destination
ownership from raw BIR `sret_storage_name` or a separate stack-layout join.

Current packet result:
- `PreparedCallPlan` now publishes a `memory_return` authority record for calls
  whose `result_abi` returns in memory, including the sret ABI-argument index,
  destination slot identity, prepared frame-slot publication, and size/alignment.
- `populate_call_plans` now snapshots aggregate-return destination ownership
  from `sret_storage_name` plus prepared stack-layout publication at call-plan
  construction time instead of leaving target backends to rejoin raw BIR and
  frame-slot state later.
- Prepared dumps now expose the published memory-return authority in both the
  summary callsite view and the `prepared-call-plans` detail section.
- Backend/prealloc tests now prove aggregate-return call contracts and printer
  dumps directly from prepared call plans.

## Suggested Next

Continue Step 3 by auditing whether any remaining call-boundary facts still
require target-local recovery outside `PreparedCallPlan`, with the best next
packet likely focused on any still-unpublished computed-address or multi-lane
aggregate call authorities if target consumers still need extra joins.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- If later aggregate returns use non-frame-slot or multi-lane storage, publish
  that shape in `PreparedCallPlan` instead of sending backends back to raw BIR
  or storage-plan side channels.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
