# Next Function-Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/827_lir_next_body_parameter_authority_handoff.md
Switched from: ideas/open/734_lir_to_new_bir_container_completeness.md after accepted Step 7.39

## Purpose

Publish and prove one next structured function-body parameter-use authority
row so 734 can later receive exactly that row into Raw BIR.

## Core Rule

Select one existing native semantic consumer relation and carry only its
structured current-function facts. Never recover authority from display text,
names, signatures, rendered operands, or diagnostics.

## Non-Goals

- Raw-BIR/importer/receiver changes or any 734 receiver implementation.
- Generic parameter admission, ABI conversion, or a multi-row sweep.
- Reopening accepted 817--826 authority rows or absorbing other LIR families.

## Ordered Steps

### Step 1 - Trace and select one next native parameter-use row

Goal: identify one currently produced parameter use whose complete typed
authority and consumer relation can be bounded without presentation recovery.

Actions:

- inspect native LIR producer and verifier seams for the first valid candidate;
- record why its value, owner, index, type, ABI, role, and consumer coherence
  are structured or what bounded producer publication is required; and
- leave every nonselected form fail closed.

Completion check: the selected one-row contract and exact proof target are
written to `todo.md`; if no safe row exists, return a narrowly evidenced
blocker rather than guessing.

### Step 2 - Publish and verify the selected authority

Goal: add only the selected native carrier and fail-closed validation.

Actions:

- implement structured current-function authority for the selected row;
- validate missing, invalid, duplicate, foreign, owner/index/type/ABI/role,
  and consumer-incoherent forms before downstream use; and
- add nearby positive and malformed-authority coverage without receiver edits.

Completion check: a fresh build and focused same-feature producer proof pass;
nonselected forms remain rejected.

### Step 3 - Record the receiver handoff and return to 734

Goal: conclude this bounded producer route with an executable receiver return.

Actions:

- record the exact authority tuple, consumer relation, proof, and commit;
- state the one corresponding Raw-BIR receiver boundary for 734; and
- return control to 734 without claiming Raw-BIR receipt or source completion.

Completion check: supervisor accepts the selected proof and plan-owner can
close this handoff and reactivate 734 at the stated bounded receiver step.
