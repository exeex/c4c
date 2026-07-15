# Next Function-Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/827_lir_next_body_parameter_authority_handoff.md
Resumed from: closed 828's accepted shared-worktree isolation checkpoint

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
- Restoring, accepting, or changing the preserved 821/822 slice.

## Ordered Steps

### Step 1 - Trace and select one next native parameter-use row

Status: complete before the 828 isolation switch.

Selected contract: an unchanged native DirectScalar current-function parameter
used as fixed direct-call argument 0. Its required authority is the exact
`LirCallOp.structured_args[0]` tuple with matching current-function
definition/value/owner/index/type/`DirectScalar` ABI, role
`FixedDirectCallArgument0`, and coherence with structured argument 0 and fixed
callee parameter 0.

### Step 2 - Publish and verify the selected authority

Goal: add only the selected native carrier and fail-closed validation.

Actions:

- implement structured current-function authority only for the selected fixed
  direct-call argument-0 row;
- reject missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority; and
- add nearby positive and malformed-authority coverage without receiver edits.

Completion check: fresh `cmake --build --preset default`, then `ctest
--test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'` pass;
nonselected forms remain rejected.

### Step 3 - Record the receiver handoff and return to 734

Goal: conclude this bounded producer route with an executable receiver return.

Actions:

- record the exact authority tuple, consumer relation, proof, and commit;
- state the one corresponding Raw-BIR receiver boundary for 734; and
- return control to 734 without claiming Raw-BIR receipt or source completion.

Completion check: supervisor accepts the selected proof and plan-owner can
close this handoff and reactivate 734 at the stated bounded receiver step.
