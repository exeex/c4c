# LIR Anonymous Aggregate Layout Type Facts Runbook

Status: Active
Source Idea: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Activated from: blocked Step 3 of 754; return to 754 only after this bounded
type-model handoff is accepted.

## Purpose

Provide native anonymous aggregate field-layout/type facts needed by the
selected extractvalue row without letting compatibility text become authority.

## Core Rule

Native structured field facts are authority. `LirTypeRef` rendering may mirror
an anonymous aggregate but must not be parsed to create or repair its layout.

## Read First

- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- closed composite type-model history and current LIR type construction/verifier seams
- direct-complex aggregate lowering plus nearby focused tests

## Non-Goals

- `LirExtractValueOp` result/use/index/result-type row validation.
- Raw-BIR, other aggregate/vector rows, broad type rewrite, lowering, MIR,
  emission, and all text-derived layout recovery.

## Ordered Steps

### Step 1 - Trace and select anonymous aggregate layout facts (accepted)

Goal: identify the exact anonymous aggregate construction and verification
boundary and select the smallest checked native field-layout carrier.

Actions:

- trace the direct-complex aggregate type from construction to verifier use;
- identify required field ordering, type ownership, and malformed proof seams;
- state the exact downstream 754 handoff and keep row validation out of scope.

Completion check: accepted in `827dae5bd3`; one bounded native layout contract
is explicit and no compatibility-text parsing or extractvalue-row work is
selected.

### Step 2 - Repair anonymous layout / structured-call compatibility

Goal: repair the rejected anonymous-layout implementation so native ordered
field facts remain checked without making a direct-complex `LirCallOp`'s
structured callee signature disagree with its call arguments.

Actions:

- start from rejected implementation commit `201f229d3` and the failing
  `frontend_lir_call_type_ref` verifier evidence; locate the ownership/type
  construction mismatch rather than weakening the signature verifier;
- repair only the anonymous-layout construction/model/validation seam needed
  to keep the direct-complex call's `callee_signature`, `arg_type_refs`, and
  structured arguments coherent;
- retain checked native field-count/field-type access and malformed-layout
  rejection; leave named structs, arrays, unrelated calls, and all
  extractvalue-row validation unchanged;
- add nearby direct-complex and call-signature coverage for the repaired
  contract, including rejection of incoherent structured facts where relevant.

Completion check: the native carrier remains authoritative, the direct-complex
call verifies, and all of the following fresh proof is accepted by the
supervisor:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$' --output-on-failure`
- supervisor-owned `ctest --test-dir build -j --output-on-failure` baseline,
  recorded in canonical `test_after.log`, compared with `test_before.log`, and
  explicitly accepted through `scripts/plan_review_state.py accept-baseline`.

Return point: remain at Step 2 until this proof is accepted.  A clean narrow
subset alone is insufficient; the rejected candidate added 48 full-suite
failures and was rejected with `scripts/plan_review_state.py reject-baseline`.

### Step 3 - Prove and publish the 754 handoff

Goal: establish positive and malformed proof and record the exact field-layout
contract that 754 Step 3 may consume.

Actions:

- after Step 2 acceptance, publish the repaired native field-layout contract
  and its focused positive/malformed evidence;
- reuse the accepted Step 2 build, call-signature, direct-complex, backend,
  and full-baseline proof; rerun any proof made stale by handoff documentation;
- state permitted facts and rejected forms for 754's field/index/result check.

Completion check: accepted proof supports reactivation of 754 at unchanged
Step 3 without treating this blocker as extractvalue-row validation.  Do not
advance to this handoff while Step 2 remains rejected.
