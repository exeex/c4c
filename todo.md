# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed another Step 3 cleanup in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by switching the
materialized joined-compare consumer from an emitter-local join-transfer scan
to the authoritative prepared join record for the actual join block/result,
while enforcing source-branch metadata when shared lowering publishes it.

## Suggested Next

The next small Step 3 packet is inspecting the remaining countdown and guard
lanes in `prepared_module_emit.cpp` for any control-flow consumers that still
rely on emitter-local topology matching instead of prepared branch/join
metadata.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Shared lowering still publishes both `incomings` and `edge_transfers`; keep
  migrating consumers before considering any producer-side compatibility
  cleanup.
- The joined compare lane should now key off the prepared join record for the
  actual join block/result and only tighten to `source_branch_block_label`
  when that producer metadata exists; reintroducing a generic join-transfer
  scan here would be route drift.
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
- The short-circuit continuation lane now assumes prepared control-flow is
  present; if a future caller bypasses semantic preparation entirely, that
  should be treated as a contract violation rather than patched with new raw
  CFG matching here.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'`.
Both passed; the narrow proof was recorded in `test_after.log`.
