# LIR Aggregate-Owner Module-Owner Canonicalization Blocker Runbook

Status: Active
Source Idea: ideas/open/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md
Resumed from: accepted 835 durable-owner carrier prerequisite

## Purpose

Repair the native LIR aggregate key-to-module-owner lookup contract that
causes the shared 516-test diagnostic, without widening into closed 832/833
routes or accepting baseline state.

## Core Rule

Consume 835's durable HIR-owned canonical identity to repair the owner
relation, not individual tests or diagnostic text. Focused repair proof is not
comparable full-suite clearance.

## Read First

- `ideas/open/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
- `ideas/closed/835_hir_durable_aggregate_owner_identity_carrier_prerequisite.md`
- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`

## Non-Goals

- Reopening closed 832 or 833, or changing 830/829 work.
- Test filtering, unsupported markers, expectation weakening, or harness work.
- Declaring a new full-suite baseline or returning 830.

## Ordered Steps

### Step 1 - Diagnose the aggregate key/module-tag ownership mismatch (complete)

Accepted result: cleanup-before-owner lookup loses parser-backed declaration
and namespace canonicalization, while pre-cleanup lookup can dereference stale
parser storage. The durable HIR carrier required for the safe repair is now
accepted in 835 commit `932c3339b`.

Completion check: complete; do not repeat the diagnosis.

### Step 2 - Repair the native owner canonicalization relation

Goal: make valid LIR aggregate keys resolve their matching module tag using
835's HIR-owned durable identity.

Actions:

- Implement the smallest ownership/provenance correction at the diagnosed
  `lir_owned_type_spec` lookup relation.
- Consume the durable carrier rather than parser-backed record/qualifier data.
- Preserve legitimate missing-owner rejection and avoid broad metadata or HIR
  rewrites.

Completion check: a fresh build succeeds and the native relation, not a
testcase-shaped exception, owns the repair.

### Step 3 - Prove representative same-feature behavior

Goal: verify repaired aggregate owner lookup across affected paths.

Actions:

- Run focused coverage from at least two affected suite categories and the
  selected malformed/missing-owner guard.
- Confirm no expectation, filtering, or diagnostic-text workaround hides the
  previous failure.

Completion check: fresh build plus focused multi-suite and negative-contract
proof are accepted by the supervisor.

### Step 4 - Hand evidence back to 831

Goal: preserve the bounded acceptance record and resume the parent correctly.

Actions:

- Record accepted implementation/proof references in the source resumption
  record.
- Switch back to 831 Step 4 only; 831 owns the comparable full-suite gate.

Completion check: lifecycle state unambiguously points to 831 Step 4, with no
baseline-clearance or 830-return claim in this blocker.
