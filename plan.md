# LIR Next Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/858_lir_next_body_parameter_authority_handoff.md
Activated From: ideas/open/734_lir_to_new_bir_container_completeness.md post-Step 7.46 separate-blocker decision

## Purpose

Select and publish exactly one next structured LIR body-parameter authority row
so idea 734 can later receive that row into typed Raw BIR without presentation
recovery.

## Goal

Implement one bounded producer/schema/verifier handoff for the next valid
function-body parameter-use row after accepted 734 Step 7.46.

## Core Rule

Publish native structured LIR authority only. Do not derive parameter identity,
role, type, opcode, operand relation, or consumer coherence from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors,
`monostate`, or testcase shape.

## Read First

- `ideas/open/858_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Accepted 734 receiver commit `d5a301ec9`
- Closed 857 producer handoff `ideas/closed/857_lir_next_body_parameter_authority_handoff.md`
- Existing LIR body-parameter authority producer/verifier patterns for the
  accepted DirectPointer and DirectScalar rows through 734 Step 7.46

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.46 as historical receiver work.
- Inspect current LIR body-parameter use sites and choose exactly one next
  valid parameter-use row that can carry native structured authority.
- Add only the producer/schema/verifier support for that selected row.
- Hand off the exact tuple, malformed matrix, proof, and 734 return action.

## Non-Goals

- Do not edit Raw-BIR containers, builders, views, importer dispatch,
  verifier, or backend receiver tests.
- Do not reopen any accepted DirectPointer or DirectScalar body-parameter row.
- Do not select multiple rows or perform a generic parameter sweep.
- Do not absorb ABI-expanded or aggregate parameters, memory/VA,
  aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, target-lowering, or later backend
  work.
- Do not weaken unsupported diagnostics or expectation contracts.

## Execution Rules

- Keep the packet producer-side only and tied to one selected current-LIR row.
- Reuse the established native body-parameter authority conventions where they
  fit the selected row.
- Reject malformed authority before printing or downstream consumption.
- Add focused positive and malformed producer/verifier coverage for the
  selected tuple and consumer relation.
- For code changes, run a fresh build, focused producer/verifier proof, and
  `git diff --check`. Escalate to the matching broader guard when shared LIR
  verifier or producer code is touched.

## Steps

### Step 1 - Select and publish one next body-parameter authority row

Goal: choose exactly one next valid function-body parameter-use row and give it
native LIR authority.

Primary targets:

- LIR producer/schema surface for the selected row.
- LIR verifier checks for the selected authority tuple and consumer relation.
- Focused producer/verifier coverage and handoff documentation.

Actions:

- Inspect the accepted body-parameter authority rows through 734 Step 7.46 and
  exclude all already received rows.
- Select one next valid current-LIR parameter-use row that can be expressed
  with native value identity, owner, parameter index, `LirTypeRef`, ABI, role,
  and selected consumer coherence.
- Add only the required authority carrier, producer population, verifier
  validation, and focused malformed coverage for that row.
- Document the exact handoff back to 734, including fields, malformed cases,
  accepted proof, and unsupported/nonselected rows.
- Keep Raw-BIR receiver work out of this idea.

Completion check:

- Fresh build passes.
- Focused producer/verifier proof passes.
- `git diff --check` passes.
- Any required matching regression guard is non-regressive.
- The handoff names one exact 734 receiver return action and does not claim
  Raw-BIR receipt.
