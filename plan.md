# LIR Truthiness-LHS Parameter Authority Completion Runbook

Status: Active
Source Idea: ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md
Activated from: 831 ordered successor after closed 832 HIR repair

## Purpose

Complete only the native authority relation missing from the selected
direct-scalar truthiness-LHS failures, after 832's HIR crash repair accepted
focused proof.

## Core Rule

Use native LIR identity, owner, parameter, and type facts. Do not recover
truthiness authority from text or weaken the verifier for named torture cases.

## Read First

- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `verify_truthiness_lhs_parameter_authority`
- `LirCmpOp.truthiness_lhs_parameter_authority`
- The 13-case exact subset command in 831

## Non-Goals

- HIR aggregate-owner work, 832 changes, direct-call identity, 830/829,
  Raw-BIR, generic compare/call rewrites, other truthiness forms, or ABI
  redesign.

## Ordered Steps

### Step 1 - Trace the direct-scalar truthiness-LHS authority seam

Goal: specify the smallest producer/verifier contract for the missing native
truthiness-LHS parameter authority in the 13 identified cases.

Actions:

- trace the direct-scalar truthiness LHS from construction to
  `verify_truthiness_lhs_parameter_authority`;
- identify existing native identity, owner, parameter, and type facts and the
  exact missing relation; and
- select one bounded producer/verifier seam, distinguishing missing, foreign,
  owner-incoherent, parameter-incoherent, and type-incoherent authority.

Completion check: the selected structural relation and fail-closed verifier
contract are explicit without text/signature/diagnostic recovery.

### Step 2 - Produce and verify the selected authority relation

Goal: implement the native relation at its narrow construction seam.

Actions:

- populate only the selected direct-scalar truthiness-LHS authority; and
- add nearby positive and malformed/foreign/incoherent authority coverage.

Completion check: the verifier accepts coherent native authority and rejects
malformed authority without a named-case exception.

### Step 3 - Prove the truthiness route and return to 831

Goal: provide accepted focused proof for the second ordered successor.

Actions:

- run a fresh build and supervisor-selected focused proof including the 13
  identified torture cases; and
- record accepted evidence, then reactivate 831 at Step 2 for proof collection.

Completion check: the cases no longer stop at the missing truthiness-authority
relation; no full-suite baseline clearance is claimed.
