# Current Packet

Status: Active
Source Idea Path: ideas/open/775_lir_phi_producer_helper_result_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish selected then-arm coercion result authority

## Just Finished

- Reassessment found the first remaining producer loss: the selected scalar
  ternary `then` arm still follows `emit_rval_id` to string `coerce`, while the
  accepted selected `else` arm already follows `emit_rval_operand` to
  `coerce_operand`.

## Suggested Next

- Implement only Plan Step 1's selected then-arm typed coercion-result
  publication; retain the raw PHI/final-consumer boundary.

## Watchouts

- The accepted selected else-arm, logical-RHS, and vaarg facts are retained.
  Do not widen to another expression family, generic expression APIs, PHI/751,
  Raw-BIR, or backend work.

## Proof

- After implementation, run `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`;
  Step 2 owns focused structural and malformed-authority proof.
