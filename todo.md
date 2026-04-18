# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 3 packet work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by tightening the
contract-backed guard-chain continuation path so prepared short-circuit joins
now accept only the prepared join predecessor named in control-flow metadata,
instead of following an arbitrary empty branch chain when control-flow data
exists.

## Suggested Next

Step 3 likely needs supervisor-chosen broader proof next before treating the
shared-control-flow/x86 route as a milestone; after that, the smallest
remaining code packet is probably tightening
`find_select_materialization_join_transfer(...)` so source-branch labels stay
authoritative instead of optional debt.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Shared lowering still publishes both `incomings` and `edge_transfers`; keep
  migrating consumers before considering any producer-side compatibility
  cleanup.
- `PreparedJoinTransfer::source_true_transfer_index` and
  `source_false_transfer_index` are now the authoritative truth-to-edge mapping
  for select-materialization joins; future producers in that family must
  populate them consistently with `edge_transfers`.
- Contract-backed guard-chain continuation now trusts the prepared join
  predecessor label; if a future packet loosens that back into arbitrary empty
  branch-chain traversal, treat it as route drift.
- The guarded short-circuit raw select/phi reconstruction still exists only as
  a no-control-flow fallback path; keep any future edits to that lane isolated
  from prepared-route consumers.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'`.
Proof passed and was recorded in `test_after.log`.
