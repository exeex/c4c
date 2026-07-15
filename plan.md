# Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/829_lir_next_body_parameter_authority_handoff.md
Activated from: 734 post-Step 7.40 separate-blocker decision

## Purpose

Publish one exact next native function-body parameter-use authority row so 734
can later receive that row without presentation-derived recovery.

## Core Rule

Select and publish one existing producer relation only. Native structured
authority, not names, signatures, diagnostics, printed operands, or
compatibility fields, must carry every semantic fact needed by the later
receiver.

## Non-Goals

- Raw-BIR/importer/container/verifier work or receiver implementation.
- Generic parameter support, ABI conversion, or a multi-row sweep.
- Reopening any authority row already received by 734 through Step 7.40.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, and unrelated 821/822 material.

## Ordered Steps

### Step 1 - Trace and select one next native parameter-use relation

Goal: identify one currently produced, valid function-body parameter consumer
after the accepted fixed-direct-call argument-0 row and demonstrate why it
requires a new typed authority tuple.

Actions:

- inspect only native LIR construction, its verifier, and nearby focused test
  surface;
- record the selected current-function value/owner/index/type/ABI/role and
  exact consumer coherence relation; and
- leave every nonselected form fail closed without using presentation fields.

Completion check: a single receiver-consumable relation and its bounded
producer/verifier seam are explicit; otherwise conclude no eligible row rather
than broadening scope.

### Step 2 - Publish and verify the selected authority

Goal: add only the selected structured carrier and native verification.

Actions:

- populate authority directly from the existing parameter definition and
  selected consumer;
- reject missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority before downstream use; and
- add same-feature positive plus malformed-authority coverage.

Completion check: fresh build and exact focused producer proof pass with the
selected semantic relation exercised.

### Step 3 - Record the one-row handoff to 734

Goal: make the producer result receiver-ready without implementing receipt.

Actions:

- record the exact authority tuple, consumer relation, implementation commit,
  and accepted focused proof in the source idea; and
- name 734's exact next bounded receiver return action.

Completion check: 829 can close as a producer-only capability and 734 can be
reactivated without rediscovering the selected row.
