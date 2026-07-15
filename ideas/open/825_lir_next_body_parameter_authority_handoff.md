# LIR Next Function-Body Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.37
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish one next valid function-body parameter-use semantic row as
a native structured LIR authority contract, so 734 can later receive exactly
that row without presentation-derived recovery.

## Why This Exists

734 accepted only DirectScalar parameter uses in a direct pointer GEP, binary
LHS, binary RHS, and exact return-value role. Its no-omission source contract
still includes other function-body parameter forms, but no additional row is
currently authorized for Raw-BIR receipt. The producer/schema/verifier first
owner must identify one native row before a receiver can proceed.

## In Scope

- Trace the next candidate function-body parameter-use route and select one
  semantic row only when its value identity, current-function ownership,
  parameter index, type, ABI classification, role, and exact consuming
  operand relation have native structured representation.
- Publish and verifier-check only that selected row, including malformed,
  missing, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent rejection as applicable.
- Add focused same-feature producer positive/negative coverage and write an
  exact one-row receiver handoff for 734.

## Out Of Scope

- Raw-BIR containers, importer or receiver/verifier edits, broad parameter
  admission, declaration facts treated as body-use authority, ABI conversion,
  or any second parameter row.
- Reopening accepted direct-pointer, DirectScalar binary-LHS, binary-RHS, or
  ReturnValue rows; memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, target lowering, or text
  parsing.

## Acceptance Criteria

- Exactly one selected body-parameter semantic row has a native checked
  structured contract and focused nearby positive/negative proof.
- The handoff identifies its exact tuple, consumer relation, failure cases,
  and the one bounded 734 receiver return action; all nonselected forms stay
  fail closed.
- No Raw-BIR/importer code changes, presentation-derived authority, or
  expectation weakening is used to claim the handoff.

## Reviewer Reject Signals

- Reject selection from rendered parameter names, types, signatures, operand
  spelling, printer output, or testcase shape rather than native authority.
- Reject generic parameter admission, broad ABI conversion, multiple rows,
  receiver edits, expectation downgrades, or verifier weakening claimed as
  this one-row handoff.
- Reject retaining a missing/ambiguous/foreign/consumer-incoherent authority
  path behind a renamed carrier or leaving selected malformed input accepted.
- Reject any reopening of the four accepted 734 parameter receiver rows or
  expansion into non-parameter semantic families.
