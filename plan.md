# Semantic Instruction Result Frame-Slot Write Event Producer Plan

Status: Active
Source Idea: ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
Activated From: ideas/closed/479_real_semantic_write_event_authority_producer.md

## Purpose

Produce lower-level prepared semantic instruction-result frame-slot
write/materialization events.

## Goal

Enable later real event-authority population only after a durable producer
records that a semantic instruction result was written or materialized into a
specific frame slot.

## Core Rule

Do not infer event authority from raw BIR, final homes, storage movement, names,
or layout. This plan owns only explicit semantic write/materialization event
production.

## Read First

- ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
- ideas/closed/479_real_semantic_write_event_authority_producer.md
- build/agent_state/479_step4_real_event_authority_residual/disposition.md
- build/agent_state/479_step3_real_event_authority_blocker/blocker.md
- build/agent_state/479_step2_real_event_authority_contract/contract.md
- ideas/closed/478_semantic_instruction_result_write_event_carrier.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - Real `930930-1` `%t23 = bir.ne i32 %t22, 0` into slot `#21`: semantic
    identity and destination facts exist, but no semantic write/materialization
    event producer exists.
- Rejected evidence:
  - `%t22 -> %t23` storage move has `authority=none`,
    `from_value_id=16`, and `to_value_id=17`; it is not semantic
    compare-result materialization.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Consuming the event through idea 478/479 carrier availability.
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
- Inferring event authority from raw BIR shape, final stack homes, storage,
  offsets, object ids, value names, function names, testcase names, or dump
  order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 479 could not populate real event authority because no lower-level
semantic write event exists. This plan owns that producer surface only.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream event-authority, interval, source-fact, and branch-stack-load
  availability unavailable until later packets consume explicit events.
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

### Step 1: Audit Semantic Write Event Producer Inputs

Inspect prepared instruction-result, storage, publication, move, value-home,
and frame-slot evidence for `%t23` slot `#21`. Completion means `todo.md`
records whether a semantic write-event producer packet is justified and which
rows remain out of scope.

### Step 2: Define Semantic Write Event Producer Contract

Specify records/statuses for function/site, semantic producer identity, result
value id/name/type, opcode, event source/result, destination slot/object/layout,
and event authority. Completion means `todo.md` records positive and negative
cases and names implementation/test surfaces if a bounded producer packet is
justified.

### Step 3: Implement Or Route Semantic Write Event Producer

If Step 2 finds a bounded producer packet, implement only that event-producer
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes event-authority consumption / other durable
follow-up.
