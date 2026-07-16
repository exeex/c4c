# LIR Next Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/855_lir_next_body_parameter_authority_handoff.md
Supersedes: exhausted 734 Step 7.43 runbook after accepted commit `617a8fae9`

## Purpose

Unblock idea 734 after its accepted Step 7.43 DirectScalar unary-`fneg`
receiver by publishing exactly one next native LIR body-parameter authority
handoff.

## Goal

Select, publish, verify, and hand off one next valid function-body
parameter-use row for a later bounded Raw-BIR receiver packet in 734.

## Core Rule

Use only native structured LIR authority. Do not recover parameter identity,
type, ABI, owner, role, or consumer coherence from text, names, rendered
operands, signatures, diagnostics, compatibility mirrors, `monostate`, or
testcase shape.

## Read First

- `ideas/open/855_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- The accepted Step 7.43 receiver commit `617a8fae9`
- Existing LIR body-parameter authority producers and verifier coverage

## Current Targets And Scope

- Preserve 734's accepted receiver progress through Step 7.43.
- Find exactly one next valid body-parameter use after the accepted
  DirectPointer and DirectScalar rows already listed in 734.
- Publish native producer/schema authority for that one row and verify the
  selected consumer relation.
- Produce a handoff record that lets 734 resume with one Raw-BIR receiver
  packet.

## Non-Goals

- Do not edit Raw-BIR containers, builders, importer dispatch, backend
  verifier code, or receiver tests.
- Do not reopen accepted body-parameter rows through Step 7.43.
- Do not select more than one parameter row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or other non-parameter
  families.
- Do not infer missing authority from presentation fields or testcase shape.

## Execution Rules

- Keep this runbook bounded to one producer/schema/verifier handoff.
- Add nearby same-feature malformed-authority coverage for the selected row.
- For code changes, run a fresh build and focused producer/verifier proof.
- After acceptance, return to 734 for plan-owner repair of one matching
  receiver packet.

## Steps

### Step 1 - Select The Next Body-Parameter Authority Row

Goal: identify one valid current-LIR body-parameter use that still lacks a
734 receiver handoff after Step 7.43.

Actions:

- Inspect the existing body-parameter authority matrix and tests.
- Exclude all rows already accepted through 734 Step 7.43.
- Choose exactly one next row whose producer can carry native structured
  authority without presentation recovery.
- Record why later parameter rows and non-parameter families remain out of
  scope.

Completion check:

- One selected row is named with its expected parameter tuple and consumer
  relation.
- No Raw-BIR or importer file is changed.

### Step 2 - Publish And Verify The Selected Authority

Goal: add native structured authority and verifier checks for the selected row.

Actions:

- Add the smallest producer/schema carrier needed for the selected row.
- Populate it from existing typed parameter and consumer facts.
- Verify parameter value, owner, index, type, ABI, role, and consumer
  coherence.
- Reject missing, invalid, duplicate, foreign, mismatched, and
  consumer-incoherent authority before printing or downstream use.

Completion check:

- Positive and malformed producer/verifier coverage passes.
- No Raw-BIR receiver behavior is claimed.

### Step 3 - Record The Handoff To 734

Goal: make the accepted producer result executable by a later 734 receiver
runbook.

Actions:

- Update the source idea with the exact selected row, accepted proof, and
  commit reference.
- State the exact 734 return action: repair 734 for one bounded Raw-BIR
  receiver packet derived only from this handoff.
- Preserve that all other parameter rows and semantic families remain
  fail-closed and separately scoped.

Completion check:

- Fresh build and focused proof are recorded.
- The handoff is specific enough for plan-owner to repair 734 without
  rediscovering producer facts.
