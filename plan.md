# Direct-Call Structured Argument Identity Prerequisite Runbook

Status: Active
Source Idea: ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md

## Purpose

Finish the bounded native LIR prerequisite that gives argument 1 of one valid
direct, non-variadic, specified two-parameter call a structured identity and
type relation for later 829 authority validation.

## Goal

Accept the repaired comparable-baseline gate for the already-landed Step 2
implementation, then run focused completion proof and return 829 to Step 2.

## Core Rule

Do not publish 829 body-parameter authority here. This route only proves the
native direct-call argument-1 identity/type prerequisite and records whether it
is sufficient for 829 to validate its selected tuple.

## Read First

- `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`
- `ideas/closed/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- Step 2 implementation commit `f0fc85e4f`
- Current direct-call argument construction and LIR verifier paths

## Current Targets And Scope

- Preserve accepted Steps 1 and 2 from commit `f0fc85e4f`.
- Accept Step 3 using closed 831's 3038/3038 comparable full-suite gate.
- Run Step 4 focused proof for the bounded direct-call argument-1 relation.
- Return only to 829 Step 2; do not perform 829 authority publication.

## Non-Goals

- Do not edit Raw-BIR, importer, receiver, generic call, other argument
  indices, indirect/variadic/unspecified calls, ABI conversion, or parser
  recovery.
- Do not publish `LirCurrentFunctionBodyParameterDefinition` authority or
  `FixedDirectCallArgument1` role.
- Do not reopen the closed baseline-decomposition route unless current proof
  regresses.

## Working Model

Step 2 already produced the native relation. The remaining work is acceptance:
first consume the 831 baseline clearance, then prove the specific producer and
verifier path still works on the current tree.

## Execution Rules

- Keep Step 3 lifecycle/proof-only; no 830 code edits.
- For Step 4, run a fresh build plus focused `frontend_lir_call_type_ref`
  proof unless current evidence requires a broader same-feature subset.
- Closure must name the exact relation handed back to 829 and must not claim
  body-parameter authority publication.

## Steps

### Step 3 - Accept The Repaired Comparable Baseline Gate

Goal: accept 831's completed baseline blocker as the missing post-Step-2 gate.

Actions:

- Confirm closed 831 records an accepted full-suite 3038/3038 comparable
  baseline.
- Record that no 830 code change is needed for Step 3.
- Advance to Step 4 focused completion.

Completion check:

- `todo.md` records the accepted baseline gate and points to Step 4.

### Step 4 - Prove Direct-Call Argument-1 Completion And Return 829

Goal: prove the bounded native argument-1 relation and close 830.

Actions:

- Run a fresh build.
- Run focused `frontend_lir_call_type_ref` proof.
- Confirm malformed/missing/incoherent direct-call argument relation coverage
  remains nearby.
- Record the exact produced relation and whether it is sufficient for 829's
  selected parameter-definition tuple.

Completion check:

- Supervisor accepts focused proof and lifecycle can close 830, returning
  `ideas/open/829_lir_next_body_parameter_authority_handoff.md` to Step 2.
