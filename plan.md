# Next Local/VLA Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/794_lir_next_local_vla_authority_handoff.md
Activated from: 793 successor queue order 1.

## Purpose

Establish one, and only one, remaining local/VLA producer-side authority
handoff before 734 can receive another Raw-BIR row.

## Core Rule

Select a row only from checked native current-function structured evidence.
Do not infer value, object, owner, type, liveness, or row identity from names,
formatted operands, LLVM text, testcase identity, `monostate`, or an unresolved
classification.

## Read First

- `ideas/open/794_lir_next_local_vla_authority_handoff.md`
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`
- `docs/lir_to_new_bir_remaining_coverage/first_owner_matrix.md`
- `docs/lir_local_operation_authority/handoff_to_734.md`
- `ideas/closed/792_lir_next_local_operation_receiver_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Non-Goals

- Raw-BIR destinations, importer dispatch, receiver verification, target
  lowering, MIR, or 734 receipt;
- more than one local/VLA row; memory/va, aggregate/vector, body parameter,
  type-model, or residual instruction work;
- presentation-derived recovery or weaker verifier/test contracts.

## Ordered Steps

### Step 1 - Establish the candidate evidence boundary

Goal: inspect the remaining local/VLA variants and select no candidate unless
native structured fields can support one row.

Actions:

- Record the row candidates, their current value/object/owner/type/liveness
  evidence, and why all nonselected rows remain fail closed.
- If no candidate has an admissible native route, stop and send the exact
  first missing owner fact to plan-owner for a separate blocker decision.

Completion check: exactly one candidate is selected with a structured-evidence
rationale, or an evidence-backed separate-blocker route is recorded.

### Step 2 - Publish and verify the selected producer authority

Goal: implement only the minimum producer/schema/verifier facts for the
selected row and reject malformed or foreign authority before downstream use.

Completion check: the selected row has native current-function authority and
focused nearby positive/negative producer proof; every other row remains fail
closed.

### Step 3 - Write the exact 734 handoff and obtain acceptance disposition

Goal: document selected variant, typed fields, guarantees, rejected forms, and
proof so 734 can later receive one matching row.

Completion check: the handoff lets plan-owner reactivate 734 for one receiver
packet without re-deriving authority or extending scope.
