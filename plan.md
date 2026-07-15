# LIR PHI Producer Helper Result Identity Reassessment Runbook

Status: Active
Source Idea: ideas/open/775_lir_phi_producer_helper_result_identity.md
Activated from: accepted 781 ternary-arm handoff and 775's preserved
post-781 reassessment return point.

## Purpose

Determine whether the accepted vaarg, logical-RHS, and selected ternary-arm
producer facts leave an executable, bounded route for 775's three-family
helper-result contract.

## Goal

Establish the first remaining native producer loss, or confirm that the
remaining 775 work can proceed without a generic expression API migration or
PHI-carrier work.

## Core Rule

Treat native `LirValueId` authority as semantic data before rendering. Do not
recover it from result spelling, labels, printed output, instruction order, or
testcase identity.

## Read First

- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `ideas/closed/777_lir_vaarg_result_authority_publication.md`
- `ideas/closed/778_lir_logical_rhs_result_authority_publication.md`
- `ideas/closed/781_lir_ternary_coerce_arm_result_authority_publication.md`

## Non-Goals

- no implementation or test changes during the reassessment gate
- no generic `emit_rval_*` or `coerce` API migration
- no other expression family, PHI carrier/verifier, 751, Raw-BIR/importer,
  backend, target-lowering, MIR, emission, or baseline-log work
- no side tables, synthetic values, or text-derived identity recovery

## Execution Rules

1. Compare each accepted handoff only to the corresponding 775 acceptance
   criterion; do not treat a selected producer fact as full chain coverage.
2. Identify a concrete first remaining native-authority loss before proposing
   code work.
3. If the remaining route needs a broad API migration or PHI-carrier work,
   stop and return the exact bounded successor scope for plan-owner lifecycle
   repair; do not absorb it into 775.
4. Preserve 751's blocked return point until 775 has an accepted complete typed
   handoff for all named producer families.

## Ordered Steps

### Step 1 - Reassess the accepted producer handoffs

Goal: determine whether 777, 778, and 781 together leave an executable,
bounded native producer contract for 775.

Actions:

- inventory the vaarg, logical, and ternary facts against 775's acceptance
  criteria, including what each fact expressly leaves raw;
- identify the first remaining producer authority loss, if any, without
  inferring identity from compatibility spelling; and
- classify the result as an in-scope bounded 775 implementation route or a
  separately scoped blocker.

Completion check:

- the next lifecycle action names either one bounded native 775 route with its
  exact first loss and proof boundary, or one separately scoped successor; no
  PHI/751/Raw-BIR claim is made.

### Step 2 - Repair the lifecycle route from the reassessment result

Goal: keep the active lifecycle executable without broadening 775.

Actions:

- if Step 1 identifies a bounded in-scope producer route, request a runbook
  repair that names only that route and its focused proof; or
- if Step 1 identifies an out-of-scope first loss, have plan-owner create and
  activate the named successor while preserving 775's return point.

Completion check:

- no exhausted reassessment route remains active without either an executable
  repaired 775 runbook or a named active successor.

## Proof

- This reassessment gate is documentation/lifecycle analysis; do not generate
  code proof. Any later code packet must select fresh focused build and test
  proof before acceptance.
