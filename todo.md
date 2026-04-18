# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 3 packet work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`,
`src/backend/prealloc/prealloc.hpp`, and
`src/backend/prealloc/legalize.cpp` by adding explicit select-materialization
truth-to-edge indices to the prepared control-flow contract and moving the
remaining guarded short-circuit x86 consumer off legacy
`PreparedJoinTransfer::incomings`.

## Suggested Next

Continue Step 3 with a bounded audit packet that finds any remaining x86
prepared-control-flow consumers still scanning `join_transfers` ad hoc instead
of using the strengthened lookup contract, then migrate the next smallest lane
without starting producer-side `incomings` cleanup yet.

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

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_phi_materialize|x86_handoff_boundary)$'`.
Proof passed and was recorded in `test_after.log`.
