# Pre-existing Baseline Failure-Family Decomposition Blocker Runbook

Status: Active
Source Idea: ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md
Switched from: 830 Step 3 rejected-baseline gate

## Purpose

Keep 830's accepted direct-call argument-1 slice parked while independently
pre-existing baseline failures are assigned to their first owning layer.

## Core Rule

Treat the HIR segfault and truthiness-parameter verifier failures as separate
implementation families unless concrete evidence proves one shared first-owner
seam. Do not modify 830 while this blocker is active.

## Read First

- `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`
- `test_baseline.log` and `test_baseline.new.log`
- `f0fc85e4f^` provenance evidence supplied with this switch

## Non-Goals

- Direct-call argument identity/type production, 829 authority publication,
  Raw-BIR, generic calls, or any repair by expectation/harness weakening.
- A mixed implementation patch for the two failure families.

## Ordered Steps

### Step 1 - Establish first-owner decomposition from exact reproductions

Goal: preserve the supplied pre-830 provenance and determine whether the HIR
segfault and truthiness-parameter verifier failures can share an honest repair
owner.

Actions:

- run the exact 14-failure subset and inspect only the first owning
  failure/provenance seam for each family;
- confirm the HIR segfault independently on clean `f0fc85e4f^` with backend
  enabled and confirm the torture diagnostics use the unchanged truthiness
  verifier route; and
- if the owners differ, have lifecycle create ordered, separately scoped repair
  successors before any implementation. If a shared seam is evidenced, record
  that seam and narrow the next packet to it.

Completion check: each family has an evidenced first owner and either a proven
shared bounded repair seam or named separate repair successors.

### Step 2 - Repair the selected owner route(s) without contract weakening

Goal: obtain accepted focused proof for each separately scoped owner route.

Actions:

- execute only the successor route(s) authorized by Step 1; and
- preserve test/harness contracts and return their accepted proof to this
  blocker.

Completion check: every failure family has accepted focused owner proof; no
mixed or testcase-shaped baseline workaround was used.

### Step 3 - Obtain comparable baseline proof and return 830

Goal: clear the rejected baseline gate without claiming 830 capability work.

Actions:

- obtain supervisor-owned comparable full-suite evidence against the accepted
  3038/3038 baseline; and
- record clearance, then reactivate 830 at unchanged Step 3 solely to accept
  that gate, followed by its existing Step 4 and 829 Step 2 return.

Completion check: accepted comparable proof shows no new baseline problem and
the parent return point is durable.
