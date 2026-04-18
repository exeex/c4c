# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Repaired Step 3's ordinary joined compare-against-zero branch consumer in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` so it accepts the
authoritative prepared join-transfer contract for phi-backed joins instead of
requiring a select-materialization-specific carrier. The x86 lane now keys off
prepared edge transfers and the prepared join result name, which restores the
joined branch route without reopening CFG-shape fallback.

## Suggested Next

Step 3 still has a remaining raw short-circuit select/phi reconstruction lane
in `prepared_module_emit.cpp`. The next small packet is isolating or deleting
that no-control-flow fallback so prepared-route consumers stay lookup-driven
across both ordinary joins and short-circuit continuation.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Shared lowering still publishes both `incomings` and `edge_transfers`; keep
  migrating consumers before considering any producer-side compatibility
  cleanup.
- Select-materialization consumers should now treat missing
  `source_branch_block_label` as malformed prepared metadata, not as a reason
  to match by surrounding CFG shape.
- `PreparedJoinTransfer::source_true_transfer_index` and
  `source_false_transfer_index` are now the authoritative truth-to-edge mapping
  for select-materialization joins; future producers in that family must
  populate them consistently with `edge_transfers`.
- Contract-backed guard-chain continuation now trusts the prepared join
  predecessor label; if a future packet loosens that back into arbitrary empty
  branch-chain traversal, treat it as route drift.
- Ordinary joined compare-against-zero branches may lower through phi-backed
  prepared joins as well as select-materialization joins; x86 consumers in
  that lane must use prepared join transfers as the authority, not demand a
  select carrier.
- The guarded short-circuit raw select/phi reconstruction still exists only as
  a no-control-flow fallback path; keep any future edits to that lane isolated
  from prepared-route consumers.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'`.
Both passed; the narrow proof was recorded in `test_after.log`.
