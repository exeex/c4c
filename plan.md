# Real Semantic Write Event Authority Producer Plan

Status: Active
Source Idea: ideas/open/479_real_semantic_write_event_authority_producer.md
Activated From: ideas/closed/478_semantic_instruction_result_write_event_carrier.md

## Purpose

Populate real authoritative semantic instruction-result frame-slot
write/materialization event carriers using the surface completed by idea 478.

## Goal

Enable later semantic interval population only after real event authority proves
a semantic result was written or materialized into a frame slot.

## Core Rule

Do not infer event authority from final homes, storage movement, names, or raw
shape. Real event authority must come from explicit producer evidence.

## Read First

- ideas/open/479_real_semantic_write_event_authority_producer.md
- ideas/closed/478_semantic_instruction_result_write_event_carrier.md
- build/agent_state/478_step4_semantic_write_event_carrier_residual/disposition.md
- build/agent_state/478_step3_semantic_write_event_carrier/summary.md
- build/agent_state/478_step2_semantic_write_event_carrier_contract/contract.md
- ideas/closed/477_real_semantic_materialization_interval_fact_population.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - Real `930930-1` `%t23 = ne i32 %t22, 0` into slot `#21`: semantic identity
    and destination facts exist, but no real event authority exists.
- Rejected evidence:
  - `%t22 -> %t23` storage move has `authority=none` and remains storage-only,
    not semantic compare-result materialization.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Defining another carrier/status API unless Step 1 proves idea 478's surface
  is structurally insufficient.
- Path/dominance coverage, same-slot write exclusion, and effect non-clobber
  interval proof.
- Populating semantic interval records, `PreparedFrameSlotSourceFact`, or
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

Idea 478 completed the carrier/status surface. This plan owns real producer
population for event authority only; interval proof and downstream consumers
remain later owners.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream semantic interval, source-fact, and branch-stack-load
  availability unavailable until later packets consume explicit real event
  carriers.
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

### Step 1: Audit Real Event Authority Inputs

Inspect real prepared evidence for instruction-result producers, publications,
moves, value homes, frame slots, and event authority for `%t23` slot `#21`.
Completion means `todo.md` records whether a real event-authority producer
packet is justified and which rows remain out of scope.

### Step 2: Define Real Event Authority Contract

Specify how real prepared evidence populates semantic write-event authority and
which statuses remain fail-closed. Completion means `todo.md` records positive
and negative cases and names implementation/test surfaces if a bounded producer
packet is justified.

### Step 3: Implement Or Route Real Event Authority Producer

If Step 2 finds a bounded producer packet, implement only that event-authority
population surface with focused coverage. If no implementation is justified,
record the precise blocker and route it. Completion means proof passes or
lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes semantic interval population / other durable
follow-up.
