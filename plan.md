# Select-Publication Stack-Home Intent Support Plan

Status: Active
Source Idea: ideas/open/505_select_publication_stack_home_intent_support.md
Activated From: ideas/closed/504_select_publication_move_bundle_evidence_authority.md

## Purpose

Follow the residual evidence from idea 504 by repairing the lower
select-publication intent support needed before any final RV64 consumer can be
considered.

## Goal

Teach the select-publication intent layer to represent stack-source and
stack-destination publication moves explicitly, or preserve precise
fail-closed statuses when prepared facts are insufficient.

## Core Rule

Supported select-publication stack-home intent may be published only from
explicit prepared source/destination homes, stack layout, value identity, edge
identity, carrier, and move-bundle facts. RV64 materialization, raw shape,
source names, object output, final registers, and case-log absence are not
intent authority.

## Read First

- ideas/open/505_select_publication_stack_home_intent_support.md
- ideas/closed/504_select_publication_move_bundle_evidence_authority.md
- build/agent_state/504_step3_select_publication_consumer_classification/summary.md
- build/agent_state/504_step3_select_publication_consumer_classification/rows.tsv
- build/agent_state/504_step3_select_publication_consumer_classification/classification_counts.tsv

## Current Targets

- Inputs:
  - `src/builtin-constant.c`, classified as
    `intent_status_unsupported_source_home`;
  - `src/pr58726.c`, classified as
    `intent_status_unsupported_destination_home`;
  - current `EdgePublicationMoveIntent`, prepared homes, stack layout, edge
    publication, and move-bundle evidence surfaces.
- Outputs:
  - supported stack-source and stack-destination select-publication intent
    when facts are explicit; or
  - distinguishable fail-closed statuses when intent cannot be supported;
  - a later separate RV64 consumer idea only if supported intent proves a
    coherent lowerable family.

## Non-Goals

- Final RV64 select-publication materialization.
- Generic out-of-SSA immediate materialization for `src/pr37924.c`, now owned
  by idea 506.
- Reopening move-bundle materialization completed by ideas 501-503.
- Inferring homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 504 proved that two select-publication rows are real evidence rows but
are not consumer-ready because the intent layer reports unsupported stack-home
statuses. This plan should inspect and repair that intent surface before any
target materialization work is split.

## Execution Rules

- Audit intent construction before changing target lowering.
- Preserve unavailable statuses for missing, ambiguous, contradictory, or
  raw-shape-only evidence.
- Do not implement RV64 select-publication lowering in this idea.
- If supported intent creates a coherent consumer family, split that consumer
  into a new focused open idea.
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

### Step 1: Inspect Stack-Home Intent Construction

Inspect `EdgePublicationMoveIntent` construction for select-publication
stack-source and stack-destination rows.

Completion means `todo.md` records the exact helper surfaces, missing intent
fields, representative rows, and whether implementation can proceed.

### Step 2: Publish Supported Or Fail-Closed Stack-Home Intent

Publish supported stack-source/stack-destination intent when prepared facts are
explicit, or preserve precise unsupported statuses otherwise.

Completion means focused backend coverage proves available and fail-closed
intent statuses without RV64 materialization.

### Step 3: Split Any RV64 Select-Publication Consumer

If Step 2 proves a coherent lowerable select-publication consumer family,
create a separate RV64 materialization idea.

Completion means the consumer idea exists with evidence rows and reject
signals, or `todo.md` records why no consumer is ready.

### Step 4: Residual Disposition For Intent Support

Decide whether idea 505 can close after intent support and consumer splitting,
or whether a narrowed follow-up remains.

Completion means the lifecycle state is closed, switched, or extended with no
more than one active plan.
