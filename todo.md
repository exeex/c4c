Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Argument And Result Source Authority
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed Step 3.1 "Argument And Result Source Authority" packet work for idea
88 by publishing authoritative frame-slot identity for stack-backed scalar call
results in `PreparedCallPlan`, printing that slot identity in prepared dumps,
and tightening backend/prealloc tests around a deterministic multi-call spill
fixture so consumers do not need to recover the prepared result slot from raw
offsets or target-local stack reasoning.

Current packet result:
- `PreparedCallResultPlan` now carries `destination_slot_id` alongside
  `destination_stack_offset_bytes` for frame-slot-backed scalar call results.
- `populate_call_plans` now preserves the prepared frame-slot id when a call
  result's semantic destination home is a spill/home slot.
- Prepared call-plan dumps now print `destination_slot=#...` for stack-backed
  scalar call results.
- Backend contract and printer tests now prove spilled scalar call results
  publish both their ABI source register and their prepared frame-slot
  identity.

## Suggested Next

Continue Step 3.1 by checking whether indirect-callee publication or any
remaining call-boundary scalar authority still exposes only stack offsets or
register names without the corresponding prepared slot/value identity, then
publish the next missing fact directly in `PreparedCallPlan`.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Spilled scalar call results can still report an ABI source register even when
  their prepared destination home is a frame slot; keep those as separate facts
  rather than collapsing transport into semantic storage.
- The new spill fixture depends on sequential scalar call results remaining
  live long enough to force frame-slot homes; if target register policy changes,
  preserve the underlying result-home contract rather than weakening the test
  expectation to raw offsets only.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
