# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build An Authoritative Shared Prepared CFG Model
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed the first `plan.md` Step 2 slice for idea 62. `PreparedControlFlowFunction`
now publishes semantic-id keyed per-block terminator facts through
`PreparedControlFlowBlock`, populated in `src/backend/prealloc/legalize.cpp`
alongside the existing `branch_conditions` and `join_transfers`. The
compare-driven plain-conditional entry path in
`src/backend/mir/x86/codegen/prepared_param_zero_render.cpp` now consumes those
shared per-block target labels before resolving branch destinations, but only
when they agree with the authoritative `PreparedBranchCondition` labels so the
existing canonical handoff rejection still fires on drifted prepared
contracts. This keeps the new CFG ownership fact in shared prepare without
weakening the short-circuit or local-guard contract checks.

## Suggested Next

Continue `plan.md` Step 2 by moving the next compare-driven consumers onto the
same shared per-block CFG facts, starting with the remaining local-slot guard
and compare-join helpers that still rediscover branch/join routing from BIR
block shape after the prepared handoff.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- Keep the new `PreparedControlFlowBlock` targets contract-consistent with
  `PreparedBranchCondition`; mismatches should still fail the canonical
  prepared-module handoff instead of silently preferring whichever record
  happens to look usable locally.
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` still falls
  back to `build_prepared_plain_cond_entry_render_plan(...)` and local compare
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
