# Pointer Relational Branch Publication Plan

Status: Active
Source Idea: ideas/open/449_pointer_relational_branch_publication.md

## Purpose

Define and consume explicit prepared authority for pointer relational fused
branch predicates before RV64 lowering, without treating them as covered by the
closed pointer equality/inequality branch route.

## Goal

Classify pointer relational branch residuals, define the accepted predicate
policy for pointer ordering compares, and select only bounded producer or
consumer packets that consume explicit prepared branch-condition facts.

## Core Rule

Do not infer pointer relational branch semantics, ordering policy, null
handling, operand provenance, or target labels from raw BIR compare/branch
shape, block order, filenames, function names, testcase names, or one prepared
dump layout.

## Read First

- ideas/open/449_pointer_relational_branch_publication.md
- ideas/closed/441_terminator_select_publication_authority.md
- build/agent_state/441_step4_residual_disposition/classification.md
- build/agent_state/441_step4_residual_disposition/20010329-1.prepared.out
- build/agent_state/441_step4_residual_disposition/930930-1.prepared.out
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20010329-1` pointer relational fused compare:
  `branch_condition entry ... compare=uge ptr %t5, %t7`.
- Pointer relational branch candidates in `930930-1`, including `ult ptr`
  shapes from the 441 Step 4 residual disposition.
- Prepared branch-condition facts for pointer relational predicates,
  compare operands, result home, target labels, and consumer boundary.
- Explicit policy for allowed pointer relational predicates, ordering semantics,
  null handling, provenance requirements, and fail-closed unsupported forms.

## Non-Goals

- Pointer equality/inequality branch publication completed by idea 441.
- Select-result publication into branch conditions, tracked by idea 450.
- Pointer-value memory provenance, local/global store-source publication, and
  unsupported instruction/storage residuals.
- Generic RV64 branch lowering from raw BIR without prepared authority.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.
- Touching `review/440_step5_direct_global_return_consumer_review.md`.

## Working Model

The closed 441 packet accepted only fused pointer `eq/ne` branches. Pointer
relational predicates such as `uge` and `ult` need a separate explicit policy
because pointer ordering semantics depend on predicate choice, null handling,
operand provenance, and target-consumable homes. This plan either proves a
bounded prepared publication/consumer route for such predicates or records a
precise fail-closed blocker.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat raw pointer relational compares and conditional branches as
  insufficient authority.
- Separate policy/producer gaps from coherent RV64 consumer work.
- Preserve the closed pointer `eq/ne` route; do not broaden it implicitly.
- Add focused tests for accepted relational branch facts and fail-closed
  missing/incoherent/unsupported policy boundaries.
- Select at most one narrow code packet after the predicate policy is explicit.
- Do not touch `test_baseline.new.log`, `test_before.log`, `test_after.log`,
  or `review/`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Pointer Relational Branch Evidence

Inspect the 441 Step 4 residual evidence for pointer relational fused branch
conditions. Record each row's function/block, predicate, operand types,
operand homes, provenance/null evidence, result home, prepared branch-condition
fact, target labels, current authority state, and first missing policy,
producer, or consumer fact. Completion means `todo.md` contains a bucket table
and identifies the first bounded pointer relational branch packet or exact
blocker.

### Step 2: Define Pointer Relational Predicate Policy

Specify which pointer relational predicates can be accepted, the required
ordering semantics, null/provenance constraints, operand/result homes, and
branch target label facts. Separate accepted records from unsupported,
policy-ambiguous, missing, or incoherent authority. Completion means `todo.md`
states the contract, rejected adjacent shapes, owned files/tests, and proof
command.

### Step 3: Implement Or Route First Relational Branch Packet

If a coherent producer/prepared or RV64 consumer packet exists, implement the
smallest semantic change with focused coverage. If the first owner is a
distinct policy blocker, provenance producer gap, or instruction/storage
residual, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative pointer relational branch rows against the Step 3
result, classify remaining residuals, and decide whether this source idea is
complete. Completion means close, keep active with an exact remaining pointer
relational branch packet, or route durable follow-up work.
