# LIR Anonymous Aggregate Layout Type Facts Runbook

Status: Active
Source Idea: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Resumed from: accepted 796 bounded cast-result recurrence return; unchanged
Step 2.

## Purpose

Provide native anonymous aggregate field-layout/type facts needed by the
selected extractvalue row without letting compatibility text become authority.

## Core Rule

Native structured field facts are authority. `LirTypeRef` rendering may mirror
an anonymous aggregate but must not be parsed to create or repair its layout.

## Read First

- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md`
- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
- `ideas/closed/810_lir_gep_producer_result_authority_baseline_blocker.md`
- `ideas/closed/802_lir_switch_selector_type_reference_verifier.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- direct-complex aggregate lowering plus nearby focused tests

## Non-Goals

- `LirExtractValueOp` result/use/index/result-type row validation.
- Reopening accepted 796 cast-result, 810 GEP, or 802 selector verifier work.
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

Goal: evaluate and repair the preserved anonymous-layout implementation so
native ordered field facts remain checked without making a direct-complex
`LirCallOp`'s structured callee signature or argument type mirror disagree
with its call arguments.

Actions:

- start from rejected implementation commit `201f229d3` and the preserved
  unaccepted in-progress Step 2 repair; locate the ownership/type construction
  mismatch rather than weakening verifier contracts;
- retain the native `arg_type_refs` argument-mirror and callee-signature
  contracts without treating rendered diagnostic/call text as type authority;
- retain checked native field-count/field-type access and malformed-layout
  rejection; leave named structs, arrays, unrelated calls, and all
  extractvalue-row validation unchanged;
- keep accepted 802 selector, 810 GEP, and 796 cast-result contracts as
  environmental prerequisites, not 801 progress or scope.

Completion check: the native carrier remains authoritative; mirror and
signature contracts remain fail-closed; the required fresh build, focused
call/frontend/backend proof, and supervisor-accepted full baseline pass before
Step 3. Do not advance on a narrow focused result alone.

### Step 3 - Prove and publish the 754 handoff

Goal: establish positive and malformed proof and record the exact field-layout
contract that 754 Step 3 may consume.

Completion check: accepted proof supports reactivation of 754 at unchanged
Step 3 without treating this blocker as extractvalue-row validation. Do not
advance while Step 2 remains unaccepted.
