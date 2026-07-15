# LIR Body-Parameter Receiver Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/817_lir_body_parameter_receiver_authority_handoff.md
Switched from: 734 after accepted Step 7.33 (`3b8870e2b`).

## Purpose

Produce one checked function-body parameter-use authority handoff for a later, single 734 Raw-BIR receiver packet.

## Core Rule

Only native current-function value/type/ownership facts establish parameter-use authority. Declaration publication and presentation text are non-authoritative.

## Read First

- `ideas/open/817_lir_body_parameter_receiver_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/795_lir_body_parameter_authority_handoff.md`
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`

## Non-Goals

- Raw-BIR receipt or changes to 734.
- Multiple parameter forms, broad ABI work, or non-parameter residual families.
- Text-derived identity/type recovery or weaker verifier/test contracts.

## Ordered Steps

### Step 1 - Trace and select one body-parameter use authority row

Goal: locate the smallest native current-function body-use row whose value ID, type, ownership, and required ABI classification are present or can be narrowly published.

Actions:

- distinguish body-use authority from closed 742/795 declaration or baseline-only work;
- select exactly one row, or explicitly split if no single row is native and checkable;
- define malformed, foreign, duplicate, and type-incoherent rejection without parsing text.

Completion check: one selected row and native authority tuple are explicit; all other parameter forms remain fail closed.

### Step 2 - Publish and verify the selected authority contract

Goal: add only the producer/schema/verifier support required for the selected row.

Completion check: native ownership/type checks admit only the selected row and nearby positive/malformed coverage proves the boundary.

### Step 3 - Prove and record the 734 handoff

Goal: run fresh build and focused proof, then record allowed fields, rejected forms, proof, and the one-row 734 return point.

Completion check: 817 can close and 734 can reactivate only for the documented receiver row; no Raw-BIR receipt occurs here.
