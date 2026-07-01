# Semantic Instruction Result Frame-Slot Materialization Facts Plan

Status: Active
Source Idea: ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md
Activated From: ideas/closed/475_prepared_frame_slot_source_fact_population.md

## Purpose

Produce lower-level semantic instruction-result materialization/write and
path/no-clobber interval facts for later prepared source-fact population.

## Goal

Enable idea 475-style source-fact population only after explicit semantic
instruction-result identity, frame-slot materialization/write, path validity,
same-slot exclusion, effect safety, and publication/move/copy non-clobber facts
exist.

## Core Rule

Do not populate downstream source-fact or branch-stack-load authority records
from this plan. This plan owns only semantic materialization/write and interval
facts that later packets may consume.

## Read First

- ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md
- ideas/closed/475_prepared_frame_slot_source_fact_population.md
- build/agent_state/475_step4_source_fact_population_residual/disposition.md
- build/agent_state/475_step3_source_fact_population_blocker/blocker.md
- build/agent_state/475_step2_source_fact_population_contract/contract.md
- ideas/closed/474_prepared_frame_slot_materialization_no_clobber_facts.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - `f.logic.end.14` condition `%t23`, slot `#21`: target identity is known,
    but no semantic materialization/write fact exists for `%t23 = ne i32 %t22,
    0`, and no path/no-clobber interval facts exist.
- Rejected evidence:
  - `%t22 -> %t23` stack move has `authority=none` and is storage movement, not
    semantic compare-result materialization.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- Directly populating `PreparedFrameSlotSourceFact` rows unless Step 1/2 prove
  this same source idea must expose a minimal bridge after lower-level facts
  exist.
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

Idea 475 could not populate source facts because the semantic producer layer is
missing. This plan owns that missing layer: semantic instruction-result
materialization/write records and interval no-clobber facts.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream source-fact and branch-stack-load authority unavailable until
  later packets consume explicit semantic facts.
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

### Step 1: Audit Semantic Materialization Interval Inputs

Inspect prepared evidence for semantic instruction-result identity,
frame-slot writes/materialization, path or edge validity, same-slot writes,
calls/helpers/inline asm, publications, move bundles, and parallel copies for
`%t23` slot `#21`. Completion means `todo.md` records whether a semantic
materialization/interval producer packet is justified and which rows remain
out of scope.

### Step 2: Define Semantic Materialization Interval Contract

Specify records/statuses for semantic instruction-result materialization/write
events and no-clobber intervals. Completion means `todo.md` records positive
and negative cases and names implementation/test surfaces if a bounded
producer packet is justified.

### Step 3: Implement Or Route Semantic Materialization Producer

If Step 2 finds a bounded producer packet, implement only that semantic
materialization/interval surface with focused coverage. If no implementation is
justified, record the precise blocker and route it. Completion means proof
passes or lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes source-fact population / other durable follow-up.
