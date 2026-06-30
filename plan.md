# Branch Stack-Load Policy Freshness Plan

Status: Active
Source Idea: ideas/open/470_branch_stack_load_policy_freshness.md
Activated From: ideas/closed/469_branch_stack_load_authority_metadata.md

## Purpose

Produce branch-site load policy, freshness, and clobber-safety facts for
prepared branch-stack-load authority records.

## Goal

Turn eligible scalar branch-stack-load records from unavailable to available
only when `load_from_stack_slot` policy, freshness, and clobber-safety are
explicitly proven.

## Core Rule

Do not treat populated branch-stack-load records as target-consumable authority
until policy, freshness, and clobber-safety are present. Keep pointer
provenance and select-result stack-destination owners separate.

## Read First

- ideas/open/470_branch_stack_load_policy_freshness.md
- ideas/closed/469_branch_stack_load_authority_metadata.md
- build/agent_state/469_step6_residual_disposition/disposition.md
- build/agent_state/469_step5_branch_stack_load_record_population/summary.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidates:
  - `f.block_1` condition `%t2`: currently `unsupported_terminator`.
  - `f.block_4` condition `%t8`: currently `unsupported_terminator`.
  - `f.logic.end.14` condition `%t23`: currently `missing_policy`.
- Pointer and adjacent boundaries:
  - `%t1` / `%t7` remain unavailable until pointer status is explicit.
  - `%t22` remains a select-result/block-entry stack-destination boundary.

## Non-Goals

- RV64 branch-load emission or target lowering.
- Pointer-value/provenance repair for `%t7` or external formal provenance.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Treating pointer operands as proven when `pointer_status=unknown`.
- Accepting raw BIR shape, stack-slot spelling, offsets, object ids, block
  order, filenames, function names, or testcase shape as load authority.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 469 made unavailable records visible. This plan owns the next producer
facts: when the branch site can load the stack slot, whether the slot is fresh,
and whether no clobber occurs before the branch. Available records may then
enable a later RV64 consumer, but this plan does not implement target loads.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep existing unavailable records fail-closed until all policy/freshness/
  clobber facts are explicit.
- Preserve pointer-status and select-result stack-destination boundaries.
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

### Step 1: Audit Branch-Site Policy Freshness Inputs

Inspect current branch-stack-load records and prepared control-flow/store
facts. Classify which representative rows can receive branch-site load policy,
freshness, and clobber-safety, and which remain blocked by pointer provenance
or select-result stack destinations. Completion means `todo.md` records an
audit table and the first exact producer packet or blocker.

### Step 2: Define Policy Freshness Contract

Specify the required facts for `load_from_stack_slot` policy, freshness,
clobber-safety, and fail-closed statuses. Completion means `todo.md` records
positive/negative cases and names target files/tests if implementation is
justified.

### Step 3: Implement Or Route Policy Freshness Packet

If Step 2 finds a bounded producer packet, implement only that policy/freshness
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative records and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes an RV64 consumer / other durable follow-up.
