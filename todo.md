# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 3 packet work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by narrowing the
remaining guarded short-circuit fallback so prepared-route cases now fail
closed unless the prepared control-flow contract can provide the plan, instead
of reconstructing select/phi meaning from raw join blocks.

## Suggested Next

Step 3 likely needs a supervisor route check next: the remaining raw
short-circuit reconstruction now only serves the no-control-flow fallback, so
the next bounded packet should verify whether any covered x86 consumer lanes
still depend on non-contract join semantics before choosing more code work.

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
- The guarded short-circuit raw select/phi reconstruction still exists only as a
  no-control-flow fallback path; if a future packet touches it, treat any
  prepared-route re-expansion as route drift.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_phi_materialize|x86_handoff_boundary)$'`.
Proof passed and was recorded in `test_after.log`.
