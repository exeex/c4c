# Select-Edge ULE Source-Producer Rematerialization Plan

Status: Active
Source Idea: ideas/open/463_select_edge_ule_source_producer_rematerialization.md
Activated From: ideas/closed/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md

## Purpose

Classify and, if justified, implement the RV64 source-producer
rematerialization route for the successor-defined `%t46` value feeding `%t50`
on predecessor edge `logic.rhs.end.40 -> logic.end.41`.

## Goal

Decide whether `%t46 = bir.ule ptr %t42, %t45` can be safely rematerialized at
the predecessor edge for `%t46 -> %t50`, using explicit source-producer, edge,
carrier, and operand-authority facts.

## Core Rule

Do not treat successor-block `%t46` as predecessor-available from prepared
value homes alone. Any progress must come from a sound source-producer
rematerialization contract for `%t46 = bir.ule ptr %t42, %t45` and its
operands.

## Read First

- ideas/open/463_select_edge_ule_source_producer_rematerialization.md
- ideas/closed/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
- build/agent_state/462_step3_preterminator_parallel_copy_consumer/blocker.md
- build/agent_state/462_step4_residual_disposition/disposition.md
- build/agent_state/462_step1_preterminator_parallel_copy_audit/audit.md
- build/agent_state/462_step2_preterminator_parallel_copy_contract/contract.md
- src/backend/mir/riscv/codegen/object_emission.cpp

## Current Target

- source producer: `%t46 = bir.ule ptr %t42, %t45`
- destination/select result: `%t50`
- duplicate carriers: `%t50.phi.sel0` / `%t50.phi.sel1`
- edge: `logic.rhs.end.40 -> logic.end.41`
- predecessor block: `logic.rhs.end.40`
- successor/source-producer block: `logic.end.41`
- object-route event: `pre_terminator_copies`
- authority: `out_of_ssa_parallel_copy`
- rejected route: plain `%t46 -> %t50` predecessor-edge GPR copy

## Non-Goals

- Plain preterminator predecessor-edge register copies for successor-defined
  values.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Treating successor-block values as predecessor-available from value homes
  alone.
- Reopening ideas 456, 458, 459, 460, 461, or 462 without new
  coordinate-bearing evidence.
- Testcase-name, function-name, block-label, value-id-only, raw BIR-shape, or
  prepared-dump-order matching.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The remaining failure is not a move-copy availability problem; it is a
source-producer placement/rematerialization question. The route must explain
why the current source-producer consumer does not rematerialize the unsigned
pointer relational producer for `%t46`, whether duplicate carriers are a valid
carrier-only shape, and whether `%t42` / `%t45` can be consumed at the
predecessor edge.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must consume explicit source-producer, edge, carrier, and
  operand-authority facts.
- Preserve fail-closed behavior for missing, ambiguous, duplicate-unproven,
  non-edge, non-consumable, stale stack-load, and generic move cases.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a rematerialization packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Select-Edge Source-Producer Rejection

Re-read the 462 artifacts and current prepared/object output. Record why the
existing source-producer consumer rejects or misses `%t46 = bir.ule ptr %t42,
%t45` for `%t50` on edge `logic.rhs.end.40 -> logic.end.41`, including
carrier facts, operand homes, and authority facts. Completion means `todo.md`
contains the rejection/audit table and identifies whether a contract can be
defined or a producer/prepared blocker must be split.

### Step 2: Define ULE Source-Producer Rematerialization Contract

Specify the accepted route for rematerializing this source producer at the
predecessor edge: required edge identity, carrier proof, operand availability,
pointer comparison authority, RV64 operand forms, emission point, and
fail-closed cases. Completion means `todo.md` records the contract, target
files/tests if implementation is justified, or the exact blocker.

### Step 3: Implement Or Route First Rematerialization Packet

If Step 2 finds a bounded packet, implement only the coordinate/authority-backed
rematerialization route with focused coverage. If no implementation is
justified, record the precise blocker and route it to a separate source idea.
Completion means proof passes for the selected packet or lifecycle state
records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative object route and classify any remaining first
owner. Completion means the source idea closes, remains active with one exact
in-scope packet, or splits a separate durable follow-up.
