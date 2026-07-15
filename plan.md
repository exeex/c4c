# LIR PHI Residual Producer-Family Authority Trace Runbook

Status: Active
Source Idea: ideas/open/806_lir_phi_residual_producer_family_authority_trace.md
Resumed from: 754 Step 4's 3036/3037 full-baseline gate; preserve 806 Steps 1--2.
Supersedes: 754 Step 4 until this separately scoped PHI producer-family blocker resolves.

## Purpose

Finish classification of the one named residual left by the failed 3036/3037
full gate, then repair only its evidenced producer handoff before retrying the
required full-baseline gate. Do not reopen accepted handoffs.

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

### Step 3 - Trace and classify the remaining 20060910-1.c producer seam

Goal: identify the immediate native producer/lowering handoff for the sole
remaining original residual without assuming that its PHI diagnostic matches
an accepted family.

Actions:

- Reproduce only `llvm_gcc_c_torture_src_20060910_1_c` and trace its failing
  PHI incoming backwards to the immediate producer/lowering handoff.
- Compare that native handoff with the accepted postfix old-value route and
  the closed 807 floating `fneg` and 808 scalar bit-not `xor` routes.
- Record one classification: shared accepted handoff (with concrete evidence),
  an in-scope unshared 806 producer family, or a prerequisite outside 806's
  stated producer-family scope. Do not change code in this classification
  step.

Completion check: the trace names the immediate producer/handoff and supports
one classification; a repair packet or separately scoped successor can be
selected without a generic residual sweep.

### Step 4 - Repair the classified in-scope producer handoff and prove the blocker

Goal: establish full-baseline acceptance before releasing the parent gate.

Actions:

- If Step 3 establishes an in-scope 806 producer handoff, make the smallest
  native-authority repair there and add nearby same-family positive and
  malformed-authority coverage. If it establishes an outside-scope
  prerequisite, stop this runbook and switch through a separately scoped
  successor with 806's resumption record.
- Obtain a fresh build and focused same-family proof before the full baseline.
- Run the supervisor-selected full baseline only after the focused repair is
  accepted.
- Require a supervisor-accepted 100% result; focused successor proof does not
  clear this parent gate.
- Record the accepted proof and reactivate 804 at unchanged Step 3 only after
  full-baseline acceptance.

Completion check: an in-scope repair has fresh focused proof and 100%
full-baseline acceptance is recorded; otherwise retain the executable Step 3
classification route or perform the explicit successor switch without
reopening completed producer work.
