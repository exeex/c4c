# LIR Next Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/862_lir_next_body_parameter_authority_handoff.md
Switched From: ideas/open/734_lir_to_new_bir_container_completeness.md after accepted Step 7.49

## Purpose

Publish exactly one next structured current-LIR body-parameter authority row so
idea 734 can later receive it into typed Raw BIR.

## Goal

Implement one bounded producer/schema/verifier handoff for the next valid
function-body parameter-use row after accepted 734 Step 7.49.

## Core Rule

Publish native structured LIR authority only. Do not recover semantic identity
from text, names, rendered operands, signatures, diagnostics, compatibility
mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/862_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Accepted 734 Step 7.49 receiver commit `44edfbacd`
- Closed producer handoffs 817, 818, 823, 824, 825, 826, 827, 853, 854, 855,
  856, 857, 858, 859, and 860 for accepted body-parameter authority patterns

## Current Targets And Scope

- Inspect the current LIR body-parameter producer and verifier matrix after
  accepted DirectPointer and DirectScalar receipts through Step 7.49.
- Select exactly one next valid function-body parameter-use row.
- Add only the producer/schema/verifier fields, checks, and tests for that row.
- Preserve a handoff that lets 734 later receive one matching typed Raw-BIR
  row without re-deriving identity from presentation.

## Non-Goals

- Do not edit Raw-BIR containers, builders, views, importer dispatch,
  verifier, or backend receiver tests.
- Do not reopen accepted DirectPointer or DirectScalar body-parameter rows,
  including binary `fmul`, `fadd`, or `fsub` LHS/RHS receipts.
- Do not select multiple rows, run a generic parameter sweep, or absorb
  ABI-expanded/aggregate parameters, memory/VA, aggregate/vector,
  module/type/global/metadata, residual instruction/terminator,
  inline-assembly, or target-lowering work.
- Do not weaken unsupported diagnostics or expectation contracts.

## Execution Rules

- Keep the packet producer-side only and tied to one selected current-LIR row.
- Verify the original parameter `LirValueId`, current-function owner,
  parameter index, `LirTypeRef`, native body-parameter ABI, explicit role, and
  selected consumer relation.
- Reject malformed authority before printing or downstream use.
- Add nearby malformed coverage for absent, invalid, duplicate, foreign,
  owner/index/type/ABI/role-incoherent, and consumer-incoherent forms.
- For code changes, run a fresh build, the focused producer/verifier proof,
  and `git diff --check`. Escalate to a matching broader frontend LIR proof if
  shared producer or verifier code is touched.

## Steps

### Step 1 - Select and publish one next body-parameter authority row

Goal: identify and publish exactly one next valid current-LIR body-parameter
use after accepted 734 Step 7.49.

Primary targets:

- LIR producer/schema surface for the selected row.
- LIR verifier checks for the selected authority tuple.
- Focused frontend LIR coverage for positive and malformed cases.
- Handoff text for returning to 734 after acceptance.

Actions:

- Inspect existing accepted body-parameter authority patterns and the current
  verifier matrix.
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
- No Raw-BIR receiver work, generic parameter sweep, or unrelated family is
  included.
