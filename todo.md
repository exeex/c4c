# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 3 packet work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and
`src/backend/prealloc/prealloc.hpp` by adding a prepared
select-materialization lookup helper and moving the materialized-compare join
x86 consumer off ad hoc `join_transfers` scanning onto that lookup contract.

## Suggested Next

Continue Step 3 with a bounded audit packet that finds any remaining x86
prepared-control-flow consumers still selecting joins inline, then migrate the
next smallest lane onto lookup helpers without starting producer-side
`incomings` cleanup yet.

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
- The guarded short-circuit lane still performs its own select-materialization
  transfer scan; if that lane is touched next, prefer reusing the new helper
  instead of growing another local matcher.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_phi_materialize|x86_handoff_boundary)$'`.
Proof passed and was recorded in `test_after.log`.
