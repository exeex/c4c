# LIR Cross-Function `LirValueId` Ownership Restoration Runbook

Status: Active
Source Idea: ideas/open/780_lir_cross_function_value_id_ownership_restoration.md
Activated from: 778's rejected full-suite baseline candidate after `b4685da80`.

## Goal

Restore correct shared cross-function native value-ID allocation and ownership
without weakening standalone cast-result authority or expanding into PHI or
generic expression migration.

## Core Rule

Follow the evidence from allocation through ownership registration and
verification. Do not add logical-only exemptions or change the verifier's
flag-gated malformed-authority contract.

## Read First

- `ideas/open/780_lir_cross_function_value_id_ownership_restoration.md`
- `ideas/open/778_lir_logical_rhs_result_authority_publication.md`
- `src/codegen/lir/` allocation, `LirFunction`, and verifier ownership paths
- `build/Testing/Temporary/LastTest.log` around `pr52129.c`

## Non-Goals

- no PHI, generic expression, ternary/coerce, vaarg, or other-family migration
- no verifier weakening, testcase-specific exception, or baseline edit
- no parent handoff work for 775

## Ordered Steps

### Step 1 - Diagnose cross-function native-ID ownership

Goal: establish the exact allocation, registration, and verification path that
makes a selected opt-in cast ID collide with or appear foreign in a separate
`LirFunction`.

Actions:

- trace `fresh_value` and native-ID ownership state through two functions;
- reduce the evidence to the smallest nearby multi-function reproduction and
  identify the shared seam that owns the repair;
- record why the defect cannot be fixed by a logical-only exception.

Completion check:

- a bounded repair target and proof case are identified without changing code,
  PHI, generic APIs, or verifier requirements.

### Step 2 - Repair the bounded shared ownership model

Goal: implement the smallest evidence-backed allocation/ownership repair.

Completion check:

- distinct functions allocate/register native IDs so selected opt-in casts are
  current-function-owned, while genuine malformed authority still fails closed.

### Step 3 - Prove nearby multi-function authority behavior

Goal: add focused positive and malformed coverage for the repaired shared model.

Completion check:

- nearby multi-function tests pass and do not rely on an external named case.

### Step 4 - Restore regression confidence and return to 778

Goal: prove non-regression and preserve the parent return point.

Completion check:

- fresh build, matching focused guard, and fresh full-suite candidate restore
  non-regression; then return 778 to baseline acceptance and Step 4.

## Proof

- Each code packet: fresh build plus its matching focused test subset.
- Before return: supervisor-owned fresh full-suite candidate; do not write or
  roll forward canonical root logs from this runbook.
