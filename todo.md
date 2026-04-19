# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build An Authoritative Shared Prepared CFG Model
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2 slice for idea 62. The compare-driven local
countdown route in
`src/backend/mir/x86/codegen/prepared_countdown_render.cpp` now consumes
authoritative prepared guard-branch and continuation-loop join ownership when
that metadata is present and contract-consistent, instead of rejecting any
prepared ownership on those blocks outright. The x86 boundary suite now proves
that the countdown fallback keeps following prepared guard targets over raw
guard-terminator drift and keeps the continuation init value authoritative over
a drifted local carrier store.

## Suggested Next

Continue `plan.md` Step 2 by moving the remaining compare-driven local-slot
fallbacks in `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` onto
the same authoritative prepared CFG ownership model, especially the plain
compare/guard entry path that still reconstructs branch meaning from local BIR
shape when prepared branch metadata is already available.

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
- The countdown fallback now accepts authoritative guard/join ownership only
  when the prepared branch/join records still agree with the supported local
  slot route; any mismatch should keep failing through the canonical prepared
  handoff message instead of reopening the bootstrap path.
- The loop-carried countdown fast path still depends on raw block-shape checks
  after it has identified a prepared loop-carry join; that remains a distinct
  Step 2 gap if we keep pushing `prepared_countdown_render.cpp`.

## Proof

Ran the delegated proof command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`.
The build completed cleanly, `backend_x86_handoff_boundary` passed with the new
countdown ownership coverage, and the backend subset stayed at the expected
baseline with only the same four pre-existing route failures already tracked
for this runbook:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
