# Current Packet

Status: Active
Source Idea Path: ideas/open/823_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify one DirectScalar binary-RHS authority contract
你該做code review了

## Just Finished

- Step 1 traced the next adjacent candidate, a native `DirectScalar`
  `LirBinOp.rhs` use, and selected no row. Its value identity, owner,
  parameter index, type, and ABI exist in
  `LirCurrentFunctionBodyParameterDefinition`, but the first required missing
  fact is a native RHS role/use binding: `LirScalarBinaryParameterRole` has
  only `Lhs`, `LirBinOp` carries only `scalar_lhs_parameter_authority`, and
  the producer/verifier inspect and validate only `lhs`. This is distinct from
  the accepted pointer GEP-base and DirectScalar binary-LHS rows, but it is
  not receiver-ready and remains fail-closed.

## Suggested Next

- Implement only the in-scope Step 2 repair: publish one explicit
  `LirBinOp.rhs` DirectScalar parameter role and independent authority carrier,
  bind it from the existing checked current-function definition facts, and
  verify matching RHS role/value/type coherence with nearby fail-closed tests.
  Do not begin Raw-BIR receiver work.

## Watchouts

- Do not infer a RHS role from `LirBinOp.rhs`, operand spelling, or the LHS
  carrier. The current schema and verifier intentionally have no RHS role.
- Ideas 821 and 822 retain pending, unaccepted implementation work; do not
  modify, discard, or claim acceptance for either slice.
- No generic scalar/parameter admission, Raw-BIR/importer/builder work, or
  presentation-derived recovery is authorized.
- The accepted pointer and DirectScalar binary-LHS rows are historical
  progress; do not reopen or repeat them.

## Proof

- No proof run: this was a read-only producer/schema/verifier trace with no
  code change and no selected row. No `test_after.log` was produced.
