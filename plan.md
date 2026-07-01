# RV64 Out-Of-SSA Parallel-Copy Move Materialization Plan

Status: Active
Source Idea: ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
Activated From: ideas/closed/501_rv64_before_instruction_prepared_move_materialization.md

## Purpose

Implement the next coherent move-bundle follow-up split out by idea 495: RV64
materialization for prepared out-of-SSA pre-terminator parallel copies.

## Goal

Materialize coherent prepared out-of-SSA parallel-copy move bundles in RV64
while preserving predecessor-edge execution semantics and prepared
parallel-copy authority.

## Core Rule

RV64 may lower out-of-SSA parallel copies only from explicit prepared authority
records that provide the predecessor execution site, parallel-copy semantics,
destination storage, and move reason. Raw block order, source shape, filenames,
labels, final homes, and target output are not edge or phi authority.

## Read First

- ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
- ideas/closed/495_prepared_move_bundle_materialization_bucket_review.md
- ideas/closed/501_rv64_before_instruction_prepared_move_materialization.md
- build/agent_state/495_step2_move_bundle_coherence/summary.md
- build/agent_state/495_step2_move_bundle_coherence/classification.tsv
- build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv
- build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv

## Current Targets

- Inputs:
  - 91 coherent prepared out-of-SSA/pre-terminator rows from idea 495 Step 2;
  - prepared move-bundle facts with `event_kind=pre_terminator_copies`,
    `phase=block_entry`, `authority=out_of_ssa_parallel_copy`,
    `parallel_copy=yes`, and `execution_site=predecessor_terminator`.
- Outputs:
  - RV64 materialization for `phi_join_register_to_register` moves;
  - RV64 materialization for
    `edge_consumer_preservation_register_to_register` moves;
  - RV64 materialization for
    `edge_consumer_preservation_register_to_stack` moves;
  - focused backend coverage for representative rows and fail-closed
    incomplete-authority boundaries.

## Non-Goals

- Before-instruction consumer moves already completed by idea 501.
- Before-return move materialization.
- Select-publication evidence or authority repair.
- Inventing predecessor/successor, phi, edge, execution-site, or storage
  identity in RV64.
- Producer/prepared fact changes unless implementation proves a separate
  producer gap and routes it to a new idea.
- F128, call ABI, broad stack-frame rewrites, expectation rewrites,
  unsupported-marker downgrades, allowlists, pass/fail accounting changes,
  runtime-comparison changes, or baseline churn.

## Working Model

Idea 495 Step 2 found this family has coherent prepared parallel-copy
authority and is blocked at RV64 materialization. Implementation should consume
the prepared bundle semantics, preserve predecessor-terminator execution, and
avoid serializing parallel copies in a way that clobbers live sources.

## Execution Rules

- Consume prepared out-of-SSA parallel-copy records, not case-log text.
- Preserve predecessor-edge execution semantics.
- Preserve parallel-copy cycle/order safety from the prepared bundle
  representation.
- Keep missing edge identity, missing execution site, incomplete parallel-copy
  facts, and contradictory storage fail-closed.
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

### Step 1: Inspect Out-Of-SSA Parallel-Copy Surfaces

Locate the prepared out-of-SSA parallel-copy publication and RV64 consumption
surfaces for predecessor-terminator move bundles.

Completion means `todo.md` records the exact records/helpers to consume, the
representative proof rows, and whether the current prepared facts are complete
enough for implementation.

### Step 2: Materialize Phi Join Register Moves

Implement RV64 lowering for coherent prepared
`phi_join_register_to_register` out-of-SSA parallel copies.

Completion means focused backend coverage proves a representative phi-join row
and preserves fail-closed behavior for missing or ambiguous prepared facts.

### Step 3: Materialize Edge Consumer Preservation Moves

Implement RV64 lowering for coherent prepared edge-consumer preservation moves,
including register-to-register and register-to-stack destination families.

Completion means focused backend coverage proves representative edge-consumer
preservation rows and keeps the path scoped to prepared out-of-SSA
parallel-copy authority.

### Step 4: Residual Disposition For Out-Of-SSA Moves

Decide whether idea 502 can close after out-of-SSA parallel-copy materialization
or whether a new focused producer/consumer idea is needed.

Completion means `todo.md` records the residual disposition and the active
lifecycle state is closed, switched, or extended with no more than one active
plan.
