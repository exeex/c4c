# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build An Authoritative Shared Prepared CFG Model
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2 slice for idea 62. The local-slot guard
consumers in `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` now
treat an existing prepared control-flow block for the guard entry as
authoritative ownership for the prepared branch-condition record instead of
reopening the raw guard matcher when that prepared branch record disappears.
The x86 handoff boundary guard suites now prove both the i32 and i16 local-slot
guard routes reject missing prepared branch records with the canonical
prepared-module handoff failure instead of silently falling back to raw
terminator carriers.

## Suggested Next

Continue `plan.md` Step 2 by pushing the remaining compare-driven local-slot
fallbacks in `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` onto
the same authoritative prepared CFG route, especially the short-circuit or
plain conditional helper paths that can still reopen raw trailing-compare or
branch-carrier recovery when prepared control-flow block facts already exist.

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
- `prepared_countdown_render.cpp` now consumes prepared control-flow block
  successor labels even when a local countdown segment does not have a
  dedicated prepared branch-condition record; keep that route strict about
  rejecting mismatched authoritative metadata instead of silently falling back
  to raw terminator drift.

## Proof

Ran the delegated proof command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. `backend_x86_handoff_boundary`
passed with the new missing-branch-record coverage. The backend subset stayed
at the expected baseline with the same four pre-existing route failures
already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
