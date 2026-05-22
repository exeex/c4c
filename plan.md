# AArch64 Pointer-Derived Load Address Scaling Refresh Runbook

Status: Active
Source Idea: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md

## Purpose

Refresh whether idea 362 still has a live AArch64 pointer-derived load or
address-scaling first bad fact after the current tree's broader Hanoi and
memory-lowering repairs.

## Goal

Determine whether the source idea can be closed, must remain parked, or needs
fresh implementation work because the pointer-derived load/address-scaling
timeout owner has reappeared.

## Core Rule

Do not reopen materialized pointer store writeback, direct `LoadGlobal`
select-store handling, recursive formal post-call repairs, semantic string
loads, frontend admission, ABI composite work, or variadic/floating residuals
unless fresh generated-code evidence proves the current first bad fact moved
there and lifecycle state is split accordingly.

## Read First

- `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
- Any current generated/debug artifacts for `00181` only after the focused
  refresh proof shows a live failure.

## Current Targets

- AArch64 pointer-derived load address scaling in the `00181` Hanoi path.
- Stability for idea 360 starting-state output and idea 361 materialized
  pointer store writeback.
- Focused nearby memory operand and representative coverage for `00170`,
  `00181`, and `00189`.

## Non-Goals

- No implementation edits unless Step 1 exposes a live in-scope failure.
- No expectation, timeout, runner, CTest-registration, unsupported, or
  proof-log-policy changes.
- No filename-specific handling for `00181`, `Hanoi`, one tower, one source
  line, one stack offset, one ABI register, or one emitted instruction
  sequence.
- No source-idea rewrite unless closure, deactivation, or a real lifecycle
  split requires durable metadata.

## Working Model

The old owner was pointer-derived load/address scaling after materialized
pointer stores began writing back correctly. It was repaired by preserving the
live index/result carrier while materializing immediate scales. The most
recent refresh showed the focused proof passing 9/9 and no live in-scope first
bad fact, but closure was rejected because the strict close gate requires a
before/after pass-count increase.

## Execution Rules

- Treat this as a closure-refresh runbook first, not an implementation
  runbook.
- If the focused proof is green, record the result in `todo.md` and route to a
  plan-owner close/deactivate decision.
- If a focused test fails, classify the first bad fact from generated-code or
  dump evidence before editing code.
- If the first bad fact is outside pointer-derived load/address scaling,
  record the handoff in `todo.md` and ask lifecycle to split or switch.
- If implementation is needed, add focused backend coverage before relying on
  the external representative.

## Steps

### Step 1: Refresh Current Pointer-Derived Load Proof

Goal: determine whether a live in-scope failure exists on the current tree.

Actions:

- Build the current tree.
- Run the focused pointer-derived load/address-scaling and memory-lowering
  representative subset selected by the supervisor.
- Preserve the exact command and result in `test_after.log`.
- Update `todo.md` with the pass/fail summary and first observable bad fact.

Completion Check:

- The selected proof has been run fresh.
- `todo.md` records whether any current failure is in scope for idea 362.

### Step 2: Classify Or Repair The First Bad Fact

Goal: respond only if Step 1 exposes a live failure.

Actions:

- If Step 1 is green, skip implementation and recommend Step 3.
- If `00181` or focused coverage fails, compare semantic BIR, prepared BIR,
  and generated AArch64 around the producing pointer value, scale/index
  calculation, emitted load address, and timeout-causing consumer.
- Classify whether the first bad fact is pointer-derived load/address scaling
  or an adjacent owner.
- For an in-scope owner, repair the general lowering rule and add focused
  backend coverage.
- For an out-of-scope owner, stop with a lifecycle handoff note instead of
  widening this runbook.

Completion Check:

- Either no implementation was needed, or the first bad fact was classified
  and repaired/reassigned with focused proof.

### Step 3: Close Or Deactivate Decision

Goal: decide lifecycle state after the refreshed proof and classification.

Actions:

- If the source idea is complete, run the required close gate through
  `c4c-regression-guard` using matching canonical logs.
- Close only if source completion is true and the close gate passes.
- If source scope appears satisfied but the strict close gate rejects, leave
  the idea parked/open and deactivate the active runbook.
- If fresh implementation or handoff scope remains, keep or switch lifecycle
  state according to the classified owner.

Completion Check:

- There is no ambiguous active lifecycle state: the idea is closed, the
  runbook remains active for real in-scope work, or the runbook is deactivated
  with a durable parked/open reason.
