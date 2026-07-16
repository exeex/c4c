# LIR Next Body-Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Trace, publish, verify, and hand off exactly one next valid function-body
parameter-use authority row for a later typed Raw-BIR receiver.

## Why This Exists

734's accepted Step 7.40 received only closed 827's DirectScalar
fixed-direct-call argument-0 row. Its next receiver row needs independently
structured producer authority; 734 cannot infer it from display text,
signatures, rendered operands, diagnostics, or compatibility fields.

## In Scope

- Trace the next valid, currently produced function-body parameter use after
  the accepted 827 row and select one bounded semantic consumer relation.
- Publish only the selected row's current-function value identity, owner,
  parameter index, type, ABI, explicit role, and required consumer coherence
  facts in native LIR.
- Verify missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority fails closed.
- Add focused same-feature positive and malformed-authority proof, then record
  an exact one-row receiver handoff to 734.

## Out Of Scope

- Any Raw-BIR container, builder, importer, or Raw-BIR verifier change.
- Reopening accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, truthiness-comparison-LHS, or
  fixed-direct-call-argument-0 rows.
- Generic parameter admission, ABI conversion, declaration-only authority,
  presentation-derived identity, or a multi-row parameter sweep.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, and downstream receiver work.

## Acceptance Criteria

- Exactly one selected function-body parameter-use row has a structured,
  verifier-checked current-function contract and focused positive/negative
  producer proof.
- All nonselected parameter forms remain fail closed; no semantic fact is
  reconstructed from text, names, signatures, diagnostics, or rendered
  operands.
- The completion record names the exact receiver tuple, consumer relation,
  accepted proof, and 734 return action without claiming Raw-BIR receipt.

## Reviewer Reject Signals

- Reject a testcase-shaped selection, named-case-only producer shortcut, or
  any generic parameter authority claimed as this one-row capability.
- Reject expectation downgrades, weaker verifier/test contracts, or
  classification-only edits claimed as authority publication.
- Reject Raw-BIR/importer edits, broad ABI rewrites, or changes outside the
  selected producer/schema/verifier seam.
- Reject text-, name-, signature-, printer-, or diagnostic-derived identity,
  type, role, or consumer coherence.
- Reject retaining the exact missing authority behind a renamed carrier, or
  accepting a row without malformed/foreign/duplicate rejection coverage.

## Resumption Record: structured direct-call argument identity prerequisite

- Last accepted progress: Step 1 was traced and recorded in `78b17b3d3`
  (`[todo_only] Trace next call parameter authority`). The initially selected
  relation was the second current-function `DirectScalar` parameter at
  `LirCallOp.structured_args[1]` of a direct, non-variadic, specified call.
  This is trace evidence only; it did **not** publish an authority row.
- Interrupted step and disposition: Step 2, *Publish and verify the selected
  authority*, is blocked and this idea remains open. Closure is rejected:
  its acceptance criterion requires one structured, verifier-checked producer
  contract and focused positive/negative proof, neither of which exists for
  argument 1.
- Blocker outside this idea's scope: valid two-parameter direct calls do have
  native parameter definitions and callee signature entries, but
  `LirCallOp.arg_type_refs` is empty and
  `structured_args[1].operand` is non-SSA/text-only. Thus there is no existing
  authoritative value/type relation that 829 may publish. The attempted
  `frontend_lir_call_type_ref` positive fixture fails; it is preserved in
  `test_after.log` and is not acceptance evidence. Text, signature, rendered
  operand, diagnostic, and parser-shaped recovery are forbidden.
- Separate-blocker route: `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`
  owns establishing a bounded native LIR call-argument structured identity and
  type relation at the producing seam, with native verification and focused
  proof. It must not publish 829's body-parameter authority or implement any
  Raw-BIR receiver work.
- Exact return point: after 830 accepts one valid structured direct-call
  argument-1 identity/type relation, reactivate 829 at Step 2. Revalidate that
  the relation can carry the existing current-function parameter definition's
  value/owner/index/type/ABI/role tuple without recovery; only then publish
  that one 829 authority row and add its malformed-authority coverage.
- Preserved evidence: no 829 code commit exists. `test_before.log` remains the
  focused 1/1 before baseline; do not replace either canonical regression log
  during this lifecycle switch.
