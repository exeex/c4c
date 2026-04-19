# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build An Authoritative Shared Prepared CFG Model
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2 slice for idea 62. The dedicated local-slot
arithmetic guard renderers in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` now resolve their
true/false branch destinations through a shared
`resolve_prepared_compare_branch_target_labels(...)` helper in
`src/backend/prealloc/prealloc.hpp`, so these compare-driven guard paths consume
the authoritative prepared CFG targets when available instead of trusting
`PreparedBranchCondition` labels in isolation. If the prepared branch condition
and prepared per-block CFG facts drift, the x86 handoff now rejects that
contract instead of silently rendering from whichever labels happen to be
present locally.

## Suggested Next

Continue `plan.md` Step 2 by moving the remaining compare-driven local-slot
guard-chain helpers onto the same authoritative prepared CFG target resolution,
especially the recursive branch/continuation path in
`render_prepared_local_slot_guard_chain_if_supported(...)` that can still fall
back to local BIR-shape discovery after the prepared handoff.

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
boundary handoff tests passed, and `test_after.log` shows only the same four
pre-existing backend-route failures already tracked for this runbook:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
