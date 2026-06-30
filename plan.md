# Representative Select Carrier Alias Authority Plan

Status: Active
Source Idea: ideas/open/466_representative_select_carrier_alias_authority.md
Activated From: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md

## Purpose

Find and repair, or precisely classify, why the real `20010329-1`
representative does not publish or match a carrier-alias authority record for
the `%t46 -> %t50` select-edge route.

## Goal

Make the representative carrier-alias authority path explicit enough that idea
465 can either resume as a target consumer or stay parked behind a precise
producer/prepared blocker.

## Core Rule

Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value ids,
block labels, function names, testcase names, or dump order. The only accepted
path is producer-owned carrier-alias authority for the real representative, or
a precise blocker explaining why that authority cannot be produced or matched.

## Read First

- ideas/open/466_representative_select_carrier_alias_authority.md
- ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
- build/agent_state/465_step4_residual_disposition/disposition.md
- build/agent_state/465_step4_residual_disposition/20010329-1.prepared.out
- build/agent_state/465_step4_residual_disposition/20010329-1.object.err
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/mir/riscv/codegen/object_emission.cpp

## Current Target

- representative: `20010329-1`
- object failure: same `pre_terminator_copies` coordinate for
  `logic.rhs.end.40 -> logic.end.41`
- source producer: `%t46 = bir.ule ptr %t42, %t45`
- selected source / destination: `%t46 -> %t50`
- carriers: `%t50.phi.sel0` / `%t50.phi.sel1`
- join transfer: `logic.end.41 result=%t50`
- operands: `%t42` home `s1`, `%t45` home `s2`
- missing evidence: no printed `carrier_alias` / `select_carrier` authority
  record in the fresh prepared dump

## Non-Goals

- RV64 ULE rematerialization changes before representative authority is proven
  present and matchable.
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

The Step 3 RV64 consumer in idea 465 is valid when explicit authority records
exist. The representative remains blocked because the authority record is not
visible or not matched for the real module. This plan owns the producer/probe
question: missing record, present-but-not-visible record, or
present-but-RV64-mismatched record.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must publish, expose, or prove producer-owned authority,
  not raw-name or raw-shape inference.
- Preserve fail-closed behavior for missing source producer, wrong edge,
  mismatched carrier/final result, extra non-carrier uses, stale stack-loads,
  plain successor-defined copies, and generic move cases.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a producer/probe packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Representative Authority Collection

Inspect the real `20010329-1` prepared module and
`collect_prepared_select_carrier_alias_authorities` path. Determine whether a
matching authority record is produced, not produced, hidden from the printed
probe, or produced with fields the RV64 consumer cannot match. Completion
means `todo.md` contains an audit table and identifies the first exact packet
or blocker.

### Step 2: Define Representative Authority Evidence Contract

Specify the required record or probe evidence for the representative:
function, edge, join transfer, source producer, selected source, destination,
carrier values, and use closure. Completion means `todo.md` records positive
and negative cases and names target files/tests if implementation is justified.

### Step 3: Implement Or Route Representative Authority Packet

If Step 2 finds a bounded producer/collector/printer packet, implement only
that packet with focused coverage. If the record already exists but RV64
matching is wrong, route the exact consumer packet back to idea 465. If no
implementation is justified, record the precise blocker and route it.
Completion means proof passes or lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative and classify any remaining first owner. Completion
means this source idea closes, remains active with one exact in-scope packet,
or routes back to idea 465 / another durable follow-up.
