# BIR Local-Memory Alloca And Scalar Semantics Runbook

Status: Active
Source Idea: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md

## Purpose

Repair BIR production for alloca-backed local memory and scalar/local-memory mixed semantics after the primary local-memory load, store, and GEP producer boundaries are explicit.

## Goal

Move multiple alloca or scalar/local-memory mixed RV64 gcc_torture rows beyond their original BIR producer stops while preserving existing ownership boundaries for load, store, GEP, prepared authority, and RV64 consumers.

## Core Rule

Fix only the BIR semantic producer for alloca-backed local memory and scalar/local-memory mixed cases. Do not infer RV64 consumer behavior, broaden stack-frame or ABI lowering, rewrite expectations, or special-case a named testcase.

## Read First

- ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md

## Current Targets

- Owning layer: BIR semantic producer.
- Evidence breadth from source idea: `12` alloca rows plus `26` scalar/local-memory mixed rows.
- Proof surface: RV64 gcc_torture backend-object rows whose first reproduced stop is alloca local-memory or scalar/local-memory mixed semantic production.
- Adjacent owner risk: local-memory load/store/GEP production, prepared authority, RV64 local-memory consumption, global initializer/bootstrap, ABI stack-frame layout, runtime, and expectation/accounting work.

## Non-Goals

- Do not implement global initializer bootstrap, ABI stack-frame layout, RV64 stack-frame consumption, runtime support, expectations, unsupported markers, allowlists, timeouts, or accounting.
- Do not claim prepared/RV64 authority failures as BIR alloca or scalar/local-memory success.
- Do not weaken diagnostics for load, store, or GEP failures that remain owned by their own producer routes.
- Do not special-case `src/20180921-1.c` or any other single mixed row.

## Working Model

- Treat a row as in scope only when the first missing fact is produced by BIR alloca-backed local-memory or scalar/local-memory mixed lowering.
- Use representative rows as probes, then require same-family breadth before closure.
- Preserve explicit handoffs to load, store, GEP, prepared authority, RV64 consumer, ABI, global, and runtime owners.
- If evidence shows a row depends on target-side frame layout or consumer materialization, record it as adjacent ownership instead of pulling it into this runbook.

## Execution Rules

- Keep packet progress and refreshed row lists in `todo.md`; rewrite this runbook only for a real route correction.
- Each code-changing step needs at least `cmake --build --preset default` plus the supervisor-delegated RV64 gcc_torture proof subset.
- Prefer narrow representative proof first, then broaden to the same-family rows used for closure evidence.
- Reject testcase-overfit routes even when a selected row becomes green.

## Ordered Steps

### Step 1: Select Alloca And Scalar-Local Proof Rows

Goal: identify representative alloca and scalar/local-memory mixed rows plus guard rows from adjacent owners.

Actions:
- Inspect current RV64 gcc_torture scan artifacts and per-case logs for alloca local-memory and scalar/local-memory mixed semantic failures.
- Refresh the starting row list if source evidence is stale.
- Select more than one in-scope alloca or mixed row for the narrow proof.
- Select guard rows for load, store, GEP, prepared/RV64, global, ABI, runtime, and expectation ownership where available.
- Record selected rows, expected pre-fix outcomes, and the exact supervisor proof command in `todo.md`.

Completion check:
- `todo.md` names the representative proof rows, guard rows, starting expectations, refreshed evidence location, and delegated proof command.

### Step 2: Trace The Producer Boundary

Goal: prove that the selected failures belong to BIR alloca-backed local-memory or scalar/local-memory mixed semantic production.

Actions:
- Compare HIR, BIR, and LLVM-route behavior for the selected rows.
- Identify the first producer diagnostic and the lowering functions/files that emit it.
- Separate alloca/scalar producer gaps from load/store/GEP producer gaps and prepared/RV64 consumer handoffs.
- Record any excluded rows as guard or adjacent-owner evidence in `todo.md`.

Completion check:
- `todo.md` records the owning BIR producer path, missing semantic fact, and rows excluded from Step 3 implementation ownership.

### Step 3: Repair In-Scope BIR Production

Goal: implement the narrow BIR producer behavior needed for the selected alloca or scalar/local-memory mixed subfamily.

Primary target:
- BIR local-memory lowering and scalar/local-memory interaction code that can publish the missing producer facts before prepared or RV64 handoff.

Actions:
- Implement the minimal semantic producer change for the selected subfamily.
- Preserve existing diagnostics and owner boundaries for load, store, GEP, prepared authority, RV64 consumer, global, ABI, runtime, expectation, and allowlist cases.
- Build and run the supervisor-delegated narrow proof.
- Stop for plan review if the required fix becomes stack-frame, ABI, RV64 consumer, or named-case logic.

Completion check:
- Build passes.
- More than one selected alloca or scalar/local-memory row progresses beyond the original BIR producer stop or reaches a defensible downstream owner.
- Guard rows keep their established ownership.

### Step 4: Prove Same-Family Breadth

Goal: show that the repair generalizes beyond the initial proof rows without collapsing adjacent owner boundaries.

Actions:
- Rerun the narrow proof if Step 3 changed after first validation.
- Run the broader same-family RV64 gcc_torture backend-object subset requested by the supervisor.
- Compare current row outcomes against the refreshed Step 1 evidence.
- Record progressed rows, remaining alloca/scalar-local limitations, adjacent-owner rows, and guard preservation in `todo.md`.

Completion check:
- `todo.md` summarizes same-family movement, remaining in-scope limitations, preserved adjacent owners, and any downstream handoffs.

### Step 5: Closure Readiness Summary

Goal: make the lifecycle handoff clear enough for the supervisor to decide close, rewrite, or continue.

Actions:
- Summarize implementation surface and proof results.
- Separate remaining alloca/scalar-local limitations from load/store/GEP, prepared/RV64, global, ABI, runtime, and expectation owners.
- State whether the source idea acceptance criteria are satisfied.
- Recommend the next lifecycle action.

Completion check:
- `todo.md` contains final evidence, remaining limitations, and a clear closure-readiness recommendation.

## Acceptance Gate

This plan is complete only when multiple alloca or scalar/local-memory mixed rows progress beyond the old BIR producer stop, load/store/GEP diagnostics remain owned by their explicit producer routes, and proof uses the RV64 gcc_torture backend-object route or an equivalent same-family subset.
