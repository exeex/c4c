# Pre-existing Baseline Failure-Family Decomposition Blocker Runbook

Status: Active
Source Idea: ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md
Resumed from: closed 833 capability-complete successor

## Purpose

Collect the accepted independent repair evidence, preserve their distinct
ownership, and then establish comparable full-suite baseline status before
returning 830 to its unchanged acceptance gate.

## Core Rule

Keep the HIR aggregate-owner and truthiness-LHS authority families separate.
Focused successor proof is not comparable full-suite or baseline clearance.

## Read First

- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `ideas/closed/832_hir_aggregate_owner_function_parameter_crash_repair.md`
- `ideas/closed/833_lir_truthiness_lhs_parameter_authority_completion.md`

## Non-Goals

- Any 830 direct-call, 829 authority, Raw-BIR, generic-call, test-contract, or
  harness change.
- Claiming baseline clearance from either focused successor proof.

## Ordered Steps

### Step 1 - Establish first-owner decomposition from exact reproductions

Status: Complete.

Completion record: `ba7958ee4` established separate first-owner seams and the
ordered 832 then 833 repair routes. No shared implementation seam exists.

### Step 2 - Collect accepted successor evidence and confirm family boundaries

Goal: record 832 and 833 accepted focused proof under this blocker without
merging their scope or treating either result as baseline clearance.

Actions:

- collect the accepted 832 HIR aggregate-owner proof and 833 truthiness-LHS
  authority proof, including commits and before/after results;
- preserve the residual four aggregate-owner-family failures as visible
  evidence outside 833; and
- confirm the two completed successor routes satisfy their focused contracts
  without mixed implementation or weakened test/harness contracts.

Completion check: both successor proofs and their ownership boundaries are
durably recorded; the remaining action is only Step 3 comparable full-suite
proof.

### Step 3 - Classify the rejected comparable full-suite gate

Goal: determine the first owner of the rejected full-suite delta without
changing baseline state or silently widening either completed successor.

Actions:

- preserve the accepted `test_baseline.log` (3038/3038 at `8418036b`) and the
  canonical rejected `test_after.log` (2520 passed / 518 failed after a
  successful fresh build) without replacement or acceptance;
- compare their inventories, group the 516 newly failing tests by suite
  category and earliest common owner, and explicitly distinguish the 831 HIR
  aggregate-owner and truthiness-LHS families from unrelated failures; and
- decide the route from that ledger: make an in-scope repair only if the
  evidence identifies an uncompleted 831-owned family; otherwise create a
  separately scoped blocker and switch before further work.

Completion check: a bounded provenance ledger names the first owner and an
executable route.  No fresh baseline is claimed, no 830 return occurs, and the
closed 832/833 contracts remain unchanged unless direct evidence links a
specific failure to them.

### Step 4 - Obtain comparable baseline proof and return 830

Goal: clear the rejected 3038/3038 baseline gate with supervisor-owned,
comparable full-suite evidence.

Actions:

- after Step 3's owner route has been resolved and accepted, run the
  supervisor-selected comparable full suite against the accepted baseline; and
- if accepted, record the result and reactivate 830 at unchanged Step 3 only.

Completion check: accepted comparable proof shows no new baseline problem; no
capability work is attributed to 830 by this blocker.
