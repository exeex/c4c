# Frontend LIR Manual Switch Modelled-Result Authority

Status: Open (active blocker for Idea 820 Step 3)
Type: narrow frontend LIR fixture/modelled-result authority correction
Blocked Parent: `ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md`,
Step 3

## Goal

Correct the manually built frontend LIR `LirSwitch` fixture so its selector
identifies the required current-function integer value definition through the
existing structured modelled-result authority contract.

## Why This Exists

The Step 3 broader probe for Idea 820 runs `frontend_lir_call_type_ref` before
the DirectScalar boundary coverage. That test currently stops at
`LirSwitch.selector: must identify a current-function integer value definition`.
The manual switch fixture and selector verifier branch are unchanged from
`4bcc7c8ff^`; therefore this is an independent frontend LIR test-fixture /
modelled-result-authority defect, not a DirectScalar producer/verifier change.

## In Scope

- Trace the manually built `LirSwitch` selector fixture and the structured
  current-function integer modelled-result authority it is meant to represent.
- Make the smallest fixture/modelled-result authority correction needed for
  that selector to satisfy the existing verifier contract.
- Add nearby positive and malformed/foreign/missing authority tests for the
  selected manual-switch fixture contract.
- Prove `frontend_lir_call_type_ref` reaches and passes its DirectScalar tests,
  then return 820 unchanged at Step 3.

## Out of Scope

- DirectScalar producer/verifier changes, Raw-BIR, importer, builder, or
  generic scalar-parameter receipt.
- Generic switch lowering, switch semantic expansion, unrelated LIR fixtures,
  or a broad rewrite of value-definition authority.
- Testcase-specific verifier exceptions, presentation-text reconstruction, or
  weakening the current-function integer selector requirement.

## Acceptance Criteria

1. The selected manual `LirSwitch` fixture supplies structured
   current-function integer modelled-result authority required by the existing
   selector verifier contract.
2. Missing, foreign, or type-incoherent selector authority remains rejected
   by nearby coverage; no default or textual selector classification is added.
3. A fresh build and focused `^frontend_lir_call_type_ref$` proof pass, and
   the test executes beyond the previous switch-selector abort.
4. The return record directs only Idea 820 to resume unchanged at Step 3 for
   its own fresh broader checkpoint and selected 734 handoff.

## Reviewer Reject Signals

- Reject any DirectScalar, Raw-BIR, importer, builder, generic scalar receipt,
  or unrelated switch-lowering change presented as this fixture correction.
- Reject a named-test exception, expectation downgrade, or test reorder that
  hides `LirSwitch.selector` rather than supplying structured authority.
- Reject deriving selector identity/type from rendered names, diagnostics,
  operands, or other presentation text.
- Reject weakening acceptance of missing, foreign, or type-incoherent
  selector authority, including a default integer classification.
- Reject a broad value-authority or generic switch redesign when the selected
  manual fixture/modelled-result seam can be corrected locally.
