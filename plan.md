# LIR Logical RHS Result Authority Publication Runbook

Status: Active
Source Idea: ideas/open/778_lir_logical_rhs_result_authority_publication.md
Resumed from: closed 780's accepted cross-function native value-ID ownership repair.

## Goal

Publish one native logical RHS conversion result ID and its bounded 775 handoff
without migrating generic expression APIs or changing the downstream PHI
receiver.

## Core Rule

Allocate authority with `fresh_value` before rendering; raw display text must
not be used to create or recover a value ID.

## Read First

- `ideas/open/778_lir_logical_rhs_result_authority_publication.md`
- `ideas/closed/779_lir_cast_result_authority_contract.md`
- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `src/codegen/lir/hir_to_lir/expr/binary.cpp`
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- no logical PHI/incoming/result carrier work or final logical consumer claim
- no ternary/coerce, vaarg, or generic expression migration
- no text recovery, maps, side tables, Raw-BIR/importer, backend, target
  lowering, MIR, or emission work

## Accepted Progress

- Step 1, logical RHS conversion allocation, accepted in `3b716c12d`.
- Step 2, selected RHS native-result-authority opt-in, accepted in `54ebfa4df`.
- Step 3, focused positive and malformed authority proof, accepted in
  `b4685da80`.
- Closed blocker 780 restored the shared cross-function ownership baseline in
  `7b9d6152b` and `3602e8fd2`; its focused guard passed 2/2 and the supervisor
  accepted a 3037/3037 full-suite candidate.

## Ordered Steps

### Step 1 - Publish the logical RHS conversion result allocation (accepted)

The selected non-`i1` RHS conversion uses a native current-function
`fresh_value` result. No PHI, generic API, or other-family work is included.

### Step 2 - Opt the selected logical RHS cast into native result authority (accepted)

Only the selected RHS conversion opts into the accepted standalone cast-result
verifier contract. The verifier and IR contracts remain unchanged here.

### Step 3 - Prove the logical RHS result authority contract (accepted)

Focused structural positive and malformed missing, invalid, duplicate, and
foreign result-authority proof is accepted for the selected RHS conversion.

### Step 4 - Publish the bounded 775 handoff

Goal: first re-establish 778's parent acceptance boundary after closed blocker
780, then record the logical-RHS-only handoff to 775.

Actions:

- rerun the focused build/test and the matching focused guard:
  `^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr52129_c)$`;
- obtain a fresh supervisor-owned full-suite candidate against the restored
  baseline; do not publish the handoff or close 778 unless it restores
  non-regression;
- after that acceptance gate passes, publish only the logical RHS conversion
  field, accepted proof, unresolved raw PHI result/incoming boundary, and 775
  return point; do not reactivate 775 or 751.

Completion check:

- the focused build/guard and fresh full-suite candidate pass, then 775 may
  consume the logical-RHS-only producer fact while PHI and other-family work
  remain separate.

## Proof

- Parent pre-handoff gate: fresh build, focused guard
  `^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr52129_c)$`, then a
  supervisor-owned fresh full-suite candidate.
- The supervisor owns canonical regression logs; this runbook does not write
  or roll them forward.
