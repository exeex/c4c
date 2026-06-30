# Branch Site Stack-Slot Freshness Clobber Safety Metadata Plan

Status: Active
Source Idea: ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
Activated From: ideas/closed/470_branch_stack_load_policy_freshness.md

## Purpose

Produce explicit branch-site stack-slot freshness and clobber-safety facts for
prepared branch-stack-load authority records.

## Goal

Turn eligible scalar branch-stack-load records from unavailable to available
only when the stack slot is proven fresh at the branch site and proven safe from
clobber before the load.

## Core Rule

Do not set `load_from_stack_slot` authority from carrier capacity alone. The
branch-site load policy, stack-slot freshness, and clobber-safety facts must all
be explicit before any later RV64 consumer can use the record.

## Read First

- ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
- ideas/closed/470_branch_stack_load_policy_freshness.md
- build/agent_state/470_step4_residual_disposition/disposition.md
- build/agent_state/470_step3_policy_freshness_population/blocker.md
- build/agent_state/470_step2_policy_freshness_contract/contract.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - `f.logic.end.14` condition `%t23`: populated value/home/slot/object facts,
    currently `status=missing_policy`.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- RV64 branch-load emission or target lowering.
- Consuming unavailable branch-stack-load records as target authority.
- Pointer-value/provenance repair.
- Select-result/block-entry stack-destination repair.
- Inferring freshness or clobber safety from stack homes, offsets, object ids,
  raw BIR shape, block labels, function names, testcase names, or dump order.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 470 proved the carrier can express the needed facts but found no producer
authority to populate them for representative rows. This plan owns that missing
producer evidence: whether the exact branch-site stack slot contains the
current value and whether no intervening operation can clobber it.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep unavailable records fail-closed until freshness and clobber safety are
  explicitly proven.
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

### Step 1: Audit Branch-Site Freshness And Clobber Inputs

Inspect current prepared records, stack-home facts, branch-site control-flow
facts, stack writes, calls/helpers, publications, and move bundles around the
representative rows. Completion means `todo.md` records which rows can be
considered for freshness/clobber-safety production and which remain owned by
pointer, select-result, or unsupported-terminator prerequisites.

### Step 2: Define Freshness Clobber-Safety Contract

Specify the exact facts required to prove a stack slot is fresh at a branch
site and safe from clobber before a load. Completion means `todo.md` records
positive and negative cases and names implementation/test surfaces if a bounded
producer packet is justified.

### Step 3: Implement Or Route Freshness Clobber-Safety Producer

If Step 2 finds a bounded producer packet, implement only that producer surface
with focused coverage. If no implementation is justified, record the precise
blocker and route it. Completion means proof passes or lifecycle state records
the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative records and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes RV64 branch-load consumption / other durable
follow-up.
