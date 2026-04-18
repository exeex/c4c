# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed Step 3 packet work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by moving the covered
return-select and countdown loop consumers onto prepared `edge_transfers`
lookups instead of relying on legacy join `incomings`.

## Suggested Next

Continue Step 3 with a bounded packet that removes the remaining x86
short-circuit helper dependence on legacy `PreparedJoinTransfer::incomings`,
likely by extending the prepared contract so truth-to-edge ownership is explicit
without inferring it from rewritten incoming labels.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Shared lowering still publishes both `incomings` and `edge_transfers`; follow-up
  packets should keep migrating consumers before considering any producer-side
  compatibility cleanup.
- The short-circuit control-flow ownership check currently mutates join incoming
  labels independently of `edge_transfers`, so that consumer site needs a
  stronger prepared truth-to-edge contract before it can migrate cleanly.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_phi_materialize|x86_handoff_boundary)$'`.
Proof passed and was recorded in `test_after.log`.
