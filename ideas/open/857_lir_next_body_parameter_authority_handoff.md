# LIR Next Body-Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish one exact structured LIR authority handoff for the next valid
function-body parameter-use row after 734's accepted Step 7.45 DirectScalar
binary-`fmul` RHS receipt.

## Why This Exists

Idea 734 cannot continue receiving body-parameter rows unless LIR first
publishes native identity, owner, type, ABI, role, and consumer coherence for a
specific current-function parameter use. Accepted Step 7.45 completed only the
closed-856 RHS `fmul` row; the next row must be selected and proved at the
producer/schema/verifier layer before Raw-BIR receipt is authorized.

## In Scope

- Trace the next valid function-body parameter-use row after the accepted
  DirectPointer and DirectScalar body-parameter receipts through Step 7.45.
- Publish only the native structured authority required for that one selected
  row, including parameter source identity, current-function ownership,
  parameter index, exact type, native body-parameter ABI, role, and consumer
  relation coherence.
- Add focused verifier coverage for present, missing/omitted, invalid,
  duplicate, foreign-owner, wrong-index, wrong-type, wrong-ABI, wrong-role, and
  selected-consumer incoherence forms as applicable to the selected row.
- Record an exact one-row handoff back to 734 only after focused proof accepts
  the producer/schema/verifier contract.

## Out Of Scope

- Raw-BIR containers, builders, importers, verifier receipt, or receiver tests.
- Reopening accepted DirectPointer or DirectScalar body-parameter receipts
  through 734 Step 7.45.
- Selecting from display text, names, rendered operands, signatures,
  compatibility mirrors, `monostate`, or testcase shape.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, or more than one body-parameter row.
- Weakening unsupported diagnostics, expectation contracts, or fail-closed
  malformed-authority behavior.

## Acceptance Criteria

- Exactly one next valid function-body parameter-use row is selected and named
  with its native producer surface and consumer relation.
- LIR publishes all structured authority needed for that row without Raw-BIR
  receiver changes.
- The LIR verifier rejects malformed, ambiguous, foreign, role/type/ABI
  incoherent, and selected-consumer-incoherent authority before downstream use.
- Focused same-feature positive and malformed-authority tests pass with
  supervisor-accepted proof and `git diff --check`.
- The handoff back to 734 states the exact tuple, proof, and receiver
  exclusions; all nonselected rows remain fail closed.

## Reviewer Reject Signals

- Reject Raw-BIR/importer edits or claiming a receiver receipt under this
  producer handoff idea.
- Reject repeating or relabeling accepted DirectPointer or DirectScalar
  body-parameter rows through Step 7.45 as new progress.
- Reject deriving authority from text, names, rendered operands, signatures,
  compatibility mirrors, `monostate`, or testcase-shaped selectors.
- Reject publishing a generic body-parameter authority bucket instead of one
  selected row with explicit owner, index, type, ABI, role, and consumer
  coherence.
- Reject weakening verifier diagnostics, unsupported contracts, malformed
  rejection, or fail-closed behavior for nonselected rows.
- Reject absorbing memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or any second row.
