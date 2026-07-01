# RV64 Before-Instruction Prepared Move Materialization Plan

Status: Active
Source Idea: ideas/open/501_rv64_before_instruction_prepared_move_materialization.md
Activated From: ideas/closed/495_prepared_move_bundle_materialization_bucket_review.md

## Purpose

Implement the highest-priority coherent move-bundle follow-up split out by idea
495: RV64 materialization for prepared before-instruction consumer moves.

## Goal

Materialize coherent prepared before-instruction move bundles in RV64 from the
prepared record stream, without reconstructing authority, storage identity, or
consumer ordering from raw BIR shape.

## Core Rule

RV64 may lower a before-instruction move only when explicit prepared
move-bundle facts provide the phase, destination storage, move reason, and
consumer ordering. Testcase names, source shape, raw instruction order, case-log
text, final homes, and object output are not authority.

## Read First

- ideas/open/501_rv64_before_instruction_prepared_move_materialization.md
- ideas/closed/495_prepared_move_bundle_materialization_bucket_review.md
- build/agent_state/495_step2_move_bundle_coherence/summary.md
- build/agent_state/495_step2_move_bundle_coherence/classification.tsv
- build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv
- build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv

## Current Targets

- Inputs:
  - 328 coherent prepared before-instruction rows from idea 495 Step 2;
  - existing prepared move-bundle facts with `phase=before_instruction`,
    `authority=none`, `parallel_copy=no`, destination `stack_slot`, and move
    reasons `consumer_register_to_stack` or `consumer_stack_to_stack`.
- Outputs:
  - RV64 materialization for prepared register-to-stack consumer moves;
  - RV64 materialization for prepared stack-to-stack consumer moves;
  - focused backend coverage for representative rows and fail-closed
    incomplete-authority boundaries.

## Non-Goals

- Out-of-SSA/pre-terminator parallel-copy materialization.
- Before-return move materialization.
- Select-publication evidence or authority repair.
- Producer/prepared fact changes unless implementation proves a separate
  producer gap and routes it to a new idea.
- F128, call ABI, broad stack-frame rewrites, expectation rewrites,
  unsupported-marker downgrades, allowlists, pass/fail accounting changes,
  runtime-comparison changes, or baseline churn.

## Working Model

Idea 495 Step 2 found that this family has coherent prepared authority and is
blocked at RV64 materialization. The first implementation should consume the
prepared move-bundle surface as the authority source and prove both
register-to-stack and stack-to-stack before-instruction copies.

## Execution Rules

- Consume prepared move records, not case-log text.
- Preserve prepared ordering relative to the consuming instruction.
- Keep incomplete, ambiguous, or contradictory prepared facts fail-closed.
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

### Step 1: Inspect Before-Instruction Prepared Move Surfaces

Locate the prepared move-bundle publication and RV64 consumption surfaces for
`before_instruction_copies` with destination `stack_slot`.

Completion means `todo.md` records the exact records/helpers to consume, the
representative proof rows, and whether the current prepared facts are complete
enough for implementation.

### Step 2: Materialize Register-To-Stack Consumer Moves

Implement RV64 lowering for coherent prepared before-instruction
`consumer_register_to_stack` moves.

Completion means focused backend coverage proves a representative
register-to-stack row and preserves fail-closed behavior for missing or
ambiguous prepared facts.

### Step 3: Materialize Stack-To-Stack Consumer Moves

Implement RV64 lowering for coherent prepared before-instruction
`consumer_stack_to_stack` moves.

Completion means focused backend coverage proves a representative
stack-to-stack row and keeps this path scoped to prepared before-instruction
facts.

### Step 4: Residual Disposition For Before-Instruction Moves

Decide whether idea 501 can close after both before-instruction move families
are materialized or whether a new focused producer/consumer idea is needed.

Completion means `todo.md` records the residual disposition and the active
lifecycle state is closed, switched, or extended with no more than one active
plan.
