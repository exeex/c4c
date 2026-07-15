# LIR PHI Floating Unary-Minus `fneg` Authority

Status: Closed
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

## Completion Record

Disposition: capability complete.

- Step 1 bounded the route to the scalar floating `UnaryOp::Minus` / `fneg`
  producer handoff. The intervening ternary PHI-incoming consumer authority
  blocker was separately completed in `6cf72bd9e`; it did not expand this
  source's scope.
- Step 2 is accepted in `8f31e2535` (`Publish floating fneg PHI authority`):
  the scalar floating-minus producer uses `fresh_value(ctx)` so its `fneg`
  result carries the checked current-function `LirValueId`. Nearby positive
  and malformed-authority coverage exercise the unchanged verifier contract.
- Step 3 acceptance evidence: fresh `cmake --build --preset default`; focused
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`;
  and the matching `test_before.log`/`test_after.log` monotonic guard accepted
  with `--allow-non-decreasing-passed` because this CTest scope is one binary
  containing the added coverage cases.

This closure does not clear 806 or 804. 806 remains parked at Step 3,
*Prove the blocker and return to 804*, pending the separately scoped 808
scalar bit-not `xor` route and the required follow-on 100% full baseline.
