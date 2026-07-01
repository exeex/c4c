# Prepared Move-Bundle Materialization Bucket Review Plan

Status: Active
Source Idea: ideas/open/495_prepared_move_bundle_materialization_bucket_review.md
Activated From: ideas/closed/500_semantic_global_static_gep_admission_producer.md

## Purpose

Resume the highest-priority open RV64 gcc_torture follow-up after the completed
BIR semantic GEP producer chain by reviewing the dominant
`unsupported_move_bundle_target_shape` bucket.

## Goal

Classify current move-bundle failures and split them into prepared/BIR
producer-authority gaps versus coherent RV64 materialization follow-ups.

## Core Rule

Move-bundle lowering may proceed only for coherent prepared bundle facts. Do
not reconstruct predecessor/successor identity, execution site, storage class,
value identity, or phi/join semantics in RV64 from raw BIR shape, testcase
names, or object output.

## Read First

- ideas/open/495_prepared_move_bundle_materialization_bucket_review.md
- ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
- docs/rv64_gcc_torture_post_contract/current_scan_summary.md
- docs/rv64_gcc_torture_post_contract/failure_bucket_map.md
- docs/rv64_gcc_torture_post_contract/followup_idea_plan.md
- build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv

## Current Targets

- Inputs:
  - current `unsupported_move_bundle_target_shape` rows from
    `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`;
  - prepared move-bundle, edge, phi, preservation, storage, and copy records
    available in the BIR/prealloc/RV64 publication surfaces.
- Outputs:
  - durable row counts, representative rows, and first-owner classifications;
  - one or more focused follow-up ideas for prepared/BIR producer-authority
    gaps;
  - one or more focused follow-up ideas for coherent RV64 move-bundle
    materialization families.

## Non-Goals

- Implementing move-bundle lowering inside this review idea.
- Reconstructing missing move authority, predecessor/successor identity,
  storage class, edge execution site, or value identity in RV64.
- F128-specific move bundles unless explicitly quarantined outside the priority
  route.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

The fresh RV64 gcc_torture scan ranked `unsupported_move_bundle_target_shape`
as the largest ordinary-C first-owner bucket. This idea should not implement
the bucket directly; it should determine which rows already have coherent
prepared move-bundle facts and which rows require producer/authority repair
before target lowering can consume them.

## Execution Rules

- Preserve RV64 gcc_torture as external evidence, not default CTest coverage.
- Prefer row-level evidence and representative shapes over aggregate counts
  alone.
- Split producer/prepared gaps before any RV64 lowering idea can claim those
  rows.
- Lifecycle-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
git diff --check
```

## Steps

### Step 1: Reproduce The Move-Bundle Bucket

Read the current RV64 gcc_torture summary and extract the
`unsupported_move_bundle_target_shape` row set.

Completion means a durable artifact records the row count, source cases,
functions when available, and enough representative rows for later owner
classification.

### Step 2: Classify Prepared Bundle Coherence

Classify representatives by event kind, authority, parallel-copy status,
execution site, destination storage, source storage, and move reason.

Completion means rows are separated into coherent RV64-lowerable bundles versus
missing, ambiguous, or contradictory prepared authority.

### Step 3: Split Producer And RV64 Follow-Ups

Create focused open ideas for producer/authority gaps and coherent RV64
materialization families.

Completion means each follow-up idea names its owning layer, evidence rows,
acceptance criteria, and concrete reviewer reject signals.

### Step 4: Residual Lifecycle Disposition

Decide whether idea 495 is complete after follow-up creation or whether another
review pass is needed for unresolved move-bundle subfamilies.

Completion means the active lifecycle state is closed, switched, or extended
with no more than one active plan.
