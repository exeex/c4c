# LIR Next Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/856_lir_next_body_parameter_authority_handoff.md
Switched From: ideas/open/734_lir_to_new_bir_container_completeness.md after accepted Step 7.44 receiver commit `e0540da75`

## Purpose

Find and prove exactly one next native function-body parameter-use authority
row for 734 without using presentation-derived identity.

## Goal

Publish a verifier-checked producer handoff for one body-parameter row that
734 can later receive into typed Raw BIR.

## Core Rule

Use only native structured LIR authority. Do not select, prove, or repair a
parameter row from text, names, rendered operands, diagnostics, signatures,
compatibility mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/856_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Accepted 734 Step 7.44 receiver commit `e0540da75`
- Closed 855 producer handoff `ideas/closed/855_lir_next_body_parameter_authority_handoff.md`
- Existing LIR body-parameter verifier and producer patterns

## Current Targets And Scope

- Preserve 734's accepted receiver progress through Step 7.44.
- Trace the remaining current LIR function-body parameter-use matrix after the
  accepted DirectScalar binary-`fmul` LHS receipt.
- Select one next valid parameter-use row only if it can carry native
  current-function parameter identity, owner, index, type, ABI, explicit role,
  and selected consumer coherence.
- Produce an exact handoff back to 734 for one future Raw-BIR receiver packet.

## Non-Goals

- Do not edit Raw-BIR containers, builders, importer dispatch, reachable Raw
  verifier paths, or backend receiver tests.
- Do not reopen accepted DirectPointer or DirectScalar body-parameter receipts
  through 734 Step 7.44.
- Do not select or implement more than one parameter-use row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or other non-parameter
  families.

## Execution Rules

- Keep the route producer-side only.
- If no native receiver-ready row exists, record the first missing producer
  authority owner rather than weakening 734's no-presentation-recovery rule.
- Add nearby same-feature positive and malformed producer/verifier coverage for
  the selected row.
- For code changes, run a fresh build and focused producer/verifier proof.
- End with an exact handoff naming the selected row, tuple, consumer relation,
  proof, commit, and 734 return action.

## Steps

### Step 1 - Trace And Select One Next Body-Parameter Use Authority Row

Goal: identify the next valid current-LIR function-body parameter use after
734 Step 7.44 that can be proved with native structured authority.

Actions:

- Inspect the body-parameter authority matrix and current producer/verifier
  surfaces after accepted binary-`fmul` LHS receipt.
- Exclude already accepted rows through 734 Step 7.44.
- Choose exactly one row only if native fields can prove parameter value,
  owner, index, type, ABI, explicit role, and selected consumer coherence.
- If the first candidate lacks native authority, identify the precise
  producer/schema gap and keep 856 bounded to that selected gap.

Completion check:

- The selected row or first blocking authority gap is documented in `todo.md`
  for the next executor packet.
- No Raw-BIR/importer receiver work is started.

### Step 2 - Publish And Verify The Selected Native Authority

Goal: implement the smallest producer/schema/verifier change needed for the
one selected parameter row.

Actions:

- Add or tighten only the selected row's native authority publication.
- Verify missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent forms fail closed before printing or downstream use.
- Add focused positive and malformed same-feature coverage.
- Preserve existing accepted parameter rows and do not broaden generic
  parameter authority.

Completion check:

- Fresh build and focused producer/verifier proof pass.
- `git diff --check` passes.
- No Raw-BIR/importer receiver files are changed.

### Step 3 - Record The 734 Handoff

Goal: close this producer-side route with an exact return packet for 734.

Actions:

- Update the source idea with the selected row, tuple fields, consumer
  relation, malformed coverage, accepted proof, and implementation commit.
- Name the exact 734 return action for one bounded Raw-BIR receiver packet.
- Preserve that all nonselected parameter rows and non-parameter families
  remain out of scope and fail closed.

Completion check:

- The handoff is sufficient for plan-owner to reactivate 734 without
  re-deriving producer authority.
- The runbook is ready for close/reassessment.
