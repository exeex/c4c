# LIR Aggregate-Owner Module-Owner Canonicalization Blocker Runbook

Status: Active
Source Idea: ideas/open/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md
Activated from: 831 Step 3 provenance decision

## Purpose

Repair the native LIR aggregate key-to-module-owner lookup contract that
causes the shared 516-test diagnostic, without widening into closed 832/833
routes or accepting baseline state.

## Core Rule

Repair the canonical owner relation, not individual tests or diagnostic text.
Focused repair proof is not comparable full-suite clearance.

## Read First

- `ideas/open/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`

## Non-Goals

- Reopening closed 832 or 833, or changing 830/829 work.
- Test filtering, unsupported markers, expectation weakening, or harness work.
- Declaring a new full-suite baseline or returning 830.

## Ordered Steps

### Step 1 - Diagnose the aggregate key/module-tag ownership mismatch

Goal: establish the smallest native relation that loses the valid module owner.

Actions:

- Trace representative failing C and C++ cases through `lir_owned_type_spec`,
  structured-key construction, and `find_struct_def_tag_by_owner`.
- Compare a valid resolving aggregate path with a failing one; identify the
  exact owner/canonicalization invariant rather than test names.
- Select focused multi-suite coverage and an invalid/missing-owner guard.

Completion check: a bounded owner relation and implementation target are
recorded; no code or test-contract workaround is proposed.

### Step 2 - Repair the native owner canonicalization relation

Goal: make valid LIR aggregate keys resolve their matching module tag.

Actions:

- Implement the smallest ownership/provenance correction at the diagnosed
  lookup relation.
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
