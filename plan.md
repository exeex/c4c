# Select-Publication Move-Bundle Evidence And Authority Plan

Status: Active
Source Idea: ideas/open/504_select_publication_move_bundle_evidence_authority.md
Activated From: ideas/closed/503_rv64_before_return_prepared_move_materialization.md

## Purpose

Resolve the remaining select-publication move-bundle rows split out by idea
495 before any RV64 materialization treats them as lowerable.

## Goal

Publish or route enough select-publication move-bundle evidence to decide
whether the three ambiguous rows are coherent RV64-lowerable move bundles or
producer-authority gaps.

## Core Rule

Select-publication move bundles may become RV64-lowerable only after explicit
prepared/BIR evidence provides event kind, phase, authority, parallel-copy
status, execution site, destination storage, source storage when available, and
move reason. Absent case-log tokens, testcase names, raw BIR shape, object
output, and final registers are not authority.

## Read First

- ideas/open/504_select_publication_move_bundle_evidence_authority.md
- ideas/closed/495_prepared_move_bundle_materialization_bucket_review.md
- ideas/closed/501_rv64_before_instruction_prepared_move_materialization.md
- ideas/closed/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
- ideas/closed/503_rv64_before_return_prepared_move_materialization.md
- build/agent_state/495_step2_move_bundle_coherence/summary.md
- build/agent_state/495_step2_move_bundle_coherence/classification.tsv
- build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv
- build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv

## Current Targets

- Inputs:
  - three `select_publication_move_bundle` rows from idea 495 Step 2;
  - current prepared/BIR select-publication and move-bundle record streams;
  - current case-log event publication for move-bundle diagnostics.
- Outputs:
  - explicit evidence for event, phase, authority, parallel-copy status,
    execution site, destination storage, source storage when available, and
    move reason; or
  - distinguishable fail-closed statuses for missing, ambiguous,
    contradictory, or raw-shape-only select-publication authority;
  - a separate RV64 consumer idea if coherent lowerable select-publication
    materialization remains after the evidence packet.

## Non-Goals

- RV64 materialization of select-publication move bundles before authority is
  explicit.
- Before-instruction moves completed by idea 501.
- Out-of-SSA/pre-terminator moves completed by idea 502.
- Before-return moves completed by idea 503.
- Inferring authority from source names, case-log absence, raw BIR shape,
  object output, final registers, or target behavior.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 495 Step 2 found three select-publication rows whose current case-log
tokens omit the move-bundle authority details needed for a safe RV64 consumer.
This idea is a producer/evidence splitter first. It should publish or route the
missing evidence, then split any coherent lowerable family into a separate RV64
materialization idea.

## Execution Rules

- Audit producer records before proposing target lowering.
- Keep unavailable cases distinguishable instead of collapsing them into one
  generic unsupported status.
- Do not lower the three select-publication rows in RV64 during the evidence
  packet.
- If evidence proves a coherent consumer family, create a separate focused
  open idea for that RV64 materialization work.
- Code-changing proof:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

- Lifecycle-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
git diff --check
```

## Steps

### Step 1: Inspect Select-Publication Evidence Surfaces

Audit the prepared/BIR select-publication record stream and current
move-bundle diagnostic publication for the three ambiguous rows.

Completion means `todo.md` records the exact missing fields, producer surfaces,
representative rows, and whether this idea can publish evidence directly or
must route a lower prerequisite.

### Step 2: Publish Or Route Select-Publication Authority

Publish durable select-publication move-bundle evidence when underlying facts
exist, or record the precise lower producer gap when they do not.

Completion means backend coverage proves explicit available or fail-closed
evidence statuses without RV64 materialization.

### Step 3: Split Any Coherent RV64 Consumer

If Step 2 proves a coherent lowerable select-publication family, create a
separate focused RV64 materialization idea.

Completion means the new idea names its evidence rows, owner boundary,
acceptance criteria, and reviewer reject signals, or `todo.md` records why no
consumer idea is available yet.

### Step 4: Residual Disposition For Select-Publication Evidence

Decide whether idea 504 can close after evidence publication/routing and
consumer splitting, or whether the plan needs a narrowed follow-up step.

Completion means `todo.md` records the residual disposition and the active
lifecycle state is closed, switched, or extended with no more than one active
plan.
