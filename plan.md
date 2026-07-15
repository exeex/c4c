# LIR Anonymous Aggregate Layout Type Facts Runbook

Status: Active
Source Idea: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Resumed from: 806 Step 3 full-baseline return point; preserve 806 Steps 1–2
and resume that route only at its recorded full-baseline proof.

## Purpose

Provide native anonymous aggregate field-layout/type facts needed by the
selected extractvalue row without letting compatibility text become authority.

## Core Rule

Native structured field facts are authority. `LirTypeRef` rendering may mirror
an anonymous aggregate but must not be parsed to create or repair its layout.

## Read First

- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md`
- `ideas/open/802_lir_switch_selector_type_reference_verifier.md` resumption
  record
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- direct-complex aggregate lowering plus nearby focused tests

## Non-Goals

- `LirExtractValueOp` result/use/index/result-type row validation.
- 802 switch selector verifier repair or acceptance of its parked hunk/tests.
- Raw-BIR, other aggregate/vector rows, broad type rewrite, lowering, MIR,
  emission, and all text-derived layout recovery.

## Ordered Steps

### Step 1 - Trace and select anonymous aggregate layout facts (accepted)

Goal: identify the exact anonymous aggregate construction and verification
boundary and select the smallest checked native field-layout carrier.

Completion check: accepted in `827dae5bd3`; one bounded native layout contract
is explicit and no compatibility-text parsing or extractvalue-row work is
selected.

### Step 2 - Repair anonymous layout / structured-call compatibility

Goal: repair the rejected anonymous-layout implementation so native ordered
field facts remain checked without making a direct-complex `LirCallOp`'s
structured callee signature or argument type mirror disagree with its call
arguments.

Actions:

- start from rejected implementation commit `201f229d3` and the preserved
  in-progress Step 2 repair; locate the ownership/type construction mismatch
  rather than weakening verifier contracts;
- repair the native `arg_type_refs` argument-mirror verifier defect exposed by
  `frontend_lir_call_type_ref`: do not treat rendered diagnostic/call text as
  the argument type authority;
- retain checked native field-count/field-type access and malformed-layout
  rejection; leave named structs, arrays, unrelated calls, and all
  extractvalue-row validation unchanged;
- prove `frontend_lir_call_type_ref` reaches the switch selector check, then
  return and resume 802 unchanged at Step 1 with its parked unaccepted
  verifier/test hunk and exact focused command.

Completion check: the native carrier remains authoritative, mirror and
signature contracts remain fail-closed, and the focused call test reaches the
switch check. This only clears 802's prerequisite; Step 2 still requires its
fresh build, focused call/frontend/backend proof, and supervisor-accepted full
baseline before Step 3.

### Step 3 - Prove and publish the 754 handoff

Goal: establish positive and malformed proof and record the exact field-layout
contract that 754 Step 3 may consume.

Completion check: accepted proof supports reactivation of 754 at unchanged
Step 3 without treating this blocker as extractvalue-row validation. Do not
advance while Step 2 remains unaccepted.
