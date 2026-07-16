# Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/829_lir_next_body_parameter_authority_handoff.md

## Purpose

Publish exactly one next function-body parameter-use authority row for later
734 Raw-BIR receipt, now that closed 830 established the missing direct-call
argument-1 structured identity/type prerequisite.

## Goal

Add native LIR producer/schema/verifier authority for the selected
DirectScalar current-function parameter used as argument 1 of a direct,
non-variadic, specified call, then hand off the exact tuple to 734.

## Core Rule

Do not recover parameter identity, type, role, or call coherence from text,
names, signatures, rendered operands, diagnostics, or compatibility fields.
The row must be carried by native structured facts and fail closed for
malformed authority.

## Read First

- `ideas/open/829_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/830_lir_direct_call_structured_argument_identity_prerequisite.md`
- Step 1 trace commit `78b17b3d3`
- Current LIR call argument authority structs, direct-call lowering, and
  verifier checks

## Current Targets And Scope

- Preserve accepted Step 1 trace: selected relation is
  `LirCallOp.structured_args[1]` for a direct, non-variadic, specified call.
- Consume closed 830's argument-1 identity/type relation only as a prerequisite.
- Publish exactly one `FixedDirectCallArgument1` DirectScalar body-parameter
  authority row, with owner/index/type/ABI/role and consumer coherence checks.
- Add focused positive and malformed-authority coverage.

## Non-Goals

- Do not edit Raw-BIR, importer, receiver, 734 receipt, generic call handling,
  other argument indices, indirect/variadic/unspecified calls, or ABI
  conversion.
- Do not reopen accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, truthiness-comparison-LHS, or
  fixed-direct-call-argument-0 rows.
- Do not publish multiple parameter rows or generic parameter admission.

## Working Model

The verifier should accept the row only when the selected structured call
argument and current-function parameter definition agree on value, owner,
parameter index, type, DirectScalar ABI, explicit argument-1 role, and callee
parameter-1 coherence. Missing, invalid, duplicate, foreign, mismatched, or
consumer-incoherent forms must reject before publication.

## Execution Rules

- Keep implementation bounded to one producer/schema/verifier seam and nearby
  tests.
- Preserve all nonselected parameter forms as fail closed.
- Run a fresh build plus focused `frontend_lir_call_type_ref` proof.
- Use matching before/after focused regression logs if accepting code changes.

## Steps

### Step 2 - Publish And Verify The Selected Authority

Goal: publish one DirectScalar fixed-direct-call argument-1 body-parameter
authority row and prove malformed forms fail closed.

Actions:

- Revalidate that closed 830's argument-1 identity/type relation carries the
  selected current-function parameter definition without recovery.
- Add or extend the native authority carrier for argument-1 role.
- Populate it only for the selected direct, non-variadic, specified call
  argument-1 relation.
- Verify missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority rejection.
- Add focused positive and malformed coverage.

Completion check:

- Fresh build and focused `frontend_lir_call_type_ref` proof pass, matching
  focused regression guard is non-regressing, and the completion record names
  the exact tuple handed to 734.
