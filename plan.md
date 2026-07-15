# LIR Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/824_lir_next_body_parameter_authority_handoff.md
Activated from: post-Step 7.36 separate-blocker decision for 734

## Purpose

Establish one and only one native structured function-body parameter-use
authority handoff before 734 receives another Raw-BIR row.

## Core Rule

Use only native structured current-function authority. Text, names, rendered
operands, signatures, diagnostics, `monostate`, and testcase identity are not
authority.

## Non-Goals

- Do not edit Raw-BIR, importer, builder, or receiver code.
- Do not reopen accepted direct-pointer or DirectScalar binary LHS/RHS rows.
- Do not combine parameter forms or absorb other semantic families.

## Ordered Steps

### Step 1 - Trace and select one receiver-candidate parameter-use row

Goal: identify one valid native body-use with enough existing or minimally
publishable structured facts for a later single 734 receiver row.

Actions:

- trace the producer, LIR schema, and verifier path for one candidate only;
- classify its current-function value identity, owner, index, type, ABI, role,
  and exact consuming operand relation;
- if the facts are absent or require broader authority, stop and create a
  separately scoped successor rather than recovering them from presentation.

Completion check: one exact candidate contract or an evidence-backed split is
recorded; no producer or receiver behavior changes occur in this step.

### Step 2 - Publish and verify the selected structured authority

Goal: implement only the minimum producer/schema/verifier authority contract
selected in Step 1.

Actions:

- publish the native carrier and enforce current-function ownership, validity,
  uniqueness, type, ABI, role, and operand-relation checks relevant to that row;
- retain fail-closed behavior for missing, malformed, foreign, duplicate, and
  nonselected forms;
- add nearby same-feature positive and malformed-authority coverage.

Completion check: exactly one selected authority row is natively structured
and verified without generic parameter admission or receiver changes.

### Step 3 - Prove and hand off the one row to 734

Goal: create an exact receiver contract or route a newly discovered prerequisite.

Actions:

- run a fresh build and focused same-feature proof selected by the supervisor;
- record the carrier fields, malformed rejection boundary, accepted commit,
  and proof in the source idea;
- close this idea only if the row is handoff-ready; then reactivate 734 for
  exactly the matching receiver packet.

Completion check: 734 has an unambiguous one-row resume contract, or a named
separate successor owns the missing prerequisite.
