# LIR Ternary PHI-Incoming Operand-Authority Handoff

Status: Open
Type: bounded blocker for 807 Step 2
Blocked Parent: `ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md`

## Goal

Preserve a returned `LirOperand`'s native checked current-function
`LirValueId` when ternary lowering constructs its matching PHI incoming,
without changing producer creation or PHI verification semantics.

## Why This Exists

807 Step 2 demonstrated that `fresh_value(ctx)` gives floating unary-minus
`fneg` native authority, but for `condition ? -input : 0.0` the matching
`%t6` PHI incoming is reconstructed without `value_id`. The loss occurs in
ternary lowering's PHI-incoming construction. This prevents the 807 producer
route from proving that its returned operand reaches the existing verifier
contract intact.

## In Scope

- Trace the ternary lowering path that turns a returned `LirOperand` into a
  PHI incoming and retain/pass its native ID at that construction seam.
- Add nearby ownership-preservation and rejection coverage only if needed to
  demonstrate that the returned operand's authority survives the handoff.
- Preserve the existing current-function PHI verifier contract.

## Out Of Scope

- Floating `fneg` producer creation, other unary families, postfix,
  scalar bit-not, or any producer-side authority repair.
- Other PHI semantics, schema, verifier weakening, predecessor/edge work,
  Raw-BIR, text/display recovery, or testcase-specific behavior.
- The parent 807 focused coverage and acceptance work except as a consumer of
  this blocker after the handoff is repaired.

## Acceptance Criteria

- A ternary PHI incoming retains the native checked `LirValueId` of the
  returned `LirOperand` used to construct it.
- Existing rejection of missing, unknown, foreign, and stale authority remains
  intact; no verifier contract is weakened.
- Nearby preservation/rejection coverage and focused proof are accepted by the
  supervisor, with the exact parent return point recorded.

## Reviewer Reject Signals

- Reject moving this repair back into `fneg` creation, another unary producer,
  or a generic provenance conversion while claiming this PHI-incoming handoff
  is fixed.
- Reject schema or verifier relaxation, rendered `%t` recovery,
  testcase-name branching, or expectation downgrades.
- Reject accepting missing, unknown, foreign, or stale authority merely to
  pass a positive ternary case.
- Reject broad ternary/CFG/PHI rewrites outside the returned-operand to
  incoming-construction seam, or coverage that proves only one named fixture.
