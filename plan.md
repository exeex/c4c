# Branch Site Stack-Slot Materialization No-Clobber Source Plan

Status: Active
Source Idea: ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md
Activated From: ideas/closed/472_branch_site_stack_slot_current_value_no_clobber_certificate.md

## Purpose

Produce lower-level prepared source facts for branch-site stack-slot
materialization/current-value and no-clobber safety.

## Goal

Enable downstream current-value/no-clobber certificate production only when
source value, exact frame-slot materialization or write, path validity,
same-slot write exclusion, call/helper/inline-asm safety, and
publication/move/parallel-copy non-clobber facts are explicit.

## Core Rule

Do not mark downstream branch-stack-load records available from this plan. This
plan owns only source facts that a later certificate/authority packet may
consume.

## Read First

- ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md
- ideas/closed/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
- build/agent_state/472_step4_residual_disposition/disposition.md
- build/agent_state/472_step3_current_value_no_clobber_producer/blocker.md
- build/agent_state/472_step2_current_value_no_clobber_contract/contract.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - `f.logic.end.14` condition `%t23`, slot `#21`: identity facts exist, but no
    exact frame-slot materialization/current-value or same-slot no-clobber
    source fact exists.
- Boundary rows:
  - `%t22` remains a select-result stack-destination and block-entry
    publication boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- RV64 branch-load emission or target lowering.
- Populating available `PreparedBranchStackLoadAuthority` records directly.
- Consuming unavailable branch-stack-load records as target authority.
- Pointer-value/provenance repair.
- Select-result/block-entry stack-destination repair.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring source facts from stack homes, offsets, object ids, raw BIR shape,
  value names, block labels, function names, testcase names, or dump order.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 472 proved certificate production cannot proceed without lower-level
source facts. This plan owns only those source facts: materialization/current
value and no-clobber evidence for a specific frame slot at a specific branch
site.

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

### Step 1: Audit Materialization No-Clobber Source Inputs

Inspect prepared evidence for exact source value, frame-slot writes, stack
objects, path/dominance relations, same-slot writes, calls/helpers/inline asm,
publications, move bundles, and parallel copies around `%t23` slot `#21`.
Completion means `todo.md` records whether a source-fact producer packet is
justified and which boundary rows remain out of scope.

### Step 2: Define Source Fact Contract

Specify source-fact records/statuses for exact materialization/current value,
path validity, same-slot write exclusion, call/helper/inline-asm safety, and
publication/move/parallel-copy non-clobber. Completion means `todo.md` records
positive and negative cases and names implementation/test surfaces if a bounded
producer packet is justified.

### Step 3: Implement Or Route Source Fact Producer

If Step 2 finds a bounded producer packet, implement only that source-fact
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative records and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes downstream certificate/authority / other durable
follow-up.
