# Semantic Result Frame-Slot Materialization Point Producer Plan

Status: Active
Source Idea: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
Activated From: ideas/closed/480_semantic_instruction_result_frame_slot_write_event_producer.md

## Purpose

Produce explicit prepared materialization-point facts for semantic instruction
results written into frame slots.

## Goal

Enable later write-event production only after a durable materialization point
proves the semantic result was written or materialized into the destination
frame slot.

## Core Rule

Do not infer materialization points from raw BIR, branch conditions, final
homes, storage movement, names, or layout. This plan owns only explicit
materialization-point production.

## Read First

- ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
- ideas/closed/480_semantic_instruction_result_frame_slot_write_event_producer.md
- build/agent_state/480_step4_semantic_write_event_producer_residual/disposition.md
- build/agent_state/480_step3_semantic_write_event_producer_blocker/blocker.md
- build/agent_state/480_step2_semantic_write_event_producer_contract/contract.md
- ideas/closed/479_real_semantic_write_event_authority_producer.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - Real `930930-1` `%t23 = bir.ne i32 %t22, 0` into slot `#21`: semantic
    identity and final destination facts exist, but no explicit
    materialization point exists.
- Rejected evidence:
  - `%t22 -> %t23` storage move has `authority=none`,
    `from_value_id=16`, and `to_value_id=17`; it is not semantic
    compare-result materialization.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Consuming materialization-point records through ideas 478/479/480.
- Path/dominance coverage, same-slot write exclusion, and effect non-clobber
  interval proof.
- Populating semantic interval, `PreparedFrameSlotSourceFact`, or
  `PreparedBranchStackLoadAuthority` rows.
- RV64 branch-load emission or target lowering.
- Treating storage-only moves as semantic materialization when authority is
  absent.
- Pointer-value/provenance repair.
- Select-result/block-entry stack-destination repair.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring materialization points from raw BIR shape, branch conditions, final
  stack homes, storage, offsets, object ids, value names, function names,
  testcase names, or dump order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 480 could not produce semantic write events because no explicit
materialization point exists. This plan owns that missing producer surface only.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream write-event, event-authority, interval, source-fact, and
  branch-stack-load availability unavailable until later packets consume
  explicit materialization points.
- Preserve select-result, pointer/provenance, and unsupported-terminator rows
  as separate owners.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a producer packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Materialization Point Inputs

Inspect prepared instruction-result, branch-condition, storage, publication,
move, value-home, and frame-slot evidence for `%t23` slot `#21`. Completion
means `todo.md` records whether an explicit materialization-point producer
packet is justified and which rows remain out of scope.

### Step 2: Define Materialization Point Contract

Specify records/statuses for function/site, semantic producer identity, result
value id/name/type, opcode, event source/result, destination slot/object/layout,
and materialization authority. Completion means `todo.md` records positive and
negative cases and names implementation/test surfaces if a bounded producer
packet is justified.

### Step 3: Implement Or Route Materialization Point Producer

If Step 2 finds a bounded producer packet, implement only that
materialization-point surface with focused coverage. If no implementation is
justified, record the precise blocker and route it. Completion means proof
passes or lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes write-event consumption / other durable follow-up.
