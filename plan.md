# LIR Next Non-Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/865_lir_next_non_body_parameter_authority_handoff.md
Switched From: ideas/open/734_lir_to_new_bir_container_completeness.md after
accepted Step 7.51 receiver commit `750b6b3ba`

## Purpose

Select and publish the next single structured LIR authority row that idea 734
can later receive into typed Raw BIR.

## Goal

Produce one bounded producer/schema/verifier handoff for a valid current-LIR
non-body-parameter semantic row not already accepted by 734 through Step 7.51.

## Core Rule

Publish only native structured authority. Do not recover semantic identity,
type, role, opcode, operand relation, ownership, or consumer coherence from
text, names, rendered operands, signatures, diagnostics, compatibility
mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/865_lir_next_non_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Accepted 734 Step 7.51 receiver commit `750b6b3ba`
- Closed 863 and 864 non-body-parameter handoffs
- Existing LIR producer/verifier authority patterns for the candidate family

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.51 as historical receiver work.
- Inspect current LIR producer and verifier behavior for one non-body-
  parameter family still missing a typed 734 disposition.
- Select exactly one valid row that can carry native structured authority.
- Add only the LIR producer/schema/verifier fields and focused tests required
  for that selected row.
- Return an exact one-row handoff to 734; do not perform Raw-BIR receiver work.

## Non-Goals

- Do not edit Raw-BIR containers, builders, views, importer dispatch, verifier,
  or backend receiver tests.
- Do not reopen accepted direct call-result rows, body-parameter rows,
  DirectPointer, DirectScalar, CFG, PHI, memory/object, or other accepted 734
  receipts.
- Do not select fixed direct-call argument 0 or 1 parameter authority, the
  zero-argument floating call-result row, or the `double(double)` direct
  call-result row.
- Do not absorb a generic residual sweep, final 797 convergence, memory/VA
  sweep, aggregate/vector sweep, module/type/global/metadata sweep, CFG/PHI
  rewrite, inline-assembly template/constraint parsing, or target-lowering
  work.
- Do not weaken unsupported diagnostics, expectation contracts, or fail-closed
  behavior.

## Execution Rules

- Start from the current LIR surface and accepted 734 receiver history.
- Choose one row only after proving it is not already covered by 734 through
  Step 7.51.
- Keep the selected row's authority native, explicit, and verifier checked
  before printing or downstream use.
- Add nearby positive and malformed producer/verifier coverage for the selected
  row.
- Keep nonselected rows fail closed without presentation recovery.
- For code changes, run a fresh build, focused producer/verifier proof, and
  `git diff --check`. Escalate to a matching regression guard when shared LIR
  verifier or producer code is touched.

## Steps

### Step 1 - Trace and select one next non-body-parameter authority row

Goal: identify exactly one valid current-LIR non-body-parameter row that still
lacks a typed 734 receiver disposition and can be made native-authoritative.

Primary targets:

- Current LIR producer/verifier behavior for candidate non-body rows.
- Accepted 734 history through Step 7.51.
- Any stale open predecessor ideas that might appear to overlap the selected
  row.

Actions:

- Enumerate plausible next non-body-parameter candidates without implementing
  them.
- Reject candidates already accepted by 734 through Step 7.51.
- Reject candidates whose authority would require presentation recovery or a
  broader family sweep.
- Select one exact row and record the required native tuple and malformed
  matrix in `todo.md`.

Completion check:

- One candidate row is selected or the route is returned to plan-owner with
  evidence that no bounded non-body row is executable.
- The selected row is not a reopened accepted receiver row or body-parameter
  row.
- The selected row has a concrete native authority tuple and focused malformed
  matrix.

### Step 2 - Publish and verify the selected authority

Goal: add the minimum LIR producer/schema/verifier support for the selected
row.

Actions:

- Add only the carrier, role, producer publication, and verifier checks needed
  for the selected row.
- Require native identity, owner, type, and role coherence before printing or
  downstream use.
- Add focused positive and malformed-authority coverage.
- Keep all nonselected forms fail closed.

Completion check:

- Fresh build passes.
- Focused producer/verifier proof passes.
- `git diff --check` passes.
- Any required matching regression guard is accepted by the supervisor.
- No Raw-BIR/importer receiver file is edited.

### Step 3 - Hand off exactly one receiver row to 734

Goal: record the accepted selected row and the exact 734 return action.

Actions:

- Update this source idea with the selected row, native tuple, producer/
  verifier disposition, malformed matrix, proof, and exact 734 return action.
- State which families remain unsupported or separately scoped.
- Send the completed route to plan-owner for close and return to 734.

Completion check:

- The source idea contains a one-row handoff sufficient to repair 734 without
  re-deriving producer authority.
- The closure record does not claim Raw-BIR receipt.
- The successor back to 734 is exactly one bounded receiver packet.
