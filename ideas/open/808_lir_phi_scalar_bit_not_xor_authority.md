# LIR PHI Scalar Bit-Not `xor` Authority

Status: Open
Type: bounded PHI producer-handoff successor
Blocked Parent: `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`

## Goal

Restore checked current-function `LirValueId` authority for the scalar
bit-not `xor` result that reaches a PHI incoming, without broadening the
repair to other unary operators or changing PHI semantics.

## Why This Exists

806 Step 1 traced `pr68376-2.c` (`~x` in ternary arms) to scalar
`UnaryOp::BitNot` lowering in `src/codegen/lir/hir_to_lir/expr/misc.cpp`
(lines 134–161). Its `xor` result is created with `fresh_tmp(ctx)`, so the
returned SSA display operand lacks the native value ID required by the
existing PHI incoming verifier. This is distinct from 804's accepted
scalar-integer-minus `sub` seam, 806's selected postfix old-value handoff,
and floating unary-minus `fneg`.

## In Scope

- Trace and repair only the scalar `UnaryOp::BitNot` / `xor` result-authority
  handoff evidenced by `pr68376-2.c`.
- Preserve the existing PHI incoming verifier contract while publishing a
  checked current-function ID for valid scalar bit-not results.
- Add nearby scalar-bit-not positive and malformed-authority coverage, then
  obtain focused proof for this producer family.

## Out Of Scope

- Postfix increment old-value work owned by active 806 Step 2.
- Floating unary-minus `fneg`, scalar-integer-minus `sub`, other unary
  operators, generic expression provenance, or a residual unary sweep.
- CFG/PHI schema, verifier, predecessor, edge, ternary-consumer, Raw-BIR,
  text/display identity recovery, and testcase-specific behavior.

## Acceptance Criteria

- The scalar bit-not `xor` producer result entering a PHI incoming carries a
  valid checked current-function `LirValueId` under the existing contract.
- Missing, unknown, foreign, and stale authority remains rejected.
- Nearby same-family positive and malformed-authority coverage passes, along
  with focused proof accepted by the supervisor.

## Reviewer Reject Signals

- Reject a PHI-side, ternary-side, or generic unary conversion that claims to
  repair this one scalar bit-not `xor` handoff.
- Reject reopening 804's scalar-integer-minus `sub` seam or absorbing 806's
  postfix old-value route or the floating unary-minus `fneg` route.
- Reject rendered `%t` recovery, testcase-name branching, expectation
  downgrades, or a named-test-only pass without nearby malformed coverage.
- Reject weakening PHI verification or accepting missing, foreign, stale, or
  unknown native authority merely to make `pr68376-2.c` pass.
