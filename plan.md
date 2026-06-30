# Select-Result Branch Publication Plan

Status: Active
Source Idea: ideas/open/450_select_result_branch_publication.md

## Purpose

Define and consume explicit prepared authority for select-result values that
feed branch conditions, without reconstructing select publication from raw BIR
shape.

## Goal

Classify select-result branch publication and move-bundle materialization
residuals, define the prepared facts required for accepted select-result
branch conditions, and select only bounded producer or consumer packets that
consume explicit authority.

## Core Rule

Do not infer select-result publication, condition conversion, move-bundle
targets, branch operands, or value homes from raw select instructions,
`ne i32 <select>, 0` shape, block order, filenames, function names, testcase
names, or one prepared dump layout.

## Read First

- ideas/open/450_select_result_branch_publication.md
- ideas/closed/441_terminator_select_publication_authority.md
- ideas/closed/449_pointer_relational_branch_publication.md
- build/agent_state/441_step4_residual_disposition/classification.md
- build/agent_state/449_step4_residual_disposition/classification.md
- build/agent_state/449_step4_residual_disposition/20010329-1.prepared.out
- build/agent_state/449_step4_residual_disposition/20000622-1.prepared.out
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20010329-1` current `unsupported_move_bundle_target_shape` residual after
  pointer relational branch publication.
- `root_is_select=yes` select-chain rows for `%t22`, `%t36`, and `%t50` in
  `20010329-1`.
- `20000622-1` select-result branch candidates such as `%t13/%t24` feeding
  `ne i32 ..., 0`.
- Prepared facts for root select identity, select result value home, condition
  conversion, comparison against zero, branch targets, move-bundle target
  materialization, and consumer boundary.

## Non-Goals

- Pointer equality/inequality branch publication completed by idea 441.
- Unsigned pointer relational branch publication completed by idea 449.
- Stack-home branch operand/condition materialization, tracked by idea 451.
- Pointer-value memory provenance, local/global store-source publication, and
  unsupported instruction/storage residuals.
- Generic RV64 select or branch lowering from raw BIR without prepared
  publication facts.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.
- Touching `review/440_step5_direct_global_return_consumer_review.md`.

## Working Model

The prior branch-publication ideas handled direct fused compare predicates.
Select-result branches are different: a select result must be explicitly
published into a branch condition or move-bundle target before RV64 can consume
it. This plan either proves a bounded prepared publication/consumer route for
select-result branch conditions or records a precise instruction-side,
producer, or materialization blocker.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat `root_is_select=yes` and raw `ne i32 <select>, 0` as candidate
  evidence only, not authority.
- Separate select-result publication gaps from instruction-side,
  stack-home, pointer-value, and storage residuals.
- Add focused tests for accepted select-result branch facts and fail-closed
  missing/incoherent publication boundaries.
- Select at most one narrow code packet after the publication contract is
  explicit.
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

### Step 1: Audit Select-Result Branch Evidence

Inspect the 441 and 449 residual evidence for select-result branch
publication rows. Record each row's function/block, root select identity,
condition conversion, comparison shape, result value home, move-bundle target,
branch targets, current publication authority, and first missing producer,
consumer, instruction-side, or materialization fact. Completion means
`todo.md` contains a bucket table and identifies the first bounded
select-result branch packet or exact blocker.

### Step 2: Define Select-Result Publication Contract

Specify the prepared facts required to publish select results into branch
conditions and move-bundle targets, including root select identity, selected
value provenance, result home, condition conversion, comparison against zero,
target labels, and consumer boundary. Separate accepted records from
instruction-side blockers, stack-home blockers, and incoherent authority.
Completion means `todo.md` states the contract, rejected adjacent shapes, owned
files/tests, and proof command.

### Step 3: Implement Or Route First Select-Result Packet

If a coherent producer/prepared or RV64 consumer packet exists, implement the
smallest semantic change with focused coverage. If the first owner is an
instruction-side blocker, stack-home materialization gap, or pointer-value
provenance gap, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative select-result branch rows against the Step 3 result,
classify remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining select-result
branch packet, or route durable follow-up work.
