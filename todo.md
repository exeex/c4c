# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Finish Contract-Strict CFG Handoff Surfaces
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2.3 slice for idea 62. The generic
local-slot single-successor branch helper in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` now treats a
missing prepared control-flow block as a hard canonical handoff failure
instead of reopening raw `terminator.target_label` recovery, and
`tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`
now proves that the local-slot passthrough carrier rejects a removed prepared
CFG block record after prepare has already published authoritative control-flow
metadata.

## Suggested Next

Check whether any other `plan.md` Step 2.3 x86 handoff helpers still keep raw
single-successor recovery when a prepared CFG block record is missing. If the
local-slot and countdown carriers were the last contract-strict cases, move
the next packet toward Step 3 consumer migration instead of extending Step 2.3.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- Keep `PreparedBranchCondition` and `PreparedControlFlowBlock` targets
  contract-consistent; mismatches should still fail the canonical
  prepared-module handoff instead of silently preferring whichever record
  happens to look usable locally.
- The dedicated i32/i16 local guard renderers now also treat a missing
  prepared branch record as a contract failure whenever the entry block already
  has an authoritative prepared control-flow block; keep the remaining
  compare-driven helpers equally strict about not reopening raw guard carriers.
- The generic local-slot short-circuit renderer now prefers authoritative
  prepared passthrough branch targets over drifted raw local labels, but keep
  the remaining branch-carrier consumers equally strict about not reopening
  raw successor recovery when a prepared CFG block record goes missing.
- `prepared_local_slot_render.cpp` now rejects a removed passthrough carrier
  block record, so follow-up cleanup should target the remaining strict
  consumer surfaces rather than softening this contract back into raw branch
  fallback.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after adding local-slot
single-successor passthrough coverage that removes the prepared CFG carrier
record and verifies the x86 prepared-module handoff now rejects reopening the
raw branch fallback. `test_after.log` is the proof artifact for this packet.
