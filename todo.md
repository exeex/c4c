# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory The Existing Prepared Control-Flow Contract
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for idea 62. The current shared prepared
contract is built in `src/backend/prealloc/legalize.cpp`: `legalize_module(...)`
interns `FunctionNameId`, records one `PreparedBranchCondition` per
`CondBranch`, lowers phi/join state into `PreparedJoinTransfer`, then runs
`collect_select_materialized_join_transfers(...)`,
`annotate_branch_owned_join_transfers(...)`, and
`publish_short_circuit_continuation_branch_conditions(...)` before publishing
`PreparedControlFlowFunction`. The preserved authoritative facts in
`src/backend/prealloc/prealloc.hpp` are `branch_conditions` keyed by
`BlockLabelId` plus `join_transfers` keyed by semantic ids with
`source_branch_block_label`, `source_true/false_transfer_index`, and
`source_true/false_incoming_label` for branch-owned joins. Current x86 consumer
entry points are routed from `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
through `render_prepared_compare_driven_entry_if_supported(...)`,
`render_prepared_countdown_entry_routes_if_supported(...)`, and the prepared
local-slot guard render paths. Those consumers already read prepared helpers
such as `find_prepared_branch_condition(...)`,
`find_prepared_compare_branch_target_labels(...)`,
`find_authoritative_branch_owned_join_transfer(...)`, and
`find_prepared_short_circuit_join_context(...)`, but they still mix that with
local block-shape recovery instead of a fully authoritative prepared CFG model.

## Suggested Next

Start `plan.md` Step 2 by adding explicit per-block prepared CFG ownership
facts in `src/backend/prealloc/prealloc.hpp` and
`src/backend/prealloc/legalize.cpp` for predecessor/successor and continuation
targets, then migrate the compare-driven entry helpers to consume that shared
model instead of resolving branch/join meaning from local block traversal.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` still falls
  back to `build_prepared_plain_cond_entry_render_plan(...)` and local compare
  matching when a prepared short-circuit or compare-join plan is missing.
- `src/backend/mir/x86/codegen/prepared_param_zero_render.cpp` still depends on
  local direct-target resolution and entry-shape checks around prepared branch
  conditions instead of a shared per-block CFG record.
- `src/backend/mir/x86/codegen/prepared_countdown_render.cpp` still scans BIR
  blocks and join edges directly to rediscover loop/body/exit structure even
  after it has identified a prepared loop-carry join.
- Several x86 paths deliberately `throw` when an authoritative branch-owned
  join exists but the prepared handoff is not rich enough to replace the old
  bootstrap route; that is the gap to close in Step 2.

## Proof

Inventory-only packet. No code change, no build/tests run, and no `test_after.log`
was produced by design.
