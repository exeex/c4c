# LIR Next Function-Body Parameter Authority Handoff

Status: Closed (capability complete)
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.38
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish one next valid function-body parameter-use semantic row as a native
structured LIR authority contract, so 734 can later receive exactly that row
without presentation-derived recovery.

## Closure Record

Disposition: capability complete for this bounded producer/schema/verifier
handoff. The accepted implementation is `02ef01e94`.

The selected and only authorized receiver row is an unchanged
current-function DirectScalar integer parameter used directly as
`LirCmpOp.lhs` by `StmtEmitter::to_bool_operand`. Its native authority tuple
is `LirCurrentFunctionBodyParameterDefinition.value`, current
`LirFunction.link_name_id` owner, definition index, definition integer
`LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and a bounded
truthiness-comparison LHS role. The checked consumer relation is exact:
`LirCmpOp.lhs == authority.value`, `LirCmpOp.type_str == authority.type`,
integer predicate `ne`, and an authoritative integer-zero RHS.

The producer definition seam is `init_fn_ctx` in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`; the consumer is
`StmtEmitter::to_bool_operand` in `src/codegen/lir/hir_to_lir/core.cpp`; the
verifier seams are `verify_cmp_op_authority`,
`verify_integer_cmp_operand_authority`, and modeled value-use traversal in
`src/codegen/lir/verify.cpp`. Missing, invalid, duplicate-definition, foreign,
owner/index/type/ABI/role-mismatched, and consumer-incoherent authority reject
transactionally. `LirCondBr.condition` is excluded because it denotes the
comparison result, not the direct parameter use. Nonselected forms remain fail
closed.

Accepted proof: fresh `cmake --build --preset default`; exact focused
`ctest --test-dir build -j --output-on-failure -R
'^frontend_lir_call_type_ref$'` passed 1/1; matching non-regression guard
passed 1/1. No Raw-BIR/importer/receiver code landed in this idea.

Exact consumer return point: reactivate 734 at **Step 7.39 - Receive the one
826-authorized DirectScalar truthiness-comparison-LHS parameter authority
row**. Add only the matching typed Raw-BIR destination, importer dispatch,
reachable verification, and transactional positive/malformed-authority
coverage; do not repeat Steps 1 through 7.38, use `LirCondBr.condition` as
direct authority, or receive another parameter form.

## Reviewer Reject Signals

- Reject presentation-derived identity from parameter spelling, rendered
  operands, signatures, diagnostics, or testcase names.
- Reject generic parameter admission, ABI broadening, Raw-BIR/importer edits,
  a second row, expectation downgrades, or weaker verifier/test contracts.
- Reject an ambiguous, foreign, or consumer-incoherent authority path hidden
  behind a new carrier, or any reopening of accepted 734 parameter rows.
