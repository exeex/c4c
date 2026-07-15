# LIR Next Body-Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: accepted 734 Step 7.34 direct-pointer receiver receipt
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish exactly one further valid function-body parameter-use
authority row that a later 734 Raw-BIR receiver packet can consume without
presentation-derived recovery.

## Why This Exists

734 has accepted the direct non-expanded pointer `LirGepOp.ptr` parameter row
from closed 817, but its completion matrix still contains other valid
function-body parameter forms. Existing open 795's accepted parameter-index
handoff returns to its separate 810 baseline route and does not authorize a
734 receiver; it must not be repurposed.

## In Scope

- Trace native production, schema, and verifier authority for one candidate
  body-parameter use row; select it only if its value identity, parameter
  position, type, owner, and ABI classification are structurally available.
- Publish and verify an exact one-row handoff, including the permitted fields,
  rejected malformed/foreign/incoherent forms, and focused producer proof.
- Preserve all nonselected body-parameter forms as classified or fail-closed.

## Out Of Scope

- Raw-BIR destinations, importer/dispatcher or receiver tests;
- reopening closed 817's direct-pointer GEP-base row or existing 795's
  parameter-index route;
- text/name/signature/diagnostic recovery; broad ABI conversion; and every
  memory/VA, aggregate/vector, module/type/global/metadata, instruction,
  terminator, or inline-assembly family.

## Acceptance Criteria

- Exactly one selected body-parameter use row has an explicit native,
  current-function structured authority contract and focused positive/negative
  producer proof.
- The handoff names only fields a later 734 receiver may consume and specifies
  all rejected forms transactionally.
- 734 has a durable return point at Step 7.35; no Raw-BIR receipt is claimed
  by this producer idea.

## Reviewer Reject Signals

- Reject parameter spelling, rendered signatures, raw operands, diagnostics,
  LLVM text, or testcase identity as semantic authority.
- Reject treating declaration publication or closed 817/795 results as
  authority for a different body-use form.
- Reject broad ABI work, receiver/importer edits, expectation downgrades, or a
  combined multi-form parameter sweep claimed as one handoff.
- Reject retaining the selected form's old text-only failure behind renamed
  helpers or classification-only changes claimed as producer capability.
