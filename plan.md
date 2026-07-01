# Real Semantic Materialization Interval Fact Population Plan

Status: Active
Source Idea: ideas/open/477_real_semantic_materialization_interval_fact_population.md
Activated From: ideas/closed/476_semantic_instruction_result_frame_slot_materialization_facts.md

## Purpose

Populate real prepared semantic materialization/write and no-clobber interval
facts using the status surface completed by idea 476.

## Goal

Enable later source-fact population only after real semantic instruction-result
materialization/write records and path/no-clobber interval facts exist.

## Core Rule

Do not populate downstream `PreparedFrameSlotSourceFact` or
`PreparedBranchStackLoadAuthority` records from this plan. This plan owns only
real semantic materialization/write and interval fact population.

## Read First

- ideas/open/477_real_semantic_materialization_interval_fact_population.md
- ideas/closed/476_semantic_instruction_result_frame_slot_materialization_facts.md
- build/agent_state/476_step4_semantic_materialization_interval_residual/disposition.md
- build/agent_state/476_step3_semantic_materialization_interval_surface/summary.md
- build/agent_state/476_step2_semantic_materialization_interval_contract/contract.md
- ideas/closed/475_prepared_frame_slot_source_fact_population.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - Real `930930-1` `%t23 = ne i32 %t22, 0` into slot `#21`: semantic identity
    and destination evidence are visible, but no producer-populated
    materialization/write event or path/no-clobber interval facts exist.
- Rejected evidence:
  - `%t22 -> %t23` storage move has `authority=none` and is not semantic
    compare-result materialization.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Defining another independent status surface unless Step 1 proves the idea
  476 surface is structurally insufficient.
- Directly populating `PreparedFrameSlotSourceFact` rows.
- Directly marking branch-stack-load records available.
- RV64 branch-load emission or target lowering.
- Reusing storage-only moves as semantic materialization evidence when
  authority is absent.
- Pointer-value/provenance repair.
- Select-result/block-entry stack-destination repair.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring materialization or no-clobber facts from raw BIR shape, final stack
  homes, storage, offsets, object ids, value names, function names, testcase
  names, or dump order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 476 completed the semantic materialization / interval planner and status
surface. This plan owns real producer population for the representative row and
must preserve all downstream boundaries.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream source-fact and branch-stack-load authority unavailable until
  later packets consume explicit real semantic facts.
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

### Step 1: Audit Real Semantic Fact Population Inputs

Inspect real prepared evidence for semantic instruction-result identity,
materialization/write producers, path or edge validity, same-slot writes,
calls/helpers/inline asm, publications, move bundles, and parallel copies for
`%t23` slot `#21`. Completion means `todo.md` records whether a real fact
population packet is justified and which rows remain out of scope.

### Step 2: Define Real Population Contract

Specify how real prepared evidence populates existing semantic
materialization/interval records and which statuses remain fail-closed.
Completion means `todo.md` records positive and negative cases and names
implementation/test surfaces if a bounded producer packet is justified.

### Step 3: Implement Or Route Real Semantic Fact Population

If Step 2 finds a bounded producer packet, implement only that population
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes source-fact population / other durable follow-up.
