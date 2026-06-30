# Carrier Alias Identity Publication API Plan

Status: Active
Source Idea: ideas/open/468_carrier_alias_identity_publication_api.md
Activated From: ideas/open/467_unsupported_carrier_alias_planner_rejection.md

## Purpose

Resolve the caller/API boundary for publishing synthesized carrier-alias
identity so prepared carrier-alias authority and later object-route consumers
observe the same `ValueNameId` surface.

## Goal

Choose and prove a const-correct shared identity publication route before
resuming the idea 467 `unsupported_carrier_alias` planner repair.

## Core Rule

Do not hide name-table mutation behind `const PreparedNameTables&`, and do not
claim scratch-copy-only publication as authority for original-module consumers.
This plan owns the shared prepared identity/API boundary only.

## Read First

- ideas/open/468_carrier_alias_identity_publication_api.md
- ideas/open/467_unsupported_carrier_alias_planner_rejection.md
- build/agent_state/467_step1_unsupported_carrier_alias_audit/audit.md
- build/agent_state/467_step2_carrier_alias_acceptance_contract/contract.md
- build/agent_state/467_step3_carrier_alias_acceptance/blocker.md
- build/agent_state/467_step3_carrier_alias_acceptance/summary.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- synthesized aliases: `%t50.phi.sel0` / `%t50.phi.sel1`
- missing identity: durable shared `ValueNameId` publication for carrier
  aliases after semantic candidate checks pass
- rejected path: hidden `const_cast` mutation of `PreparedNameTables`
- insufficient path: scratch-copy const collector that does not update the
  original prepared module seen by RV64/original-module consumers
- acceptable boundary options:
  - explicit mutable publication stage before object-route consumers;
  - consumer-facing authority API that does not depend on scratch-copy names
    being inserted into the original name table

## Non-Goals

- RV64 ULE rematerialization or object lowering changes.
- Continuing the idea 467 planner repair before the shared identity/API
  boundary is selected.
- Hidden mutation through `const_cast` or writes behind
  `const PreparedNameTables&`.
- Scratch-copy-only publication claimed as sufficient for original-module
  consumers.
- Alias inference from raw `.phi.sel` spelling, raw select shape, value ids,
  block labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies, generic move-bundle support, stale stack-load
  consumption, expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The carrier-alias planner needs synthesized alias identities only after
producer-owned candidate checks pass. The problem is where that identity is
published so all consumers see the same prepared module state. A repair that
only mutates a temporary copy or hides mutation behind constness is not a
coherent authority surface.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Prefer the smallest API boundary that keeps identity publication explicit
  and shared.
- Preserve fail-closed behavior for raw alias inference, unavailable evidence
  rows, wrong edge/source/final result, stale stack-loads, and generic move
  cases.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if an API packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Carrier Alias Identity Callers

Inspect prepared carrier-alias collection callers, prepared dump paths, and
RV64/object-route access to prepared modules. Record which consumers see
const modules, which can run before object emission, and where identity can be
published once for all consumers. Completion means `todo.md` contains a caller
matrix and classifies the viable API route or exact blocker.

### Step 2: Define Shared Identity Publication Contract

Specify the selected boundary: mutable pre-consumer publication stage or
consumer-facing authority API independent of scratch-copy name insertion.
Completion means `todo.md` records the contract, fail-closed cases, and target
files/tests if implementation is justified.

### Step 3: Implement Or Route Identity API Packet

If Step 2 finds a bounded packet, implement only the shared identity/API
prerequisite with focused tests. If no implementation is justified, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Decide whether idea 467 can resume its planner repair, whether this identity
API idea is complete, or whether another prerequisite remains.
