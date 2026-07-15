# LIR PHI Incoming Producer Authority Repair Runbook

Status: Active
Source Idea: ideas/open/804_lir_phi_incoming_producer_authority_repair.md
Resumed from: accepted 806 residual producer-family chain and its 3037/3037
full-baseline gate; preserve 804 Steps 1--2.

## Purpose

Complete 804's recorded return step, then restore 754's full-baseline gate
without repeating accepted PHI producer work.

## Core Rule

Native checked current-function IDs are authority. Do not use display text,
instruction order, or testcase identity, and do not weaken PHI verification.

## Read First

- `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`
- `ideas/closed/806_lir_phi_residual_producer_family_authority_trace.md`
- accepted 804 handoff commit `308fff39c`
- accepted residual-chain commits `961ce9fda`, `8f31e2535`, `b86df3b9d`, and
  `4d29f7b3e`

## Non-Goals

- Repeating 804's scalar unary-minus route or reopening CFG/PHI verifier,
  predecessor, or edge semantics.
- Residual producer conversion, generic provenance, text recovery, or
  testcase-specific behavior.

## Ordered Steps

## Completed Steps

### Step 1 - Trace the failing PHI incoming producer handoff

Completed: identified the scalar integer unary-minus ternary-else handoff.

### Step 2 - Repair only the selected producer-side handoff

Completed in `308fff39c`: publish the scalar unary-minus `sub` result through
`fresh_value(ctx)` with nearby positive and malformed-authority coverage.

## Ordered Steps

### Step 3 - Prove the blocker and return it to 754

Goal: record the accepted PHI producer-blocker chain and restore the parent
baseline gate without re-opening completed work.

Actions:

- Preserve the accepted 804 scalar unary-minus evidence and the closed 806
  successor chain.
- Record the supervisor-accepted 3037/3037 full baseline as satisfying the
  bounded blocker gate.
- Reactivate 754 at its unchanged Step 2 only after the supervisor accepts
  this return decision.

Completion check: the accepted full baseline and parent return point are
recorded; 754 can resume unchanged Step 2 without repeating 804 Steps 1--2.
