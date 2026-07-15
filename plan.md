# LIR Next Function-Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/826_lir_next_body_parameter_authority_handoff.md
Activated from: post-Step 7.38 source-gate reassessment

## Purpose

Publish one, and only one, next native function-body parameter-use authority
row that can later be received by 734 without presentation-derived recovery.

## Core Rule

Select no row until its native value, current-function owner, parameter index,
type, ABI, role, and exact consuming relation are structurally available and
verifiable. Keep every other form fail closed.

## Read First

- `ideas/open/826_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.38
  resumption record)
- `ideas/closed/825_lir_next_body_parameter_authority_handoff.md`
- Existing native LIR producer and verifier seams for candidate body-parameter
  uses

## Non-Goals

- Raw-BIR/importer/receiver edits, generic parameter admission, ABI redesign,
  a second row, or presentation-derived identity.
- Reopening accepted 734 DirectPointer and DirectScalar parameter rows, or
  absorbing memory/VA, aggregate/vector, module/type/global, residual
  instruction/terminator, or inline-assembly work.

## Ordered Steps

### Step 1 - Trace and select one native body-parameter authority row

Goal: identify one candidate whose authority tuple and consuming relation are
available natively, or route the first missing prerequisite separately.

Actions:

- inspect native producer and verifier paths without using rendered text;
- record the selected row's value, owner, index, type, ABI, role, and exact
  consuming relation; and
- keep all nonselected parameter forms fail closed.

Completion check: the selected row is explicit and bounded, or the missing
first-owner prerequisite is evidenced and separately routed before publication.

### Step 2 - Publish and verify only the selected authority contract

Goal: add the minimum native carrier and verifier checks for the Step 1 row.

Actions:

- enforce native value/owner/index/type/ABI/role and consumer coherence;
- reject missing, invalid, duplicate, foreign, and malformed authority
  transactionally as applicable; and
- add nearby positive and malformed-authority producer coverage.

Completion check: focused same-feature proof shows the one selected row is
structured and checked; nonselected forms remain fail closed.

### Step 3 - Record the exact 734 receiver handoff

Goal: preserve the selected tuple, failure boundary, proof, and one bounded
return action for 734.

Actions:

- write the durable handoff into the source idea; and
- state the exact 734 return step without claiming Raw-BIR receipt or
  source-wide coverage completion.

Completion check: the source names all handoff fields, consumer relation,
rejection boundary, focused proof, and exact 734 return point.
