# Current Packet

Status: Active
Source Idea Path: ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md
Source Plan Path: plan.md
Current Step ID: 2a
Current Step Title: Trace the actual compare construction producer

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
- The `UnaryOp::Not` / string-only `to_bool` hypothesis is disproven for this
  torture family. Temporarily routing it through `to_bool_operand` built and
  preserved the tuple, but the matching in-scope ten-case guard remained
  10/10 failing (nine missing-authority cases plus `pr88714`'s separate 832
  aggregate-owner failure). The temporary code and tests were reverted.

## Suggested Next

- Step 2a is evidence-only: trace representative in-scope direct-scalar cases
  backward from the selected `LirCmpOp` to the actual compare construction
  site. Do not make another producer patch until native evidence shows that
  site receives the exact direct-scalar operand and can carry identity, owner,
  parameter, and type facts without recovery.
- If that discriminating condition is absent, record the failed route and the
  next bounded diagnostic action; do not resurrect the disproven `UnaryOp::Not`
  or string-only `to_bool` packet.

## Watchouts

- Do not use rendered text, signatures, diagnostics, default classification,
  named-case exceptions, filters, unsupported markers, or weaker contracts.
  Return only to 831 Step 2 after accepted focused proof.
- The current focused truthiness test already exercises verifier rejection for
  missing, foreign, owner/parameter/type-incoherent, ABI/role, predicate/RHS,
  and duplicate-definition authority. Step 2 must add producer-path coverage,
  not duplicate a verifier-only fixture.

## Proof

- Evidence predecessor: 831's exact subset retains the truthiness failures at
  the unchanged missing `LirCmpOp.truthiness_lhs_parameter_authority` verifier
  relation. Canonical `test_before.log` and `test_after.log` use the exact
  same matching ten-case command and show no improvement from the reverted
  UnaryOp hypothesis (10/10 failures in both).
