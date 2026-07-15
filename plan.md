# Next Local/VLA Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/794_lir_next_local_vla_authority_handoff.md
Resumed from: closed 798 stack-restore lifetime-consumer authority blocker.

## Purpose

Complete exactly one remaining local/VLA producer-side authority handoff using
closed 798's selected native stack-restore authority, before 734 may receive
another Raw-BIR row.

## Core Rule

Use only checked native current-function structured authority. Do not infer
value, object, owner, type, liveness, row identity, or lifetime transition from
names, formatted operands, LLVM text, testcase identity, `monostate`, or an
unresolved classification.

## Read First

- `ideas/open/794_lir_next_local_vla_authority_handoff.md`
- `docs/lir_to_new_bir_remaining_coverage/794_local_vla_candidate_evidence_boundary.md`
- `docs/lir_local_operation_authority/handoff_to_734.md`
- `ideas/closed/798_lir_stack_restore_lifetime_consumer_authority.md`

## Non-Goals

- Raw-BIR destinations, importer dispatch, receiver verification, target
  lowering, MIR, or 734 receipt;
- dynamic-VLA count work, VLA GEP, other local rows, broad local/VLA
  conversion, a generic lifetime-model redesign, or multiple row admissions;
- presentation-derived authority or weaker verifier/test contracts.

## Ordered Steps

### Step 1 - Establish the candidate evidence boundary (complete)

Accepted evidence: commit `1cbad00d6` records the candidate boundary. Its
first missing fact routed the selected stack-restore authority to separate
blocker 798; do not redo the inspection or choose another candidate.

Completion check: preserved accepted Step 1 evidence identifies no broader
candidate and limits the resumed route to closed 798's exact handoff.

### Step 2 - Publish and verify the selected producer authority

Goal: consume only closed 798's selected `LirStackRestoreOp` authority in
794's one-row process and preserve malformed or foreign authority rejection.

Actions:

- Use `requires_native_stack_restore_authority`, `saved_ptr`, checked
  current-function local object/owner/pointer-type/pointee-type/live binding,
  and `RestoreSavedVlaStackCheckpoint` only as published in the closed 798
  return handoff.
- Verify this single selected producer authority with focused nearby proof;
  do not add dynamic-VLA count, VLA GEP, another local row, or Raw-BIR work.

Completion check: selected stack restore has the required native authority and
focused same-feature positive/negative producer proof; every other local/VLA
row remains fail closed.

### Step 3 - Write the exact 734 handoff and obtain acceptance disposition

Goal: document the selected variant, typed fields, guarantees, rejected forms,
and proof so 734 can later receive this one matching row.

Completion check: the handoff lets plan-owner reactivate 734 for one receiver
packet without re-deriving authority or extending scope.
