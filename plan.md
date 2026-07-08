# BIR Global Initializer Bootstrap Runbook

Status: Active
Source Idea: ideas/open/606_bir_global_initializer_bootstrap.md

## Purpose

Repair BIR semantic production for ordinary aggregate and byte global initializer shapes that currently stop before prepared/global handoff.

## Goal

Move multiple RV64 gcc_torture global initializer bootstrap rows beyond their original BIR producer stop while preserving separate ownership for prepared/global authority and RV64/global consumers.

## Core Rule

Fix only the BIR semantic bootstrap for global initializer bytes and aggregate initializer shapes. Do not implement prepared global data authority, RV64 symbol/data emission, string/library policy, runtime behavior, expectations, unsupported markers, allowlists, timeouts, or accounting.

## Read First

- ideas/open/606_bir_global_initializer_bootstrap.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md

## Current Targets

- Owning layer: BIR semantic producer.
- Evidence breadth from source idea: `38` global initializer bootstrap rows plus `2` string-pool bootstrap rows.
- Proof surface: global initializer semantic bootstrap rows that stop before prepared/global authority.
- Adjacent owner risk: prepared global object-data authority, RV64 global symbol emission, string library semantics, runtime/link behavior, expectations, unsupported markers, allowlists, timeouts, and accounting.

## Non-Goals

- Do not implement prepared/global object-data authority.
- Do not implement RV64 global symbol or data consumers.
- Do not treat string-pool, string-library, runtime, or link-policy rows as initializer bootstrap progress.
- Do not change expected outputs, unsupported markers, allowlists, timeouts, or accounting.
- Do not special-case `src/20040302-1.c` or any other named testcase.

## Working Model

- Treat a row as in scope only when the first missing fact is BIR global initializer bootstrap for bytes or aggregate initializer structure.
- Use representative rows as probes, then require same-family breadth before closure evidence.
- Preserve a clear handoff to prepared/global authority and RV64/global consumer ideas.
- If evidence shows a row depends on target-side symbol emission, object-data authority, string policy, or runtime/link behavior, record it as adjacent ownership instead of pulling it into this runbook.

## Execution Rules

- Keep packet progress, selected rows, and refreshed evidence in `todo.md`; rewrite this runbook only for a real route correction.
- Each code-changing step needs at least `cmake --build --preset default` plus the supervisor-delegated RV64 gcc_torture proof subset.
- Prefer narrow representative proof first, then broaden to the same-family rows used for closure evidence.
- Reject testcase-overfit routes even when a selected row becomes green.

## Ordered Steps

### Step 1: Select Initializer Bootstrap Proof Rows

Goal: identify representative global initializer bootstrap rows plus guard rows from adjacent owners.

Actions:
- Inspect current RV64 gcc_torture scan artifacts and per-case logs for global initializer bootstrap stops.
- Refresh the starting row list if source evidence is stale.
- Select more than one in-scope byte or aggregate initializer row for narrow proof.
- Select guard rows for prepared/global authority, RV64/global consumers, string policy, runtime/link behavior, and expectation/accounting ownership where available.
- Record selected rows, expected pre-fix outcomes, and the exact supervisor proof command in `todo.md`.

Completion check:
- `todo.md` names representative proof rows, guard rows, starting expectations, refreshed evidence location, and delegated proof command.

### Step 2: Trace The BIR Producer Boundary

Goal: prove that the selected failures belong to BIR global initializer bootstrap rather than prepared/global or RV64/global consumers.

Actions:
- Compare HIR, BIR, and LLVM-route behavior for the selected rows.
- Identify the first producer diagnostic and the lowering functions/files that emit it.
- Separate byte and aggregate initializer bootstrap gaps from prepared object-data authority, RV64 symbol/data emission, string policy, runtime/link, and expectation/accounting failures.
- Record any excluded rows as guard or adjacent-owner evidence in `todo.md`.

Completion check:
- `todo.md` records the owning BIR producer path, missing initializer fact, and rows excluded from Step 3 implementation ownership.

### Step 3: Repair In-Scope Initializer Bootstrap

Goal: implement the narrow BIR producer behavior needed for the selected initializer bootstrap subfamily.

Primary target:
- BIR global initializer lowering code that can publish the missing byte or aggregate initializer facts before prepared/global handoff.

Actions:
- Implement the minimal semantic producer change for the selected subfamily.
- Preserve existing diagnostics and owner boundaries for prepared/global authority, RV64/global consumers, string policy, runtime/link behavior, expectations, unsupported markers, allowlists, timeouts, and accounting.
- Build and run the supervisor-delegated narrow proof.
- Stop for plan review if the required fix becomes prepared object-data authority, RV64 symbol/data emission, runtime/link behavior, string-library policy, or named-case logic.

Completion check:
- Build passes.
- More than one selected initializer bootstrap row progresses beyond the original BIR producer stop or reaches a defensible downstream owner.
- Guard rows keep their established ownership.

### Step 4: Prove Same-Family Breadth

Goal: show that the repair generalizes across initializer bootstrap rows without collapsing adjacent owner boundaries.

Actions:
- Rerun the narrow proof if Step 3 changed after first validation.
- Run the broader same-family RV64 gcc_torture backend-object subset requested by the supervisor.
- Compare current row outcomes against the refreshed Step 1 evidence.
- Record progressed rows, remaining initializer bootstrap limitations, adjacent-owner rows, and guard preservation in `todo.md`.

Completion check:
- `todo.md` summarizes same-family movement, remaining in-scope limitations, preserved adjacent owners, and any downstream handoffs.

### Step 5: Closure Readiness Summary

Goal: make the lifecycle handoff clear enough for the supervisor to decide close, rewrite, or continue.

Actions:
- Summarize implementation surface and proof results.
- Separate remaining initializer bootstrap limitations from prepared/global, RV64/global, string policy, runtime/link, and expectation/accounting owners.
- State whether the source idea acceptance criteria are satisfied.
- Recommend the next lifecycle action.

Completion check:
- `todo.md` contains final evidence, remaining limitations, and a clear closure-readiness recommendation.

## Acceptance Gate

This plan is complete only when multiple global initializer bootstrap rows progress beyond the old BIR producer stop, prepared/global and RV64/global rows remain separate first-owner failures until their own ideas are activated, and proof does not depend on expected-output, unsupported-marker, or allowlist changes.
