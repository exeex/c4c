Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Call Boundary Authority Completion
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed Plan Step 3 "Call Boundary Authority Completion" for idea 88 by
publishing explicit indirect-callee operand and home authority in
`PreparedCallPlan`, printing it in prepared dumps, and tightening
backend/prealloc tests so downstream targets no longer need to recover the
indirect callee operand from raw BIR or a separate storage-plan join.

Current packet result:
- `PreparedCallPlan` now publishes an `indirect_callee` authority record for
  indirect calls, including the callee value identity plus prepared home facts
  such as encoding, bank, register/slot, and computed-address base+delta when
  available.
- `populate_call_plans` now snapshots indirect callee authority from prepared
  value-location and regalloc state at call-plan publication time instead of
  leaving target backends to recover the callee operand from raw BIR.
- Prepared dumps now expose the published indirect callee authority in both the
  summary callsite view and the `prepared-call-plans` detail section.
- Backend/prealloc tests now prove indirect-call contracts and wrapper dumps
  directly from prepared call plans rather than consulting raw BIR to learn
  which value drives the call target.

## Suggested Next

Continue Step 3 by auditing whether any remaining call-boundary facts still
require target-local recovery outside `PreparedCallPlan`, with the best next
packet likely focused on memory-return/sret or computed-address call-target
authority if target consumers still need extra joins.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep indirect-callee authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- If later indirect callees arrive as computed addresses, keep publishing the
  pointer-base and byte-delta facts in call plans instead of sending backends
  back to raw BIR.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
