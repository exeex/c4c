# RV64 Prepared Global Value-Location Consumer Runbook

Status: Active
Source Idea: ideas/open/621_rv64_prepared_global_value_location_consumer.md

## Purpose

Execute idea 621 as a narrow RV64 consumer route for prepared global-memory
loads and stores whose producer authority is already complete.

## Goal

Teach the RV64 object route to consume supported prepared global-memory facts
when value locations move through prepared register and frame-slot homes.

## Core Rule

Consume explicit prepared/global authority only. Do not reconstruct producer
facts in RV64 and do not add testcase-shaped handling for named torture rows,
specific globals, value ids, or final assembly shapes.

## Read First

- `ideas/open/621_rv64_prepared_global_value_location_consumer.md`
- Existing prepared/global and RV64 object-route diagnostics for
  `src/pr36034-1.c` and `src/pr91137.c`
- Any current backend summary or case logs the supervisor designates as proof
  inputs

## Current Targets

- Prepared global loads and stores with complete global identity, offset,
  width, extent, layout authority, and supported addressing facts.
- Value-location paths where prepared global values move through GPR/FPR
  registers or prepared frame-slot homes.
- Representative rows:
  - `src/pr36034-1.c`
  - `src/pr91137.c`
- Guard-only candidates from idea 611, until refreshed producer facts prove
  they belong here:
  - `src/ieee/20001122-1.c`
  - `src/991030-1.c`

## Non-Goals

- Prepared/global producer authority.
- Selected object-data authority.
- Direct global-symbol base-plus-offset authority.
- Mixed object-data slot authority.
- BIR initializer bootstrap.
- Local-memory, ABI, runtime/link, expectation, unsupported-marker, allowlist,
  timeout, or accounting changes.

## Working Model

Idea 608 closed producer-side prepared global authority. This plan starts by
refreshing the current rows to confirm the first remaining stop is RV64
prepared-global value-location consumption. Implementation may proceed only for
rows where producer facts are complete and the unsupported shape is the RV64
consumer path.

## Execution Rules

- Start every code-changing packet from fresh diagnostics, not stale queue
  labels.
- Keep fail-closed checks for absent, ambiguous, unsupported, or incomplete
  prepared/global facts.
- Prefer one semantic consumer path that covers multiple rows or value-location
  shapes over a narrow named-case fix.
- If refreshed evidence assigns a representative row to another owner, record
  the classification in `todo.md` and keep the implementation packet out of
  that row.
- For code changes, prove with build output plus the supervisor-delegated
  backend/object-route subset. Escalate to broader backend regression when the
  touched surface is shared across RV64 global-memory consumers.

## Steps

### Step 1: Refresh Prepared-Global Consumer Ownership

Goal: Prove which current rows still belong to this RV64 consumer route.

Concrete actions:

- Run focused direct RV64 object-route or backend diagnostics for
  `src/pr36034-1.c` and `src/pr91137.c`.
- Inspect whether each row has complete prepared/global producer facts:
  global identity, offset, width, extent, layout authority, and supported
  addressing.
- Identify the value-location shape for each row: GPR, FPR, prepared frame-slot
  home, aggregate lane, or unsupported/ambiguous shape.
- Probe `src/ieee/20001122-1.c` and `src/991030-1.c` only as guard rows unless
  current facts prove complete prepared/global authority.
- Record any row that belongs to producer authority, local memory, ABI,
  runtime, direct global-symbol policy, or another owner in `todo.md` instead
  of widening this plan.

Completion check:

- `todo.md` names the in-scope row family, first failing owner, value-location
  shapes, and rows excluded from implementation with current evidence.

### Step 2: Add Focused Positive And Fail-Closed Coverage

Goal: Lock the expected RV64 consumer contract before or alongside the first
implementation packet.

Concrete actions:

- Add backend/object-route tests for at least one floating global-load
  frame-slot path and one integer or aggregate prepared global load/store path
  when refreshed diagnostics show both are in scope.
- Add fail-closed coverage for missing producer facts and unsupported
  value-location shapes.
- Keep tests semantic; do not assert behavior by source filename, final symbol
  spelling, or a single known global name.

Completion check:

- Focused tests fail for the current unsupported consumer behavior or pass only
  when the implementation in the same packet legitimately enables the covered
  consumer path.

### Step 3: Implement Prepared Global Value-Location Consumption

Goal: Lower complete prepared global-memory facts through supported prepared
register and frame-slot value locations in the RV64 object route.

Concrete actions:

- Locate the RV64 prepared global load/store consumer and its current
  fail-closed diagnostic path.
- Accept only complete prepared/global facts and supported addressing modes.
- Materialize the load/store through explicit prepared GPR/FPR/register or
  frame-slot homes, preserving width and offset checks.
- Emit precise diagnostics for missing authority, unsupported value-location
  kinds, ambiguous homes, unsupported widths, or incomplete layout facts.
- Avoid moving producer-authority reconstruction into RV64.

Completion check:

- More than one refreshed prepared-global consumer row progresses past the old
  value-location stop, or the row family is narrowed to a documented downstream
  owner with current proof.

### Step 4: Validate Breadth And Guards

Goal: Prove the route fixed a semantic consumer gap without weakening producer
authority or absorbing adjacent owners.

Concrete actions:

- Rerun focused probes for `src/pr36034-1.c` and `src/pr91137.c`.
- Rerun any guard rows classified in Step 1.
- Run the supervisor-delegated backend subset, normally including the focused
  tests added for this plan and a broader `^backend_` check if shared backend
  consumer code changed.
- Confirm fail-closed diagnostics still reject absent or incomplete
  prepared/global authority.

Completion check:

- Proof logs show the target family progresses or is defensibly reclassified,
  guard rows remain outside this route, and backend regression proof is ready
  for supervisor review.

### Step 5: Close-Readiness Classification

Goal: Decide whether idea 621 is complete or needs a follow-up source idea.

Concrete actions:

- Summarize target progress, excluded rows, remaining downstream owners, and
  fail-closed guard results in `todo.md`.
- Verify the source idea acceptance criteria are satisfied: multiple-row
  progress or narrower downstream classification, preserved producer gates, and
  object-route proof for both representative shapes unless current evidence
  excludes one.
- If a separate initiative is discovered, request lifecycle split instead of
  silently expanding this plan.

Completion check:

- `todo.md` contains enough evidence for the supervisor to request closure,
  split, or another implementation packet without rereading all diagnostics.
