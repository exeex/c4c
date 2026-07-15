# LIR PHI Residual Producer-Family Authority Trace Runbook

Status: Active
Source Idea: ideas/open/806_lir_phi_residual_producer_family_authority_trace.md
Resumed from: 754 Step 4's 3036/3037 full-baseline gate; preserve 806 Steps 1--2.
Supersedes: 754 Step 4 until this separately scoped PHI producer-family blocker resolves.

## Purpose

Complete the required full-baseline gate after the bounded residual PHI
producer-family repairs; do not reopen their accepted handoffs.

## Core Rule

Native checked current-function IDs are authority. A green focused successor
does not clear the parent: only a supervisor-accepted 100% full baseline can
return 804, then 754 through the recorded parent chain.

## Read First

- `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`
- `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`
- closed 807 floating `fneg` and 808 scalar bit-not `xor` authority records
- accepted commits `961ce9fda`, `8f31e2535`, and `b86df3b9d`
- the supervisor-selected fresh full-baseline procedure

## Non-Goals

- Repeating accepted postfix, floating `fneg`, or scalar bit-not `xor` work;
  reopening 804's unary-minus route; or changing PHI/verifier, CFG,
  predecessor, or edge semantics.
- Generic provenance, rendered-text recovery, expectation downgrades, or
  declaring 804/754 clear from a partial baseline.

## Completed Steps

### Step 1 - Trace and classify the four residual PHI incoming paths

Completed: evidence separated residual producer families and scoped 807/808
successors rather than broadening 806.

### Step 2 - Repair one evidenced producer handoff

Completed in `961ce9fda`: only the postfix-increment old-value handoff was
repaired with focused same-family authority coverage.

## Ordered Steps

### Step 3 - Prove the blocker and return to 804

Goal: establish full-baseline acceptance before releasing the parent gate.

Actions:

- Obtain a fresh build and run the supervisor-selected full baseline.
- Require a supervisor-accepted 100% result; focused successor proof does not
  clear this parent gate.
- Record the accepted proof and reactivate 804 at unchanged Step 3 only after
  full-baseline acceptance.

Completion check: 100% full-baseline acceptance is recorded; otherwise retain
this executable Step 3 route without reopening completed producer work.
