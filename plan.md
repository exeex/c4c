# Select-Edge Source Producer Move-Bundle Placement Authority Plan

Status: Active
Source Idea: ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md

## Purpose

Define producer/prepared placement metadata for select-edge source-producer
before-instruction register-destination move bundles.

## Goal

Classify the `20010329-1` target bundle and publish only the narrow placement
authority needed to distinguish same-block, edge-owned, and suppression
semantics before any RV64 consumer lowering resumes.

## Core Rule

Do not infer placement from value ids, block indexes, instruction indexes, raw
BIR shape, or testcase layout. The producer must publish explicit placement or
edge identity authority first.

## Read First

- ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md
- ideas/closed/457_before_instruction_stack_to_register_move_materialization.md
- build/agent_state/457_step4_residual_disposition/disposition.md
- build/agent_state/457_step3_register_destination_move_materialization/blocker.md
- build/agent_state/456_step7_final_residual_disposition/20010329-1.prepared.out
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/prepared_object_traversal.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- `20010329-1` target before-instruction bundle:
  `block_index=4 instruction_index=2`.
- Moves into `%t18`/`t0`, including `consumer_stack_to_register`.
- Source producer `%t18 = bir.ule ptr %t15, %t17`.
- Select carrier `%t22 = bir.select uge ptr %t5, %t7, i32 %t18, 0`.
- Edge transfer `logic.rhs.end.13 -> logic.end.14 incoming=%t18
  destination=%t22`.
- Missing placement meaning: same-block setup, edge-owned materialization, or
  suppression because predecessor-edge publication already consumes the source
  producer.

## Non-Goals

- RV64 lowering for the register-destination bundle before placement authority
  exists.
- Generic stack-to-register or register-to-register move lowering.
- Reopening idea 456 cast-dependency consumption.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Generic stack-home branch operand/condition materialization tracked by idea
  451.
- Pointer-value provenance, local/global store publication, or generic
  instruction-side lowering.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

The target bundle might be a same-block compare-operand setup, an edge-owned
source-producer materialization, or a bundle that should be suppressed because
the predecessor-edge publication already materializes the source producer.
Those meanings have different safety properties. The prepared producer must
make the placement explicit before RV64 can consume or suppress the bundle.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat current prepared rows as candidate evidence only.
- Keep RV64 consumer lowering, generic stack move support, and pointer
  provenance separate.
- Add focused producer/prepared tests for accepted placement facts and
  fail-closed missing, ambiguous, mismatched, raw-inferred, unsafe, or
  unrelated bundles.
- Select at most one narrow producer/prepared metadata packet after the audit
  is explicit.
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

### Step 1: Audit Select-Edge Placement Evidence

Inspect the 457 artifacts and prepared output for the target bundle. Record
bundle phase, block/instruction coordinates, moves, source producer, select
carrier, edge transfer, source/destination homes, existing source-producer
facts, and first missing placement authority. Completion means `todo.md`
contains a bucket table and identifies the first bounded metadata packet or
exact blocker.

### Step 2: Define Placement Authority Contract

Specify the prepared facts required to distinguish same-block setup,
edge-owned materialization, and predecessor-edge-consumed suppression,
including predecessor/successor identity and source-producer linkage.
Completion means `todo.md` states accepted/rejected shapes, owned files/tests,
and proof command.

### Step 3: Implement Or Route First Placement Metadata Packet

If a coherent producer/prepared packet exists, implement the smallest semantic
metadata change with focused coverage. If the first owner is outside producer
metadata, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check the target bundle and representative prepared output against the Step
3 result, classify remaining residuals, and decide whether this source idea is
complete. Completion means close, keep active with one exact remaining packet,
or route durable follow-up work.
