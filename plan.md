# LIR Next Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/860_lir_next_body_parameter_authority_handoff.md
Switched From: ideas/open/734_lir_to_new_bir_container_completeness.md after accepted Step 7.48 receiver commit `6a91d07ca`

## Purpose

Unblock idea 734 by producing exactly one next structured current-LIR
function-body parameter-use authority handoff. This runbook is producer-side
only and must not edit Raw-BIR receiver code.

## Goal

Select, publish, verify, test, and hand off one next valid body-parameter-use
row after the accepted DirectScalar binary-`fsub` LHS receiver.

## Core Rule

Authority must come from native typed LIR fields and verifier checks. Do not
recover parameter identity, role, type, opcode, operand relation, or consumer
coherence from rendered text, names, signatures, diagnostics, compatibility
mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/860_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Closed predecessor `ideas/closed/859_lir_next_body_parameter_authority_handoff.md`
- Accepted 734 receiver commit `6a91d07ca`
- Existing LIR body-parameter authority producers, verifier checks, and
  focused tests for accepted rows through Step 7.48

## Current Targets And Scope

- Inspect remaining valid current-LIR body-parameter-use forms after the
  accepted DirectPointer and DirectScalar receipts through 734 Step 7.48.
- Choose exactly one next row that can be represented with structured LIR
  authority.
- Add only producer/schema/verifier/test changes required for that one row.
- Preserve the parameter `LirValueId`, current-function owner, parameter
  index, `LirTypeRef`, native ABI, explicit role, and selected consumer
  relation.
- Provide a closure handoff back to 734 naming the exact later Raw-BIR
  receiver packet.

## Non-Goals

- Do not edit Raw-BIR containers, builders, views, importer dispatch,
  verifier, or backend receiver tests.
- Do not reopen accepted 734 DirectPointer or DirectScalar body-parameter
  receiver rows.
- Do not select multiple rows or a generic parameter family.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, ABI-expanded or aggregate
  parameters, or target-lowering work.

## Execution Rules

- Keep this producer-side and tied to one selected row.
- Reject malformed authority before printing or downstream use.
- Add nearby positive and malformed verifier coverage for the selected row.
- Keep all nonselected rows fail-closed without presentation recovery.
- For code changes, run a fresh build, focused producer/verifier proof,
  `git diff --check`, and any matching regression guard required by the
  supervisor.

## Steps

### Step 1 - Select and publish one next body-parameter authority row

Goal: identify the first valid unreceived body-parameter-use row after 734
Step 7.48 and publish structured LIR authority for exactly that row.

Actions:

- Inspect accepted body-parameter rows through Step 7.48 and the remaining
  current-LIR producer surface.
- Select one next bounded row only.
- Add the minimal LIR carrier, producer population, verifier checks, and
  focused positive/malformed coverage required for that row.
- Preserve the full parameter tuple and selected consumer relation in typed
  native fields.
- Document the exact handoff and 734 return action in the source idea closure
  record.

Completion check:

- Fresh build passes.
- Focused producer/verifier proof passes.
- `git diff --check` passes.
- Matching regression guard passes if the selected proof has a comparable
  before/after baseline.
- No Raw-BIR receiver work lands in this idea.
