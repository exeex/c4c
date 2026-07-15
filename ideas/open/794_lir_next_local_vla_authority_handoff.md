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

## Resumption Record: stack-restore lifetime-consumer authority blocker

Status: parked at a separate-blocker switch after completed Step 1.

Last accepted progress: Step 1, **Establish the candidate evidence boundary**,
is complete with no candidate selected.  Commit `1cbad00d6` accepted
`docs/lir_to_new_bir_remaining_coverage/794_local_vla_candidate_evidence_boundary.md`;
its structural proof was `git diff --check` plus inspection of the documented
candidate boundary.  The preceding umbrella evidence chain is `dc6b18421`,
`248a0e39e`, and `fad426b23`.

Interrupted step: Step 2, **Publish and verify the selected producer
authority**.  The exact blocking fact is that `LirStackRestoreOp` has native
saved-pointer/object/owner/type/liveness binding, but no selected-row
admission and no structured lifetime-consumer or transition facts. That
producer/schema/verifier work was outside 794's candidate-selection boundary,
so separate blocker 798 owned it; its completed durable record is now
`ideas/closed/798_lir_stack_restore_lifetime_consumer_authority.md`.

Return condition and exact next action: after 798 publishes and verifies one
exact native stack-restore handoff with accepted focused proof, resume 794 at
Step 2 only to publish and verify that selected authority, or complete its
handoff process.  Do not widen the return to dynamic VLA allocation/count,
VLA GEP, other local rows, Raw-BIR/importer/734 receipt, or
presentation-derived facts.

## Resumption Confirmation: 798 closed

798 is capability-complete and archived at
`ideas/closed/798_lir_stack_restore_lifetime_consumer_authority.md`. Its exact
return handoff is in `docs/lir_local_operation_authority/handoff_to_734.md`;
accepted contract, implementation, and handoff commits are `8bd881842`,
`cdeacb2cd`, and `f5cfa52b7`. Resume now at Step 2 only, preserving accepted
Step 1 evidence `1cbad00d6`; the selected row remains `LirStackRestoreOp` and
the original no-expansion boundary remains in force.
