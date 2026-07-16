# Current Packet

Status: Active
Source Idea Path: ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce and verify truthiness-LHS authority

## Just Finished

- Step 1 established the native truthiness contract. A selected integer
  `LirCmpOp` (`ne`, integer type, SSA LHS, immediate zero RHS) whose LHS is a
  current-function `DirectScalar` definition must carry
  `truthiness_lhs_parameter_authority` copied from that definition: exact
  `LirValueId`, function `LinkNameId` owner, parameter index, `LirTypeRef`,
  `DirectScalar` ABI, and `TruthinessComparisonLhs` role. The verifier already
  requires one exact matching definition and rejects missing, foreign,
  owner-incoherent, parameter-incoherent, type-incoherent, ABI, role,
  predicate, RHS, and duplicate-definition variants.
- The missing producer relation is bounded to the legacy string-only
  `StmtEmitter::to_bool`: it emits the selected `icmp ne ..., 0` without a
  native operand, unlike `to_bool_operand`, which constructs the full tuple
  from `native_body_parameter_definitions`. `UnaryOp::Not` reaches that
  string-only seam, so it cannot publish native identity without recovery.

## Suggested Next

- Step 2 only: preserve the selected operand through the unary truthiness
  construction route so it calls `to_bool_operand` rather than the string-only
  `to_bool` producer. Populate no tuple unless the existing native definition
  lookup matches the exact direct-scalar value/type/current owner. Keep pointer,
  floating, computed, generic, and text-only forms outside this contract.
  Extend nearby positive/malformed truthiness coverage for the producer route.

## Watchouts

- Do not use rendered text, signatures, diagnostics, default classification,
  named-case exceptions, filters, unsupported markers, or weaker contracts.
  Return only to 831 Step 2 after accepted focused proof.
- The current focused truthiness test already exercises verifier rejection for
  missing, foreign, owner/parameter/type-incoherent, ABI/role, predicate/RHS,
  and duplicate-definition authority. Step 2 must add producer-path coverage,
  not duplicate a verifier-only fixture.

## Proof

- Evidence predecessor: 831's exact subset retains 13 GCC torture failures at
  the unchanged missing `LirCmpOp.truthiness_lhs_parameter_authority` verifier
  relation. Step 1 narrows this native producer/verifier seam before code.
- Fresh exact torture subset: 13/13 failed. Ten cases retain the expected
  missing truthiness-authority diagnostic; three (`20090113-2`, `comp-goto-1`,
  `pr51323`) now stop at the separately owned accepted 832 aggregate-owner
  guard and are not part of this 833 repair packet.
