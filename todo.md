# Current Packet

Status: Active
Source Idea Path: ideas/open/781_lir_ternary_coerce_arm_result_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove selected-arm authority and failure closure

## Just Finished

- Plan Step 1 routed only the selected ternary `else` arm through
  `emit_rval_operand` and `coerce_operand`, publishing its scalar narrowing
  `LirCastOp.result` as a native current-function value ID before compatibility
  spelling. Plan Step 2 replaced the stale loss-boundary assertion with
  structural cast authority coverage and verifier rejection coverage for
  missing, invalid, duplicate, and foreign selected-arm result IDs; the raw
  ternary PHI carrier/incomings and its raw later-consumer input remain
  explicitly outside this packet.

## Suggested Next

- Dispatch Plan Step 3 to publish the bounded 775 handoff, retaining the raw
  PHI/final-consumer boundary.

## Watchouts

- The selected `else` arm is the existing `i64` to `i32` narrowing coercion;
  the `then` arm, PHI result/incomings, and later consumer remain raw and
  untouched.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
  The focused `frontend_lir_call_type_ref` subset passed. Log:
  `test_after.log`.
