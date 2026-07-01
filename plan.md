# Semantic Instruction Result Write Event Carrier Plan

Status: Active
Source Idea: ideas/open/478_semantic_instruction_result_write_event_carrier.md
Activated From: ideas/closed/477_real_semantic_materialization_interval_fact_population.md

## Purpose

Create the authoritative prepared event carrier for semantic instruction-result
frame-slot write/materialization events.

## Goal

Enable later real semantic interval population only after explicit event
authority proves a semantic result was written or materialized into a frame
slot.

## Core Rule

Do not use storage-only moves or final homes as semantic write authority. This
plan owns only the event carrier and producer authority needed before interval
facts can be populated.

## Read First

- ideas/open/478_semantic_instruction_result_write_event_carrier.md
- ideas/closed/477_real_semantic_materialization_interval_fact_population.md
- build/agent_state/477_step4_real_semantic_fact_population_residual/disposition.md
- build/agent_state/477_step3_real_semantic_fact_population/blocker.md
- build/agent_state/477_step2_real_semantic_fact_population_contract/contract.md
- ideas/closed/476_semantic_instruction_result_frame_slot_materialization_facts.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - Real `930930-1` `%t23 = ne i32 %t22, 0` into slot `#21`: semantic identity
    and destination facts exist, but no authoritative semantic
    write/materialization event carrier exists.
- Rejected evidence:
  - `%t22 -> %t23` storage move has `authority=none`, `from_value_id=16`, and
    `to_value_id=17`; it is not semantic compare-result materialization.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Path/dominance coverage, same-slot write exclusion, and effect non-clobber
  interval proof.
- Directly populating semantic materialization interval records as available.
- Directly populating `PreparedFrameSlotSourceFact` or
  `PreparedBranchStackLoadAuthority` rows.
- RV64 branch-load emission or target lowering.
- Reusing storage-only moves as semantic materialization when authority is
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

Idea 477 found that real population cannot start until prepared data exposes an
authoritative event saying a semantic instruction result was materialized into
a frame slot. This plan owns that event carrier, not interval proof or
downstream consumption.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream semantic interval, source-fact, and branch-stack-load
  availability unavailable until later packets consume explicit event carriers.
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

### Step 1: Audit Semantic Write Event Carrier Inputs

Inspect prepared event, move, publication, instruction-result, value-home, and
frame-slot evidence for `%t23` slot `#21`. Completion means `todo.md` records
whether an authoritative semantic write-event carrier packet is justified and
which rows remain out of scope.

### Step 2: Define Write Event Carrier Contract

Specify records/statuses for semantic result identity, event site, destination
frame slot/object/offset/size/alignment, and event authority. Completion means
`todo.md` records positive and negative cases and names implementation/test
surfaces if a bounded producer packet is justified.

### Step 3: Implement Or Route Write Event Carrier

If Step 2 finds a bounded producer packet, implement only that event-carrier
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes interval population / other durable follow-up.
