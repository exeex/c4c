# RV64 Select-Edge Suppression Placement Consumer Plan

Status: Active
Source Idea: ideas/open/459_rv64_select_edge_suppression_placement_consumer.md

## Purpose

Consume explicit select-edge source-producer placement metadata in RV64 object
emission.

## Goal

Audit and implement only the bounded RV64 consumer for
`predecessor_edge_consumed_suppression` placement metadata on the
`20010329-1` before-instruction register-destination bundle.

## Core Rule

Do not infer suppression from value ids, block indexes, instruction indexes,
raw BIR shape, or testcase layout. RV64 may suppress only bundles authorized by
explicit prepared placement metadata.

## Read First

- ideas/open/459_rv64_select_edge_suppression_placement_consumer.md
- ideas/closed/458_select_edge_source_producer_move_bundle_placement_authority.md
- build/agent_state/458_step4_residual_disposition/disposition.md
- build/agent_state/458_step3_select_edge_placement_metadata/summary.md
- build/agent_state/458_step2_placement_authority_contract/contract.md
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20010329-1` target before-instruction bundle:
  `block_index=4 instruction_index=2`.
- Placement kind `predecessor_edge_consumed_suppression`.
- Source producer `%t18 = bir.ule ptr %t15, %t17`.
- Select carrier `%t22 = bir.select uge ptr %t5, %t7, i32 %t18, 0`.
- Edge transfer `logic.rhs.end.13 -> logic.end.14 incoming=%t18
  destination=%t22`.
- Register-destination moves into `%t18`/`t0`, including
  `consumer_stack_to_register`.

## Non-Goals

- Producing placement metadata, closed by idea 458.
- Generic stack-to-register or register-to-register move lowering.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening idea 456 cast-dependency consumption.
- Generic stack-home branch operand/condition materialization tracked by idea
  451.
- Pointer-value provenance, local/global store publication, or generic
  instruction-side lowering.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

The producer now publishes a placement record that says the target
before-instruction bundle is already consumed by predecessor-edge publication.
The RV64 consumer should use that explicit metadata to suppress the join-block
bundle and should reject all adjacent shapes without explicit authority.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat placement records as the only admissible suppression source.
- Keep generic move support, stack-load freshness, pointer provenance, and
  branch stack-home work separate.
- Add focused RV64 object tests for accepted suppression and fail-closed
  missing/mismatched/unsupported/raw-inferred cases.
- Select at most one narrow RV64 consumer packet after the audit is explicit.
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

### Step 1: Audit Suppression Placement Consumer Evidence

Inspect the 458 artifacts and current prepared/object route for the target
bundle. Record placement record fields, edge identity, source producer,
select carrier, bundle site, moves, current RV64 event visibility, and first
missing consumer fact. Completion means `todo.md` contains a bucket table and
identifies the first bounded consumer packet or exact blocker.

### Step 2: Define RV64 Suppression Consumer Contract

Specify the RV64 conditions for consuming `predecessor_edge_consumed_suppression`
records, including matching placement metadata, move identities, edge/source
producer linkage, bundle site, and fail-closed adjacent shapes. Completion
means `todo.md` states accepted/rejected shapes, owned files/tests, and proof
command.

### Step 3: Implement Or Route First Suppression Consumer Packet

If a coherent RV64 consumer packet exists, implement the smallest semantic
change with focused coverage. If the first owner is outside suppression
consumer work, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check `20010329-1` and focused coverage against the Step 3 result, classify
remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with one exact remaining packet, or route
durable follow-up work.
