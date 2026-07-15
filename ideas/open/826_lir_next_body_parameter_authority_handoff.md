# LIR Next Function-Body Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.38
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish one next valid function-body parameter-use semantic row as
a native structured LIR authority contract, so 734 can later receive exactly
that row without presentation-derived recovery.

## Why This Exists

734 accepted only DirectPointer plus DirectScalar parameter uses in the direct
GEP, binary-LHS, binary-RHS, exact return-value, and direct switch-selector
roles. Its no-omission source contract still includes other function-body
parameter forms, but no additional receiver-ready row is evidenced. The
producer/schema/verifier first owner must identify one native row before a
receiver can proceed.

## In Scope

- Trace the next candidate function-body parameter-use route and select one
  semantic row only when its value identity, current-function ownership,
  parameter index, type, ABI classification, role, and exact consuming operand
  relation have native structured representation.
- Publish and verifier-check only that selected row, including malformed,
  missing, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent rejection as applicable.
- Add focused same-feature producer positive/negative coverage and write an
  exact one-row receiver handoff to 734.

## Out Of Scope

- Raw-BIR containers, importer or receiver/verifier edits, broad parameter
  admission, declaration facts treated as body-use authority, ABI conversion,
  or any second parameter row.
- Reopening accepted DirectPointer, DirectScalar GEP, binary-LHS, binary-RHS,
  ReturnValue, or switch-selector rows; memory/VA, aggregate/vector,
  module/type/global/metadata, other instruction/terminator, inline-assembly,
  target lowering, or text parsing.

## Acceptance Criteria

- Exactly one selected body-parameter semantic row has a native checked
  structured contract and focused nearby positive/negative proof.
- The handoff identifies its exact tuple, consumer relation, failure cases,
  and the one bounded 734 receiver return action; all nonselected forms stay
  fail closed.
- No Raw-BIR/importer code changes, presentation-derived authority, or
  expectation weakening is used to claim the handoff.

## Step 1 Selection Record

The sole selected row is an unchanged current-function DirectScalar integer
parameter used directly as `LirCmpOp.lhs` by
`StmtEmitter::to_bool_operand`. Its native authority tuple is
`LirCurrentFunctionBodyParameterDefinition.value`, the current
`LirFunction.link_name_id` owner, the definition index, the definition integer
`LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and a bounded
truthiness-comparison LHS role.

The required consumer relation is exact: `LirCmpOp.lhs == authority.value`,
`LirCmpOp.type_str == authority.type`, integer predicate `ne`, and an
authoritative integer-zero RHS. The native definition seam is
`init_fn_ctx` in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`; the consuming
producer is `StmtEmitter::to_bool_operand` in
`src/codegen/lir/hir_to_lir/core.cpp`; verification runs through
`verify_cmp_op_authority`, `verify_integer_cmp_operand_authority`, and the
modeled value-use traversal in `src/codegen/lir/verify.cpp`.

`LirCondBr.condition` is not selected: it denotes the comparison result, not
the direct parameter use. Every other comparison and parameter form remains
fail closed until separately selected.

## Reviewer Reject Signals

- Reject selection from rendered parameter names, types, signatures, operand
  spelling, printer output, diagnostics, or testcase shape rather than native
  authority.
- Reject generic parameter admission, broad ABI conversion, multiple rows,
  receiver edits, expectation downgrades, or verifier weakening claimed as
  this one-row handoff.
- Reject retaining a missing, ambiguous, foreign, or consumer-incoherent
  authority path behind a renamed carrier or leaving selected malformed input
  accepted.
- Reject reopening the accepted 734 parameter receiver rows or expansion into
  non-parameter semantic families.
