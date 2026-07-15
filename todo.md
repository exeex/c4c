# Current Packet

Status: Active
Source Idea Path: ideas/open/824_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected structured authority

## Just Finished

- Step 2 published the optional `LirRet` ReturnValue carrier for exactly an
  unchanged integer `DirectScalar` current-function parameter.  Lowering
  copies only the native definition's value, index, type, owner, ABI, and
  `ReturnValue` role; verification requires an exact unique definition and
  rejects missing, malformed, foreign, duplicate, ABI/type/owner/role, and
  return-operand mismatches.  Nonselected return forms do not synthesize this
  carrier.

## Suggested Next

- Step 3: record the exact handoff contract and proof for 734; do not widen
  this completed producer/verifier row into a receiver or Raw-BIR change.

## Watchouts

- The verifier's missing-carrier rule is intentionally gated on integer
  return type, SSA operand, matching signature-return mirror, and matching
  native definition, so it does not generalize parameter admission.
- Do not edit Raw-BIR/importer code or reopen accepted pointer/DirectScalar
  LHS/RHS rows.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure
  -R '^backend_'` passed: 6/6 backend tests.  Log: `test_after.log`.
