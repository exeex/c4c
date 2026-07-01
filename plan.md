# RV64 Select-Publication Stack-Home Materialization Plan

Status: Active
Source Idea: ideas/open/507_rv64_select_publication_stack_home_materialization.md
Activated From: ideas/closed/505_select_publication_stack_home_intent_support.md

## Purpose

Consume the structured select-publication stack-home intent introduced by idea
505 in a separate final RV64 materialization packet.

## Goal

Materialize supported select-publication stack-home intent in RV64 without
reconstructing authority from raw shape or folding in unrelated immediate
materialization work.

## Core Rule

Final select-publication materialization may only consume supported
`EdgePublicationMoveIntent` stack-home fields. Testcase names, raw BIR shape,
object output, final registers, target behavior, and absent diagnostics are not
authority.

## Read First

- ideas/open/507_rv64_select_publication_stack_home_materialization.md
- ideas/closed/505_select_publication_stack_home_intent_support.md
- build/agent_state/505_step2_stack_home_intent_support/summary.md
- build/agent_state/505_step1_stack_home_intent_construction/rows.tsv
- build/agent_state/504_step3_select_publication_consumer_classification/rows.tsv

## Current Targets

- Inputs:
  - pointer/XLEN concrete stack-source to GPR destination intent from
    `src/builtin-constant.c`;
  - direct GPR source to 1/2/4-byte concrete stack-destination intent from
    `src/pr58726.c`;
  - current RV64 select-publication admission predicate and object emission
    surfaces.
- Outputs:
  - final RV64 materialization for supported pointer stack-source to GPR intent;
  - final RV64 materialization for supported GPR to stack-destination intent;
  - preserved fail-closed behavior for unsupported widths, offsets,
    stack-to-stack shapes, missing concrete fields, and coordinate confusion.

## Non-Goals

- Generic out-of-SSA `phi_join_immediate_materialization`, owned by idea 506.
- Extending intent support beyond idea 505 unless a separate producer gap is
  proven and routed.
- Stack-to-stack select-publication materialization without an explicit scratch
  and interleaving policy.
- Reopening move-bundle materialization completed by ideas 501-503.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 505 made the two select-publication stack-home rows structurally visible
as supported intent while intentionally keeping final RV64 admission
fail-closed. This plan should only flip final materialization for the supported
intent shapes and keep adjacent unsupported shapes closed.

## Execution Rules

- Consume `EdgePublicationMoveIntent` fields, not case-log text.
- Keep stack-to-stack select-publication unsupported without a dedicated policy.
- Keep large offsets and unsupported stack widths fail-closed unless a general
  address/materialization policy is added in a separate idea.
- Do not touch idea 506's generic immediate route.
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

### Step 1: Inspect Final Admission Gate

Inspect the final select-publication RV64 admission predicate and emission path
for the supported stack-home intent fields.

Completion means `todo.md` records the exact predicate/helper surfaces and
whether implementation can proceed without new producer facts.

### Step 2: Materialize Pointer Stack-Source To GPR

Implement final RV64 materialization for supported pointer/XLEN concrete
stack-source to GPR destination intent.

Completion means focused backend coverage proves the available path and
preserves fail-closed unsupported source-home boundaries.

### Step 3: Materialize GPR To Stack Destination

Implement final RV64 materialization for supported direct GPR source to
1/2/4-byte concrete stack-destination intent.

Completion means focused backend coverage proves byte/halfword/word stores and
preserves fail-closed unsupported destination-home boundaries.

### Step 4: Residual Disposition For Select-Publication Consumer

Decide whether idea 507 can close after the supported final consumer paths are
materialized or whether a narrowed follow-up remains.

Completion means the lifecycle state is closed, switched, or extended with no
more than one active plan.
