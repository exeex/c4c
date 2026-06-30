# RV64 ULE Select-Edge Rematerialization Consumer Plan

Status: Active
Source Idea: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
Activated From: ideas/closed/464_select_carrier_alias_metadata.md

## Purpose

Consume prepared select carrier-alias authority in the RV64 object route to
decide whether the `%t46 = bir.ule ptr %t42, %t45` source producer can be
rematerialized for the `%t46 -> %t50` predecessor-edge transfer.

## Goal

Advance the `20010329-1` select-edge residual only through explicit
`PreparedSelectCarrierAliasAuthorityRecords`, preserving fail-closed behavior
for raw alias inference, plain successor-defined copies, stale stack-loads, and
generic move support.

## Core Rule

Do not infer duplicate-carrier aliases from `.phi.sel` names, raw select
shape, value ids, block labels, function names, testcase names, or dump order.
The RV64 route must consume the prepared carrier-alias authority records and
must prove operands are target-consumable before emitting ULE rematerialization.

## Read First

- ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
- ideas/closed/464_select_carrier_alias_metadata.md
- build/agent_state/464_step4_residual_disposition/disposition.md
- build/agent_state/463_step4_residual_disposition/disposition.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/mir/riscv/codegen/object_emission.cpp

## Current Target

- authority records: `PreparedSelectCarrierAliasAuthorityRecords`
- source producer: `%t46 = bir.ule ptr %t42, %t45`
- selected source: `%t46`
- final join-transfer destination: `%t50`
- duplicate carriers: `%t50.phi.sel0` / `%t50.phi.sel1`
- edge: `logic.rhs.end.40 -> logic.end.41`
- operand check: `%t42` and `%t45` must be target-consumable at the
  predecessor edge

## Non-Goals

- Alias inference from `.phi.sel` names, raw select shape, value ids, block
  labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies or same-register no-ops for successor-defined
  `%t46`.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, 459, 460, 461, 462, 463, or 464 without new
  coordinate-bearing evidence.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The producer/prepared alias prerequisite is complete. The remaining question
is target-consumer soundness: whether RV64 can use the authority records to
rematerialize an unsigned pointer `ule` source producer at the predecessor
edge, using only target-consumable operands and rejecting adjacent raw or
generic move forms.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must consume explicit carrier-alias authority records and
  reject raw alias inference.
- Preserve fail-closed behavior for missing alias authority, wrong edge, wrong
  source producer, mismatched carrier/final result, non-consumable operands,
  stale stack-load authority, plain successor-defined copies, and generic move
  cases.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a consumer packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Carrier-Alias Consumer Surface

Inspect how `PreparedSelectCarrierAliasAuthorityRecords` are reachable from
the RV64 object route and how they match the coordinate-bearing
`pre_terminator_copies` event. Record the available authority keys, operand
homes for `%t42` / `%t45`, and current rejection point. Completion means
`todo.md` contains an audit table and identifies whether Step 2 can define a
consumer contract or must route to another blocker.

### Step 2: Define ULE Rematerialization Consumer Contract

Specify the accepted target-consumer route: authority record lookup, edge and
source-producer match, carrier/final-result match, operand target-consumability,
emission point, and fail-closed cases. Completion means `todo.md` records the
contract, target files/tests if implementation is justified, or the exact
blocker.

### Step 3: Implement Or Route First RV64 Consumer Packet

If Step 2 finds a bounded packet, implement only the carrier-authorized ULE
rematerialization consumer with focused coverage. If no implementation is
justified, record the precise blocker and route it to a separate source idea.
Completion means proof passes for the selected packet or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative object route and classify any remaining first
owner. Completion means the source idea closes, remains active with one exact
in-scope packet, or splits a separate durable follow-up.
