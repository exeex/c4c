# LIR PHI Residual Producer-Family Authority Trace Runbook

Status: Active
Source Idea: ideas/open/806_lir_phi_residual_producer_family_authority_trace.md
Supersedes: 804 Step 3 full-baseline gate until this separately scoped blocker resolves.

## Purpose

Classify the four remaining PHI incoming authority failures without assuming
they share 804's accepted scalar unary-minus seam.

## Core Rule

Native checked current-function IDs are authority. A shared diagnostic or test
name is not producer-path evidence; do not weaken the existing PHI contract.

## Read First

- `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`
- `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`
- `test_after.log` failure contexts for `pr50310.c`, `20000715-1.c`,
  `20060910-1.c`, and `pr68376-2.c`
- accepted 804 handoff commit `308fff39c`

## Non-Goals

- Repeating 804's unary-minus route or reopening CFG/PHI verifier semantics.
- Broad residual producer conversion, generic provenance, text recovery, or
  testcase-specific behavior.

## Ordered Steps

### Step 1 - Trace and classify the four residual PHI incoming paths

Goal: establish the native producer/immediate-lowering handoff for every
observed failure and determine whether any cases genuinely share one family.

Actions:

- Reproduce each named test narrowly and trace PHI incoming construction back
  to its producer result authority.
- Compare the path with 804's accepted unary-minus seam using native IDs and
  lowering ownership, not diagnostic text.
- If a case requires a different family outside this source's selected route,
  record a separate blocker instead of broadening this plan.

Completion check: an evidence-backed family map exists for all four cases and
one bounded repair target is selected, or separately scoped successors own the
unshared cases.

### Step 2 - Repair one evidenced producer handoff

Goal: publish the checked native ID for the selected family while preserving
the existing PHI verifier contract.

Actions:

- Implement only the selected producer or immediate-lowering repair.
- Add same-family positive and malformed-authority coverage.
- Do not touch CFG/PHI schema, predecessor/edge semantics, or use display text
  as authority.

Completion check: selected valid inputs carry a current-function ID and
missing, unknown, foreign, and stale forms reject under focused proof.

### Step 3 - Prove the blocker and return to 804

Goal: establish full-baseline acceptance before releasing the parent gate.

Actions:

- Obtain a fresh build and focused same-feature proof.
- Have the supervisor run and accept the required 100% full baseline.
- Record accepted proof and reactivate 804 at unchanged Step 3.

Completion check: 100% full baseline acceptance is recorded; otherwise retain
an executable repair or separately scoped successor route.
