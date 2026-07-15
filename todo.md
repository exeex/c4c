# Current Packet

Status: Active
Source Idea Path: ideas/open/775_lir_phi_producer_helper_result_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove selected then-arm authority and failure closure

## Just Finished

- Plan Step 1 routed only the selected scalar ternary `then` arm through
  `emit_rval_operand` and `coerce_operand`, so a representation-changing arm
  coercion publishes its native current-function result authority before the
  compatibility spelling is projected. The selected else arm, raw PHI result
  and incomings, and later consumer remain untouched.

## Suggested Next

- Dispatch Plan Step 2's focused structural and malformed-authority coverage
  for the selected then-arm coercion result.

## Watchouts

- The accepted selected else-arm, logical-RHS, and vaarg facts are retained.
  Do not widen to another expression family, generic expression APIs, PHI/751,
  Raw-BIR, or backend work.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
  The focused `frontend_lir_call_type_ref` subset passed. Log:
  `test_after.log`.
