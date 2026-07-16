# LIR Next Body-Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.42
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Trace, publish, verify, and hand off exactly one next valid function-body
parameter-use authority row for a later typed Raw-BIR receiver.

## Why This Exists

734's accepted Step 7.42 received only closed 853's DirectPointer
pointer-truthiness parameter row. Its no-omission source contract still
includes other function-body parameter forms, but no additional row is
currently authorized for Raw-BIR receipt. The producer/schema/verifier first
owner must identify one native row before a receiver can proceed.

## In Scope

- Trace the next valid, currently produced function-body parameter use after
  the accepted DirectPointer pointer-truthiness row and select one bounded
  semantic consumer relation.
- Publish only the selected row's current-function value identity, owner,
  parameter index, type, ABI, explicit role, and required consumer coherence
  facts in native LIR.
- Verify missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority fails closed.
- Add focused same-feature positive and malformed-authority proof, then
  record an exact one-row receiver handoff to 734.

## Out Of Scope

- Any Raw-BIR container, builder, importer, or Raw-BIR verifier change.
- Reopening accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, truthiness-comparison-LHS,
  fixed-direct-call argument-0, fixed-direct-call argument-1, or
  pointer-truthiness rows.
- Generic parameter admission, ABI conversion, declaration-only authority,
  presentation-derived identity, or a multi-row parameter sweep.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, and downstream receiver work.

## Acceptance Criteria

- Exactly one selected function-body parameter-use row has a structured,
  verifier-checked current-function contract and focused positive/negative
  producer proof.
- All nonselected parameter forms remain fail closed; no semantic fact is
  reconstructed from text, names, signatures, diagnostics, rendered operands,
  compatibility mirrors, or testcase shape.
- The completion record names the exact receiver tuple, consumer relation,
  accepted proof, and 734 return action without claiming Raw-BIR receipt.

## Reviewer Reject Signals

- Reject a testcase-shaped selection, named-case-only producer shortcut, or
  any generic parameter authority claimed as this one-row capability.
- Reject expectation downgrades, weaker verifier/test contracts, or
  classification-only edits claimed as authority publication.
- Reject Raw-BIR/importer edits, broad ABI rewrites, or changes outside the
  selected producer/schema/verifier seam.
- Reject text-, name-, signature-, printer-, diagnostic-, compatibility-, or
  rendered-operand-derived identity, type, role, or consumer coherence.
- Reject retaining the exact missing authority behind a renamed carrier, or
  accepting a row without malformed/foreign/duplicate rejection coverage.
