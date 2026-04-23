Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Argument And Result Source Authority
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed Step 3.1 "Argument And Result Source Authority" packet work for idea
88 by publishing authoritative frame-slot identity for stack-backed call
arguments in `PreparedCallPlan`, printing that slot identity in prepared dumps,
and tightening backend/prealloc tests around a byval/home-slot call fixture so
consumers do not need to recover the backing prepared slot from raw BIR or
side-channel stack-layout inspection.

Current packet result:
- `PreparedCallArgumentPlan` now carries `source_slot_id` alongside
  `source_stack_offset_bytes` for frame-slot-backed arguments.
- `populate_call_plans` now preserves the prepared frame-slot id when a call
  argument's semantic source is a home slot even if the ABI carrier is a
  register.
- Prepared call-plan dumps now print `source_slot=#...` for frame-slot-backed
  call arguments.
- Backend contract and printer tests now prove a byval/home-slot call argument
  publishes both its semantic frame-slot source and its target ABI carrier.

## Suggested Next

Continue Step 3.1 by checking whether stack-backed or spilled scalar call
results still publish only raw offsets without the corresponding prepared
frame-slot identity, then publish the next missing result-side slot/source
authority directly in `PreparedCallPlan`.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Frame-slot-backed call arguments can still travel in registers for ABI
  transport; treat `source_encoding` plus `source_slot_id`/source detail as the
  authoritative origin and the destination register/stack fields as transport.
- `source_slot_id` currently covers call arguments only; if the next packet
  needs the same authority for call results or indirect-callee storage, add it
  as a prepared-contract publication rather than asking consumers to infer it.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
