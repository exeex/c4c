# Current Packet

Status: Active
Source Idea Path: ideas/open/755_lir_conditional_branch_condition_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish conditional-branch condition authority

## Just Finished

- Step 1 published `LirCondBr.condition` as the current-function `LirValueId`
  selected from typed boolean comparison production; `cond_name` is now only a
  verifier-checked display mirror. Missing, invalid, foreign, misleading, and
  non-boolean condition authority fail closed before printer/downstream use;
  typed successor validation remains unchanged.

## Suggested Next

- Supervisor: review and accept the Step 1 typed-field handoff before selecting
  a later 734 conditional-branch receiver packet.

## Watchouts

- Do not change Raw-BIR receipt, switch/computed-goto authority, or recover a
  condition from `cond_name`, labels, or printer output.
- The Step 1 condition carrier is `LirCondBr.condition`; later consumers must
  use that ID, never reconstruct it from `cond_name`.

## Proof

- Executor: `cmake --build --preset default` passed; direct
  `build/tests/frontend/frontend_lir_call_type_ref_test` passed positive,
  misleading-display, missing, invalid, foreign, and non-boolean coverage.
- Executor: `ctest --test-dir build -j --output-on-failure -R '^backend_' >
  test_after.log` passed (5/5).
- Supervisor: select and record broader/full acceptance separately.
