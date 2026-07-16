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

## Resumption Record

Status: parked by the active DirectScalar proof-boundary switch to
`ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md`.

- Last accepted progress: Step 1 completed the diagnosis of the selected
  manual `make_switch()` fixture/modelled-result seam. No implementation slice
  from this idea has accepted focused proof or an acceptance commit.
- Completed runbook steps: Step 1 only. Step 2 has a pending, unaccepted local
  implementation slice in `tests/frontend/frontend_lir_call_type_ref_test.cpp`:
  `make_switch()` changes `LirBinOp.type_str` from display text `"i32"` to
  `LirTypeRef::integer(32)`, with a positive structured-authority assertion and
  a missing-type-authority rejection. Preserve this local patch; do not claim
  it accepted, discard it, or attach a commit reference.
- Interrupted step: Step 3 — Prove the blocker and return to 820.
- Blocker: after the local fixture slice, the focused
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  no longer stops at `LirSwitch.selector`; the pending Idea 822 lowering route
  carries it to the later DirectScalar abort. Both local patches remain
  unaccepted. The new abort is Idea 820 scope, so it is the active route before
  this idea can prove its own Step 3.
- Exact return action: after Idea 820 completes its Step 3 boundary proof,
  resume this idea at Step 3; retain the pending fixture patch and the pending
  Idea 822 lowering patch, rerun the same focused test, then perform this
  idea's required proof. Do not claim either patch accepted merely because the
  selector abort advanced.
- Proof and commit status: a fresh `cmake --build --preset default` passed for
  the pending local slice. The exact focused CTest is red before and after the
  slice (0/1 failed both times), so regression comparison is non-accepting;
  there is no accepted commit.
