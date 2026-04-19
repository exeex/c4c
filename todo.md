# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build An Authoritative Shared Prepared CFG Model
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2 slice for idea 62. The shared short-circuit
compare-driven entry helpers in
`src/backend/mir/x86/codegen/prepared_param_zero_render.cpp` and
`src/backend/prealloc/prealloc.hpp` now resolve entry-branch targets through
`resolve_prepared_compare_branch_target_labels(...)` instead of trusting
`PreparedBranchCondition` labels in isolation. That closes the remaining
guard-chain recursion gap where authoritative prepared CFG ownership could
still be bypassed by drifted entry labels, and the x86 boundary suite now
covers both select-carried and `EdgeStoreSlot` short-circuit routes rejecting
that drift.

## Suggested Next

Continue `plan.md` Step 2 by moving the remaining compare-driven local-slot
guard-chain helpers onto the same authoritative prepared CFG ownership model,
especially `src/backend/mir/x86/codegen/prepared_countdown_render.cpp`, which
still rediscovers loop/body/exit structure from local BIR block shape after it
has already identified a prepared loop-carry join.

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
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` still falls back
  to `build_prepared_plain_cond_entry_render_plan(...)` and local compare
  matching when a prepared short-circuit or compare-join plan is missing.
- `src/backend/mir/x86/codegen/prepared_countdown_render.cpp` still scans BIR
  blocks and join edges directly to rediscover loop/body/exit structure even
  after it has identified a prepared loop-carry join.
- Several x86 paths deliberately `throw` when an authoritative branch-owned
  join exists but the prepared handoff is not rich enough to replace the old
  bootstrap route; that is the gap to close in Step 2.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' | tee test_after.log`. The build completed
cleanly. The backend subset returned to the expected baseline shape: the
updated `backend_x86_handoff_boundary` suite passed, and `test_after.log`
matches `test_before.log` with only the same four pre-existing backend-route
failures already tracked for this runbook:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
