# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 3 packet work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and
`src/backend/prealloc/prealloc.hpp` by reusing the prepared
select-materialization lookup helper in the guarded short-circuit x86
consumer, eliminating the last inline `join_transfers` scan in
`prepared_module_emit.cpp`.

## Suggested Next

Continue Step 3 with a bounded audit packet that verifies no remaining x86
prepared-control-flow consumers still depend on inline select-materialization
join scans or legacy select `incomings` semantics, then hand route selection
back to the supervisor if the audit is clean.

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
- The guarded short-circuit and materialized-compare lanes now both rely on the
  shared select-materialization lookup helper, so any remaining Step 3
  follow-up should target residual consumer dependencies rather than adding new
  lane-local transfer matchers.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_phi_materialize|x86_handoff_boundary)$'`.
Proof passed and was recorded in `test_after.log`.
