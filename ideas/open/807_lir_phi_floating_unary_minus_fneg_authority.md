# LIR PHI Floating Unary-Minus `fneg` Authority

Status: Open
Type: bounded PHI producer-handoff successor
Blocked Parent: `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`

## Goal

Restore checked current-function `LirValueId` authority for the floating
unary-minus `fneg` result that reaches a PHI incoming, without broadening the
repair to other unary operators or changing PHI semantics.

## Why This Exists

806 Step 1 traced `ieee/pr50310.c` (`condition ? -1.0 : 0.0`) to the floating
`UnaryOp::Minus` lowering branch in `src/codegen/lir/hir_to_lir/expr/misc.cpp`
(lines 88–91). That branch creates its `fneg` result with `fresh_tmp(ctx)`,
leaving the returned SSA display operand without the native value ID required
by the existing PHI incoming verifier. This is distinct from both 804's
accepted scalar-integer-minus `sub` seam and 806's selected postfix old-value
handoff.

## In Scope

- Trace and repair only the floating `UnaryOp::Minus` / `fneg` result-authority
  handoff evidenced by `ieee/pr50310.c`.
- Preserve the existing PHI incoming verifier contract while publishing a
  checked current-function ID for valid floating-minus results.
- Add nearby floating-unary-minus positive and malformed-authority coverage,
  then obtain focused proof for this producer family.

## Out Of Scope

- Postfix increment old-value work owned by active 806 Step 2.
- Scalar bit-not `xor` authority, scalar-integer-minus `sub`, other unary
  operators, generic expression provenance, or a residual unary sweep.
- CFG/PHI schema, verifier, predecessor, edge, ternary-consumer, Raw-BIR,
  text/display identity recovery, and testcase-specific behavior.

## Acceptance Criteria

- The floating `fneg` producer result entering a PHI incoming carries a valid
  checked current-function `LirValueId` under the existing contract.
- Missing, unknown, foreign, and stale authority remains rejected.
- Nearby same-family positive and malformed-authority coverage passes, along
  with focused proof accepted by the supervisor.

## Reviewer Reject Signals

- Reject a PHI-side, ternary-side, or generic unary conversion that claims to
  repair this one floating `fneg` handoff.
- Reject reopening 804's scalar-integer-minus `sub` seam or absorbing 806's
  postfix old-value route or the scalar bit-not `xor` route.
- Reject rendered `%t` recovery, testcase-name branching, expectation
  downgrades, or a named-test-only pass without nearby malformed coverage.
- Reject weakening PHI verification or accepting missing, foreign, stale, or
  unknown native authority merely to make `ieee/pr50310.c` pass.

## Resumption Record: ternary PHI-incoming operand-authority blocker

Last accepted progress: Step 1 established the bounded floating
`UnaryOp::Minus` / `fneg` producer route and selected the nearby focused
coverage direction. No 807 implementation slice is accepted or committed.

Interrupted step: Step 2, `Publish native authority for the `fneg` result`.

Blocking evidence: the scoped, unaccepted change from `fresh_tmp(ctx)` to
`fresh_value(ctx)` in `src/codegen/lir/hir_to_lir/expr/misc.cpp` gives the
`fneg` result a native `LirValueId{1}` for `condition ? -input : 0.0`, but the
matching `%t6` PHI incoming is recreated without `value_id`. The authority is
therefore lost in ternary lowering's PHI-incoming construction, not at the
floating-minus producer. A fresh `cmake --build --preset default` succeeds;
the focused `frontend_lir_call_type_ref` CTest fails only the temporary
positive assertion, and those temporary test edits were removed.

This handoff is out of 807 scope because 807 explicitly excludes ternary/
PHI-consumer changes. It is owned first by
`ideas/open/809_lir_ternary_phi_incoming_operand_authority_handoff.md`.

Return point after blocker: resume 807 Step 2 with the existing scoped
`misc.cpp` hunk retained; add the floating-`fneg` positive and
malformed-authority coverage, then run the focused proof. Do not claim 807
acceptance, create a commit, or clear the 806/804 full-baseline gate until
that route completes.
