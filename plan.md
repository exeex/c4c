# LIR Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/854_lir_next_body_parameter_authority_handoff.md

## Purpose

Identify and publish the next one-row native LIR body-parameter authority
handoff needed before 734 can continue receiving parameter facts into typed
Raw BIR.

## Goal

Trace, verify, and hand off exactly one next valid function-body parameter-use
row after 734's accepted Step 7.42 DirectPointer pointer-truthiness receipt.

## Core Rule

Publish only native structured LIR authority. Do not recover parameter
identity, type, ABI, owner, role, or consumer coherence from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, or testcase
shape.

## Read First

- `ideas/open/854_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/853_lir_next_body_parameter_authority_handoff.md`
- Current LIR body-parameter producer/schema/verifier carriers
- Existing focused body-parameter authority tests

## Current Targets And Scope

- Preserve 734's accepted receiver history through Step 7.42, most recently
  commit `96853b2a2`.
- Select exactly one next valid function-body parameter-use row after the
  accepted DirectPointer pointer-truthiness row.
- Publish only the selected row's current-function value identity, owner,
  parameter index, type, ABI, explicit role, and consumer coherence facts.
- Hand off the exact tuple and accepted producer/verifier proof back to 734.

## Non-Goals

- Do not edit Raw-BIR containers, builders, importer, or Raw-BIR verifier.
- Do not reopen accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, truthiness-comparison-LHS,
  fixed-direct-call argument-0, fixed-direct-call argument-1, or
  pointer-truthiness rows.
- Do not publish generic parameter authority, declaration-only authority, or a
  multi-row parameter sweep.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or downstream receiver
  work.

## Execution Rules

- Keep the route bounded to one producer/schema/verifier handoff.
- Add same-feature positive and malformed-authority coverage for the selected
  row.
- For code changes, run a fresh build, focused producer/verifier proof, and
  the supervisor-selected matching regression guard.
- Do not claim Raw-BIR receipt or 734 source completion from this handoff.

## Steps

### Step 1 - Select The Next Body-Parameter Authority Row

Goal: find exactly one next valid function-body parameter-use row after 734's
accepted Step 7.42 DirectPointer pointer-truthiness receipt.

Actions:

- Inspect current LIR producers that use native function-body parameters after
  the accepted body-parameter rows listed in the source.
- Select one bounded semantic consumer relation that can be represented with
  native authority.
- Record why earlier accepted rows are not being reopened and why nonselected
  candidates remain fail closed.

Completion check:

- One selected row and consumer relation are identified.
- No selection depends on text, names, signatures, diagnostics, rendered
  operands, compatibility mirrors, or testcase shape.

### Step 2 - Publish And Verify The Selected Authority

Goal: add the native LIR carrier and verifier checks for only the selected
row.

Actions:

- Publish the selected row's current-function value identity, owner, parameter
  index, `LirTypeRef`, ABI, explicit role, and consumer coherence facts.
- Reject missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority before downstream use.
- Keep nonselected parameter forms fail closed.
- Add focused positive and malformed-authority coverage next to same-feature
  producer/verifier tests.

Completion check:

- Fresh build passes.
- Focused producer/verifier proof passes.
- Matching regression guard is non-regressive.
- `git diff --check` passes.

### Step 3 - Hand Off To 734

Goal: make the accepted producer fact executable by a later 734 receiver
runbook without claiming Raw-BIR receipt.

Actions:

- Record the exact receiver tuple, consumer relation, accepted proof, and 734
  return action in `ideas/open/854_lir_next_body_parameter_authority_handoff.md`.
- State that Raw-BIR container/importer/verifier receipt remains out of scope.
- Return to plan-owner for closure/switch decision after supervisor
  acceptance.

Completion check:

- The handoff is precise enough for plan-owner to repair 734 for exactly one
  later Raw-BIR receiver row.
- The source idea does not claim 734 completion or Raw-BIR receipt.
