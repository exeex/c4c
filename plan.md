# RV64 Before-Return Prepared Move Materialization Plan

Status: Active
Source Idea: ideas/open/503_rv64_before_return_prepared_move_materialization.md
Activated From: ideas/closed/502_rv64_out_of_ssa_parallel_copy_move_materialization.md

## Purpose

Implement the isolated coherent before-return move-bundle follow-up split out
by idea 495 without broadening return lowering or ABI behavior.

## Goal

Materialize the coherent prepared before-return stack-to-return-register move
in RV64 from explicit prepared facts.

## Core Rule

RV64 may lower the before-return move only when prepared move-bundle facts
provide the return-adjacent execution point, source storage, destination
register, move reason, and value homes. Testcase shape, source names, raw BIR
order, final homes, target register conventions, and object output are not
return move authority.

## Read First

- ideas/open/503_rv64_before_return_prepared_move_materialization.md
- ideas/closed/495_prepared_move_bundle_materialization_bucket_review.md
- ideas/closed/501_rv64_before_instruction_prepared_move_materialization.md
- ideas/closed/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
- build/agent_state/495_step2_move_bundle_coherence/summary.md
- build/agent_state/495_step2_move_bundle_coherence/classification.tsv
- build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv
- build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv

## Current Targets

- Inputs:
  - one coherent prepared before-return row from idea 495 Step 2;
  - prepared move-bundle facts with `event_kind=pre_terminator_copies`,
    `phase=before_return`, `authority=none`, `parallel_copy=no`, destination
    `register`, and move reason `return_stack_to_register`.
- Outputs:
  - RV64 materialization for the prepared stack-to-return-register move;
  - focused backend coverage for the representative row and fail-closed
    incomplete-authority boundaries.

## Non-Goals

- General return lowering, call ABI rewrites, or stack-frame redesign.
- Before-instruction moves already completed by idea 501.
- Out-of-SSA/pre-terminator moves already completed by idea 502.
- Select-publication evidence or authority repair.
- Inferring return storage from testcase shape, source names, raw BIR order, or
  target register conventions without prepared facts.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 495 Step 2 found one coherent before-return row:
`return_stack_to_register`, represented by `src/20080719-1.c`. The execution
point is return-adjacent and should remain separately proven even if
implementation can share local helpers with other move materializers.

## Execution Rules

- Consume prepared before-return move records, not case-log text.
- Preserve return-adjacent execution semantics.
- Keep missing return move authority or ambiguous return storage fail-closed.
- If implementation discovers missing producer facts, stop and route that
  producer gap instead of inferring in RV64.
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

### Step 1: Inspect Before-Return Move Surfaces

Locate the prepared before-return move-bundle publication and RV64 consumption
surfaces for the `return_stack_to_register` shape.

Completion means `todo.md` records the exact records/helpers to consume, the
representative proof row, and whether the current prepared facts are complete
enough for implementation.

### Step 2: Materialize The Before-Return Move

Implement RV64 lowering for the coherent prepared before-return
`return_stack_to_register` move.

Completion means focused backend coverage proves the representative
before-return row and preserves fail-closed behavior for missing or ambiguous
prepared facts.

### Step 3: Residual Disposition For Before-Return Moves

Decide whether idea 503 can close after the before-return move is materialized
or whether a new focused producer/consumer idea is needed.

Completion means `todo.md` records the residual disposition and the active
lifecycle state is closed, switched, or extended with no more than one active
plan.
