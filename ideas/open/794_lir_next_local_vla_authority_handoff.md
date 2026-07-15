# Next LIR Local/VLA Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/closed/793_lir_to_new_bir_remaining_coverage_umbrella.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish exactly one remaining local/VLA semantic row with native
current-function authority, so 734 can later receive that single row.

## In Scope

- Inspect stack restore, dynamic VLA allocation, VLA GEP, nonselected local
  load/store/GEP, local temporaries, and lifetime consumers only to identify
  the earliest row whose structured authority is already provable or can be
  minimally published and verified.
- Publish and verify exactly one selected variant's value/object/owner/type/
  liveness facts, malformed-authority rejection, focused proof, and a precise
  handoff to 734.

## Out Of Scope

- Raw-BIR/importer/verifier/lowering changes or receipt by 734.
- More than one row, a broad local/VLA conversion, memory/va, type-model,
  aggregate/vector, body-parameter, or presentation-derived recovery.

## Acceptance Criteria

- The closure handoff names one selected variant, its native fields, rejected
  forms, and focused positive/negative producer proof.
- Only after that accepted handoff may 734 be reactivated for one matching
  receiver packet; all other local/VLA rows remain fail closed.

## Reviewer Reject Signals

- Reject a row selected from names, rendered operands, LLVM text, testcase
  shape, `monostate`, or an unresolved classification.
- Reject multiple rows, receiver/importer work, weakened verification, or an
  expectation downgrade claimed as this authority handoff.
