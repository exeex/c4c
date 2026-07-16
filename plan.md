# Remaining LIR Aggregate-Owner Rejection Decomposition Blocker Runbook

Status: Active
Source Idea: ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md
Activated from: rejected 831 Step 4 comparable-baseline gate

## Purpose

Decompose and repair only the demonstrated LIR aggregate-owner residual
contract(s) blocking 831's comparable full-suite gate.

## Core Rule

Do not merge structured-key, matching-owner, and no-owner compatibility
failures without a shared first-owner seam. Focused proof is not baseline
clearance.

## Read First

- `ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `ideas/closed/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
- `test_baseline.log` and `test_baseline.new.log`

## Non-Goals

- Baseline acceptance, 830/829 work, test or harness changes, tag fallbacks,
  or reopening closed successors without direct evidence.

## Ordered Steps

### Step 1 - Decompose the remaining aggregate-owner rejection families

Goal: identify first owners for the structured-key, matching-module-owner, and
no-owner compatibility residuals.

Actions:

- Trace representative failures for each observed contract group from the
  rejected comparable-gate inventory.
- Compare their construction and owner-validation paths with closed 834's
  accepted relation.
- Decide whether one native LIR repair is evidenced or ordered successors are
  required. Do not edit code in this packet.

Completion check: a first-owner ledger selects one bounded native route or
names separately scoped successors without testcase-shaped reasoning.

### Step 2 - Repair the evidenced native relation

Goal: restore only the owner-validation relation established by Step 1.

Actions:

- Implement the smallest owning-layer correction.
- Preserve malformed, foreign, wrong-namespace, and genuinely ownerless
  rejection; do not add a tag fallback.

Completion check: fresh build succeeds and the repair is contract-based, not
case-based.

### Step 3 - Prove the route and return to 831

Goal: establish focused multi-path evidence for the selected relation.

Actions:

- Run fresh build plus representative focused coverage and relevant negative
  guards.
- Record accepted proof and reactivate 831 at Step 4 only.

Completion check: accepted bounded proof preserves rejection contracts; no
baseline clearance is claimed.
