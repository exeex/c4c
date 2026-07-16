# LIR Next Non-Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/864_lir_next_non_body_parameter_authority_handoff.md
Switched From: ideas/open/734_lir_to_new_bir_container_completeness.md after
accepted Step 7.50

## Purpose

Select and publish exactly one next producer-side non-body-parameter authority
row so idea 734 can later receive one more typed Raw-BIR row.

## Goal

Create one bounded LIR producer/schema/verifier handoff for the next valid
current-LIR semantic row not already accepted by 734 through Step 7.50.

## Core Rule

Publish native structured authority only. Do not recover identity, type, role,
opcode, operand relation, or consumer coherence from text, names, rendered
operands, signatures, diagnostics, compatibility mirrors, `monostate`, or
testcase shape.

## Read First

- `ideas/open/864_lir_next_non_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/863_lir_next_non_body_parameter_authority_handoff.md`
- Accepted 734 Step 7.50 receiver commit `72a368b06`

## Current Targets And Scope

- Inspect the remaining current-LIR semantic surface after accepted 734 Step
  7.50.
- Select exactly one non-body-parameter row that can be published with native
  structured authority.
- Add only the producer/schema/verifier carrier and focused coverage for that
  one row.
- Hand off the exact selected row, fields, malformed matrix, and 734 return
  action.

## Non-Goals

- Do not edit Raw-BIR containers, builders, views, importer dispatch, verifier,
  or backend receiver tests.
- Do not reopen accepted 734 receiver rows through Step 7.50.
- Do not select a function-body parameter row, multiple rows, a generic
  residual sweep, or final 797 convergence.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata, CFG/
  PHI, instruction/terminator, or inline-assembly families unless exactly one
  selected row from one family is the bounded handoff.

## Execution Rules

- Keep selection evidence explicit: name the chosen current-LIR row and why it
  is valid, unaccepted by 734, and producer-ready.
- Verify malformed authority before printing or downstream use.
- Keep nonselected families fail-closed and separately scoped.
- For code changes, run a fresh build, focused producer/verifier proof, and
  `git diff --check`. Escalate to broader proof if shared verifier or producer
  code is touched.

## Steps

### Step 1 - Select and hand off one next non-body-parameter authority row

Goal: produce exactly one structured LIR authority handoff for 734.

Primary targets:

- LIR producer/schema surface for the selected row.
- LIR verifier checks for positive and malformed authority.
- Focused producer/verifier tests for the selected row.
- Source idea closure note naming the 734 return action.

Actions:

- Inspect the post-Step 7.50 remaining matrix and select one valid non-body-
  parameter row with native structured authority potential.
- Add the minimal authority carrier and publication path for that row.
- Add verifier rejection for absent, invalid, duplicate, foreign, owner/type/
  role-incoherent, and consumer-incoherent forms as applicable.
- Add focused coverage for the positive row and nearby malformed matrix.
- Update the 864 source idea on closure with the selected tuple, proof, and
  exact 734 receiver return action.

Completion check:

- Exactly one row is selected and handed off.
- Fresh build passes.
- Focused producer/verifier proof passes.
- `git diff --check` passes.
- No Raw-BIR receiver work or presentation recovery lands in this idea.
