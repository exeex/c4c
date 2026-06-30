# Prepared Frame-Slot Materialization No-Clobber Facts Plan

Status: Active
Source Idea: ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md
Activated From: ideas/closed/473_branch_site_stack_slot_materialization_no_clobber_source.md

## Purpose

Create the lower-level prepared source-fact surface needed for later
branch-site stack-slot certificate production.

## Goal

Publish frame-slot materialization/write records plus path validity,
same-slot write exclusion, effect safety, and publication/move/parallel-copy
non-clobber classifications.

## Core Rule

Do not set downstream branch-stack-load authority availability in this plan.
This plan owns only independent prepared source facts.

## Read First

- ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md
- ideas/closed/473_branch_site_stack_slot_materialization_no_clobber_source.md
- build/agent_state/473_step4_residual_disposition/disposition.md
- build/agent_state/473_step3_materialization_no_clobber_source/blocker.md
- build/agent_state/473_step2_materialization_no_clobber_contract/contract.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - `f.logic.end.14` condition `%t23`, slot `#21`: branch/load identity exists,
    but no independent materialization/write event, path/dominance proof,
    same-slot write exclusion, effect safety, or publication/move/copy
    non-clobber source fact exists.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

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

Idea 473 proved there is no exact branch-site source-fact implementation packet
without a lower-level carrier. This plan owns that carrier and producer
surface, not downstream consumers.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep downstream branch-stack-load authority unavailable until a later packet
  consumes explicit source facts.
- Preserve pointer-status, select-result, and unsupported-terminator
  boundaries.
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

### Step 1: Audit Prepared Source-Fact Carrier Requirements

Inspect prepared/prealloc surfaces for frame-slot writes, value homes, stack
objects, calls/helpers/inline asm, publications, move bundles, and parallel
copies. Completion means `todo.md` records the required carrier fields,
statuses, and first exact producer packet or blocker.

### Step 2: Define Prepared Source-Fact Contract

Specify records/statuses for materialization/write events, source value and
frame-slot/object identity, path validity, same-slot write exclusion, effect
safety, and publication/move/copy non-clobber classification. Completion means
`todo.md` records positive/negative cases and implementation/test surfaces if
a bounded producer packet is justified.

### Step 3: Implement Or Route Prepared Source-Fact Producer

If Step 2 finds a bounded producer packet, implement only that source-fact
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative facts and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes downstream branch-site stack-slot source-fact /
certificate / authority follow-up.
