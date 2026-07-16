# LIR Next Non-Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/863_lir_next_non_body_parameter_authority_handoff.md
Switched From: ideas/closed/862_lir_next_body_parameter_authority_handoff.md after no-change conclusion

## Purpose

Publish exactly one next structured current-LIR non-body-parameter authority
row so idea 734 can later receive it into typed Raw BIR.

## Goal

Implement one bounded producer/schema/verifier handoff for the next valid
non-body-parameter semantic row after accepted 734 Step 7.49 and 862's
no-change body-parameter conclusion.

## Core Rule

Publish native structured LIR authority only. Do not recover semantic identity
from text, names, rendered operands, signatures, diagnostics, compatibility
mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/863_lir_next_non_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Accepted 734 Step 7.49 receiver commit `44edfbacd`
- Closed no-change route `ideas/closed/862_lir_next_body_parameter_authority_handoff.md`
- Existing open family owners such as 796 only as context; do not absorb stale
  return records without supervisor approval.

## Current Targets And Scope

- Inspect the current LIR producer and verifier matrix for one non-body-
  parameter family still missing a typed 734 disposition.
- Select exactly one next valid semantic row with native structured authority.
- Add only the producer/schema/verifier fields, checks, and tests for that row.
- Preserve a handoff that lets 734 later receive one matching typed Raw-BIR
  row without re-deriving identity from presentation.

## Non-Goals

- Do not edit Raw-BIR containers, builders, views, importer dispatch,
  verifier, or backend receiver tests.
- Do not select a function-body parameter row or reopen accepted DirectPointer
  or DirectScalar body-parameter rows.
- Do not select multiple rows, run a generic residual sweep, or absorb
  memory/VA, aggregate/vector, module/type/global/metadata, CFG/PHI,
  instruction/terminator, inline-assembly, or target-lowering work beyond the
  one selected row.
- Do not weaken unsupported diagnostics or expectation contracts.

## Execution Rules

- Keep the packet producer-side only and tied to one selected current-LIR row.
- Verify the selected row's native identity, current-function or module owner
  as applicable, `LirTypeRef` or other structured type facts, explicit role,
  and selected consumer relation.
- Reject malformed authority before printing or downstream use.
- Add nearby malformed coverage for absent, invalid, duplicate, foreign,
  owner/type/role-incoherent, and consumer-incoherent forms.
- For code changes, run a fresh build, the focused producer/verifier proof,
  and `git diff --check`. Escalate to a matching broader frontend LIR proof if
  shared producer or verifier code is touched.

## Steps

### Step 1 - Select and publish one next non-body-parameter authority row

Goal: identify and publish exactly one next valid current-LIR
non-body-parameter semantic row after accepted 734 Step 7.49 and the 862
no-change conclusion.

Primary targets:

- LIR producer/schema surface for the selected row.
- LIR verifier checks for the selected authority tuple.
- Focused frontend LIR coverage for positive and malformed cases.
- Handoff text for returning to 734 after acceptance.

Actions:

- Inspect existing accepted authority patterns, open family owners, and the
  current verifier matrix.
- Choose one row that is current-LIR valid and not already accepted by 734.
- Add only the structured authority carrier and verification required for that
  selected row.
- Document the exact handoff tuple, malformed matrix, proof, and 734 return
  action in this idea before closure.

Completion check:

- Fresh build passes.
- Focused producer/verifier proof passes.
- `git diff --check` passes.
- Any required matching broader frontend LIR proof passes.
- No Raw-BIR receiver work, body-parameter row, generic sweep, or unrelated
  family is included.
