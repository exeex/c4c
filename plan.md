# Prepared Frame-Slot Source-Fact Population Plan

Status: Active
Source Idea: ideas/open/475_prepared_frame_slot_source_fact_population.md
Activated From: ideas/closed/474_prepared_frame_slot_materialization_no_clobber_facts.md

## Purpose

Populate real prepared frame-slot source facts using the carrier/status surface
completed by idea 474.

## Goal

Produce real materialization/write, path/dominance, same-slot write exclusion,
effect safety, and publication/move/copy non-clobber facts for scalar branch
stack slots without raw-shape inference.

## Core Rule

Do not set downstream branch-stack-load authority availability in this plan.
Populate only the independent prepared source-fact records created by idea 474.

## Read First

- ideas/open/475_prepared_frame_slot_source_fact_population.md
- ideas/closed/474_prepared_frame_slot_materialization_no_clobber_facts.md
- build/agent_state/474_step4_residual_disposition/disposition.md
- build/agent_state/474_step3_source_fact_carrier/summary.md
- review/474_step3_source_fact_carrier_review.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - `f.logic.end.14` condition `%t23`, slot `#21`: carrier can preserve a
    source-fact row, but real prepared data still reports
    `missing_materialization_event`.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Defining another carrier/status surface unless Step 1 proves the existing
  idea 474 surface is structurally insufficient.
- RV64 branch-load emission or target lowering.
- Directly marking branch-stack-load records available.
- Branch-stack-load policy/freshness/clobber consumption.
- Pointer-value/provenance repair.
- Select-result/block-entry stack-destination repair.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring source facts from stack homes/storage, offsets, object ids, raw BIR
  shape, value names, block labels, function names, testcase names, or dump
  order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 474 completed the source-fact carrier/status API. This plan owns the next
producer layer: populating real records from explicit materialization/write and
no-clobber evidence while preserving protected boundary classes.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream branch-stack-load authority unavailable until a later packet
  consumes explicit source facts.
- Preserve select-result, pointer/provenance, and unsupported-terminator rows
  as `unsupported_boundary` or separate owners.
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

### Step 1: Audit Real Source-Fact Population Inputs

Inspect real prepared evidence for frame-slot writes, value homes, stack
objects, path/dominance, same-slot writes, calls/helpers/inline asm,
publications, move bundles, and parallel copies for `%t23` slot `#21`.
Completion means `todo.md` records which source facts can be populated and
which boundary rows remain out of scope.

### Step 2: Define Population Contract

Specify how real prepared evidence populates existing source-fact records and
which statuses remain fail-closed. Completion means `todo.md` records positive
and negative cases and names implementation/test surfaces if a bounded producer
packet is justified.

### Step 3: Implement Or Route Source-Fact Population

If Step 2 finds a bounded producer packet, implement only that population
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes downstream certificate/authority / other durable
follow-up.
