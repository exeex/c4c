# Unsupported Carrier Alias Planner Rejection Plan

Status: Active
Source Idea: ideas/open/467_unsupported_carrier_alias_planner_rejection.md
Activated From: ideas/closed/466_representative_select_carrier_alias_authority.md

## Purpose

Diagnose or repair the prepared carrier-alias planner rejection for the real
`20010329-1` `%t46 -> %t50` row.

## Goal

Explain why `carrier_alias_candidates=2` becomes `carrier_aliases=0` with
`status=unsupported_carrier_alias`, and publish available authority only if the
duplicate carrier candidates are semantically valid aliases of `%t50`.

## Core Rule

Do not treat unavailable evidence rows as authority and do not infer aliases
from `%*.phi.sel*` names, raw select shape, value ids, block labels, testcase
names, or dump order. This plan owns prepared planner/producer semantics only,
not RV64 lowering.

## Read First

- ideas/open/467_unsupported_carrier_alias_planner_rejection.md
- ideas/closed/466_representative_select_carrier_alias_authority.md
- ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
- build/agent_state/466_step3_representative_authority_evidence/summary.md
- build/agent_state/466_step4_residual_disposition/disposition.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- representative: `20010329-1`
- row status: `unsupported_carrier_alias`
- predecessor/successor: `logic.rhs.end.40 -> logic.end.41`
- destination: `%t50`, `destination_value_id=21`
- source: `%t46`, `source_value_id=20`
- source producer: `binary`, `logic.end.41`, `source_producer_inst=1`
- carrier candidates: `2`
- accepted aliases: `0`
- use closure: `source_use_closure=no`

## Non-Goals

- RV64 ULE rematerialization changes before the representative has an
  available authority record.
- Treating unavailable evidence rows as authority.
- Alias inference from `.phi.sel` names, raw select shape, value ids, block
  labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies or same-register no-ops for successor-defined
  `%t46`.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The evidence/probe surface is sufficient. The remaining question is why the
prepared planner rejects two carrier candidates before publication. If those
candidates are semantically valid aliases, the planner may publish available
authority. If not, the exact semantic blocker should be recorded and routed.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must use producer-owned join-transfer/source-producer
  facts, not raw carrier names or raw select shape.
- Preserve fail-closed behavior for wrong final result, wrong edge, wrong
  source, extra non-carrier uses, raw-name-only shape, unavailable evidence
  rows, stale stack-loads, and generic move cases.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a planner/producer packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Unsupported Carrier Alias Rejection

Inspect the planner path for the representative row and record why two carrier
candidates are rejected as `unsupported_carrier_alias`. Completion means
`todo.md` contains a rejection audit table and identifies the first exact
planner/producer packet or blocker.

### Step 2: Define Carrier Alias Acceptance Contract

Specify the semantic conditions that turn duplicate candidates into available
carrier aliases for one final join-transfer result, including source-use
closure and fail-closed cases. Completion means `todo.md` records positive and
negative contract cases and names target files/tests if implementation is
justified.

### Step 3: Implement Or Route Planner Rejection Packet

If Step 2 finds a bounded producer/planner repair, implement only that packet
with focused coverage. If the rejection is semantically correct, record the
precise blocker and route it. Completion means proof passes or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative authority evidence and classify any remaining
first owner. Completion means this source idea closes, remains active with one
exact in-scope packet, or routes back to idea 465 / another durable follow-up.
