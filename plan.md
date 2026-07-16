# LIR Scalar LHS Parameter Authority Baseline Repair Runbook

Status: Active
Source Idea: ideas/open/861_lir_scalar_lhs_parameter_authority_baseline_repair.md
Switched From: ideas/open/860_lir_next_body_parameter_authority_handoff.md after baseline failure blocked Step 1

## Purpose

Unblock idea 860 by repairing the current-LIR scalar LHS parameter authority
baseline failure before any next-row producer handoff is attempted.

## Goal

Repair the `LirBinOp.scalar_lhs_parameter_authority` verifier/producer failure
family so valid nonselected scalar RHS forms no longer fail the full-suite
baseline.

## Core Rule

Keep accepted DirectScalar authority contracts fail-closed while accepting the
valid nonselected RHS forms required by selected floating binary LHS authority.
Do not weaken contracts, downgrade expectations, or special-case failing tests.

## Read First

- `ideas/open/861_lir_scalar_lhs_parameter_authority_baseline_repair.md`
- Resumption record in `ideas/open/860_lir_next_body_parameter_authority_handoff.md`
- Current LIR scalar LHS parameter authority producer and verifier code
- Existing focused tests around DirectScalar binary parameter authority
- Canonical baseline logs named by the supervisor

## Current Targets And Scope

- Diagnose the selected floating binary LHS authority path and its RHS
  coherence rules.
- Repair the verifier/producer behavior for valid nonselected scalar RHS
  forms.
- Add nearby focused positive and malformed coverage for this failure family.
- Produce fresh proof that the baseline no longer blocks 860.

## Non-Goals

- Do not select or publish any new 860 body-parameter-use row.
- Do not edit Raw-BIR receiver/importer/container/backend surfaces unless the
  investigation proves they are necessary; if necessary, stop and route a
  separate scoped idea.
- Do not weaken DirectScalar authority contracts or expectation coverage.
- Do not absorb broad LIR schema, ABI, target-lowering, or unrelated authority
  family work.

## Execution Rules

- Treat the listed full-suite failures as symptoms, not as a named-case patch
  list.
- Keep malformed authority rejected before downstream use.
- Preserve accepted DirectScalar rows while repairing valid nonselected RHS
  handling.
- For code changes, run a fresh build, focused scalar authority proof,
  `git diff --check`, and a supervisor-accepted full-suite baseline proof.

## Steps

### Step 1 - Repair scalar LHS parameter authority baseline

Goal: diagnose and repair the current-LIR scalar LHS parameter authority
failure family that blocks the full-suite baseline.

Actions:

- Reproduce or inspect the failure around
  `LirBinOp.scalar_lhs_parameter_authority`.
- Identify why selected floating binary LHS authority currently requires an
  invalid selected scalar RHS instead of accepting valid nonselected scalar RHS
  forms.
- Repair only the necessary producer/verifier contract.
- Add focused positive coverage for the valid nonselected RHS form and
  malformed coverage that proves the accepted DirectScalar contract remains
  fail-closed.
- Preserve evidence for 860 resumption when this blocker closes.

Completion check:

- Fresh build passes.
- Focused scalar LHS authority proof passes.
- Fresh full-suite baseline proof is supervisor-accepted.
- `git diff --check` passes.
- No 860 next-row selection or Raw-BIR receiver work lands in this idea.
