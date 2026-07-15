# Next LIR Local-Operation Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/792_lir_next_local_operation_receiver_handoff.md
Activated from: exhausted 734 Step 7.29 receiver runbook.

## Purpose

Publish one exact next local-operation producer authority row so 734 can later
receive it without presentation-derived identity recovery.

## Goal

Select, verify, and hand off exactly one receiver-ready post-GEP local row.

## Core Rule

Native structured authority is the sole semantic input. Local spelling,
formatted operands, printer output, LLVM text, and testcase identity are never
authority.

## Read First

- `ideas/open/792_lir_next_local_operation_receiver_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.29
  resumption record)
- `docs/lir_local_operation_authority/handoff_to_734.md`
- current LIR local-object producer and verifier routes

## Scope

- inspect candidate post-GEP local rows and choose one only when its native
  authority is sufficient;
- add minimum producer/schema/verifier support and focused coverage for that
  one row;
- publish the exact 734 handoff for Step 7.30.

## Non-Goals

- Raw-BIR/importer work, target lowering, MIR, emission, and any broad local
  conversion;
- more than one local row or any memory/va, aggregate/vector, body-parameter,
  CFG/PHI, or later family;
- presentation-derived recovery.

## Ordered Steps

### Step 1 - Select one native-authority local-operation row

Goal: identify the earliest candidate that has complete native current-function
result/use, pointer/object, type, and lifetime authority.

Actions:

- inspect only relevant local producer and verifier routes;
- reject candidates that require text-derived identity or unsupported facts;
- document the selected row and exact rejected forms in the handoff.

Completion check: one exact receiver row is selected from native authority, or
an evidence-backed no-row conclusion names the next lifecycle route.

### Step 2 - Publish and verify the selected authority

Goal: make the one selected row structurally consumable and fail closed.

Actions:

- add the minimum producer/schema and verifier admission required;
- validate current-function ownership, pointer/object/type coherence, and
  liveness before downstream use;
- add nearby positive and malformed-authority coverage.

Completion check: the selected row has native typed authority and malformed or
nonselected forms reject without presentation recovery.

### Step 3 - Record the 734 handoff and prove the bounded producer slice

Goal: close this authority handoff with a precise one-row return action.

Actions:

- update `docs/lir_local_operation_authority/handoff_to_734.md` with fields,
  guarantees, rejected forms, proof, and Step 7.30 action;
- run a fresh build and focused same-feature proof;
- provide the result to the supervisor for selected broader acceptance.

Completion check: the handoff authorizes exactly one 734 receiver packet and
all later families remain fail closed.
