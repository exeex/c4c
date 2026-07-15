# Frontend LIR Manual Switch Modelled-Result Authority Runbook

Status: Active
Source Idea: ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md
Switched from: 820 Step 3 (DirectScalar boundary checkpoint remains pending)

## Purpose

Remove the independent manual-switch fixture failure that prevents Idea 820's
Step 3 frontend proof from reaching its DirectScalar coverage.

## Core Rule

Repair only the selected manual `LirSwitch` fixture/modelled-result authority
seam. Preserve the existing fail-closed current-function integer selector
contract and do not broaden switch, DirectScalar, or Raw-BIR behavior.

## Read First

- `ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md`
- `ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md`
- the `frontend_lir_call_type_ref` manual switch fixture and the
  `LirSwitch.selector` verifier branch

## Non-Goals

- DirectScalar producer/verifier publication, Raw-BIR/importer work, generic
  switch support, or unrelated fixture repair.

## Ordered Steps

### Step 1 - Trace the manual switch authority seam

Goal: identify the fixture's intended current-function integer value definition
and the exact structured modelled-result field the selector verifier requires.

Actions:

- inspect the manually constructed switch fixture and its value-definition
  registration;
- confirm the failure is representational rather than a generic switch feature
  gap;
- stop and request a separate blocker if the smallest correction requires
  DirectScalar, Raw-BIR, or a generic switch redesign.

Completion check: one local fixture/modelled-result authority seam is named
without text-derived identity or a verifier relaxation.

### Step 2 - Correct and test the selected authority

Goal: publish the required structured selector authority while preserving
fail-closed malformed cases.

Actions:

- implement the minimal selected fixture/modelled-result correction;
- add nearby positive and missing, foreign, or type-incoherent authority
  coverage appropriate to the existing test surface;
- keep all unrelated switch and value-authority routes untouched.

Completion check: the selected fixture verifies and malformed selector
authority rejects under the existing contract.

### Step 3 - Prove the blocker and return to 820

Goal: establish that the frontend test no longer stops at the selector failure
and preserve the exact parent return point.

Actions:

- run a fresh build and focused `^frontend_lir_call_type_ref$` proof;
- run the supervisor-selected matching regression/broader proof;
- record that 820 resumes unchanged at Step 3 to rerun its own broader
  checkpoint and record only the selected 734 `LirBinOp.lhs` handoff.

Completion check: the focused test passes beyond the former selector abort and
the durable return record does not claim 820 Step 3 acceptance.
