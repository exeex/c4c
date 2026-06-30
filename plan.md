# Branch Site Stack-Slot Current-Value No-Clobber Certificate Plan

Status: Active
Source Idea: ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
Activated From: ideas/closed/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md

## Purpose

Produce explicit current-value and no-clobber certificates for branch-site
stack-slot loads.

## Goal

Enable later branch-stack-load availability only when a stack slot is proven to
contain the current branch value at the branch site and proven not to be
clobbered along the relevant path.

## Core Rule

Do not infer current-value or no-clobber safety from a stack home or carrier
field. A certificate must name explicit source identity, path validity,
stack-write exclusion, call/helper/inline-asm effects, and
publication/move-bundle/parallel-copy non-clobber facts.

## Read First

- ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
- ideas/closed/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
- build/agent_state/471_step4_residual_disposition/disposition.md
- build/agent_state/471_step3_freshness_clobber_producer/blocker.md
- build/agent_state/471_step2_freshness_clobber_contract/contract.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary scalar candidate:
  - `f.logic.end.14` condition `%t23`, slot `#21`: identity facts exist, but
    no current-value/no-clobber certificate exists.
- Boundary rows:
  - `%t22` remains a select-result stack-destination boundary.
  - `%t1` and `%t7` remain pointer-status/provenance boundaries.
  - `%t2` and `%t8` remain `unsupported_terminator` relationship boundaries.

## Non-Goals

- RV64 branch-load emission or target lowering.
- Consuming unavailable branch-stack-load records as target authority.
- Pointer-value/provenance repair.
- Select-result/block-entry stack-destination repair.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring current value or no-clobber safety from stack homes, offsets,
  object ids, raw BIR shape, block labels, function names, testcase names, or
  dump order.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 471 proved the freshness/clobber fields are structurally available but
found no producer certificate for real rows. This plan owns that missing source:
the proof that a named stack slot contains the current branch value and is not
invalidated before the branch-site load.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Keep branch-stack-load records fail-closed until certificates are explicit.
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

### Step 1: Audit Current-Value No-Clobber Certificate Inputs

Inspect prepared evidence for source identity, path/dominance validity,
stack-write effects, call/helper/inline-asm effects, publications, move
bundles, and parallel copies around the representative branch rows. Completion
means `todo.md` records whether `%t23` slot `#21` can receive a certificate and
which boundary rows remain out of scope.

### Step 2: Define Certificate Contract

Specify the exact facts required for a current-value/no-clobber certificate and
the fail-closed statuses for missing or unknown evidence. Completion means
`todo.md` records positive and negative cases and names implementation/test
surfaces if a bounded producer packet is justified.

### Step 3: Implement Or Route Certificate Producer

If Step 2 finds a bounded producer packet, implement only that certificate
surface with focused coverage. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative records and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope packet, or routes branch-stack-load availability / RV64 consumption /
other durable follow-up.
