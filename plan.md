# LIR Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/853_lir_next_body_parameter_authority_handoff.md

## Purpose

Advance the 734 no-omission queue after accepted Step 7.41 by selecting the
next native function-body parameter-use authority row outside Raw BIR.

## Goal

Trace current LIR parameter-use production after the accepted
fixed-direct-call argument-1 row, choose exactly one next receiver-relevant
semantic relation, and prepare the bounded producer/schema/verifier packet
that can later hand a typed tuple back to 734.

## Core Rule

Authority must come from native structured LIR facts. Do not infer parameter
identity, type, role, ownership, ABI, or consumer coherence from text, names,
rendered operands, diagnostics, signature strings, compatibility mirrors, or
testcase shape.

## Read First

- `ideas/open/853_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/829_lir_next_body_parameter_authority_handoff.md`
- Existing LIR body-parameter authority carriers and verifier checks
- Current `frontend_lir_call_type_ref` and nearby parameter-authority coverage

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.41 and all closed parameter
  handoffs through 829.
- Select one next valid function-body parameter-use row only if its native
  current-function value, owner, parameter index, type, ABI, role, and
  consumer relation can be represented and verifier-checked without recovery.
- Keep Raw-BIR containers, importer dispatch, Raw-BIR verifier, and receiver
  tests out of this producer route.

## Non-Goals

- Do not reopen accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, truthiness-comparison-LHS,
  fixed-direct-call argument-0, or fixed-direct-call argument-1 rows.
- Do not receive any Raw-BIR row.
- Do not admit generic parameter identity, ABI conversion, declaration-only
  authority, memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, or inline-assembly forms.

## Execution Rules

- Keep Step 1 read-only except for lifecycle trace notes in `todo.md`.
- If no receiver-ready candidate has native structured authority, record the
  exact missing producer prerequisite instead of shaping a testcase.
- Later implementation may change only the selected producer/schema/verifier
  seam and nearby focused tests.
- Run fresh build plus focused same-feature proof for implementation packets.

## Steps

### Step 1 - Trace The Next Body-Parameter Candidate

Goal: identify the next valid function-body parameter-use row after accepted
fixed-direct-call argument-1 receipt and decide whether it is producer-ready or
blocked by a narrower prerequisite.

Actions:

- Inventory current LIR body-parameter authority carriers and accepted
  receiver rows so the next candidate does not duplicate prior work.
- Trace current frontend/LIR production for remaining body-parameter uses and
  select the first bounded relation with native value/type/owner/ABI/role
  evidence.
- Record either the exact Step 2 producer/verifier packet or the exact
  separate prerequisite if the candidate still lacks structured authority.

Completion check:

- `todo.md` names the selected row or blocker, its evidence, rejected
  presentation-derived alternatives, and the next executable packet.
