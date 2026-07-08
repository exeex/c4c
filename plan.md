# BIR Local-Memory GEP And Address Semantics Runbook

Status: Active
Source Idea: ideas/open/604_bir_local_memory_gep_address_semantics.md

## Purpose

Repair BIR production for local-memory GEP and address formation so RV64 gcc_torture rows that currently stop at the address producer boundary can progress to their next true owner.

## Goal

Produce BIR-local address facts for ordinary local-memory GEP shapes while preserving the pointer/address authority model and keeping RV64 consumer inference out of this route.

## Core Rule

Fix the BIR address semantic producer. Do not make a row pass by inferring target-side pointer authority, broadening unsupported pointer arithmetic policy, matching named cases, weakening diagnostics, or rewriting expectations.

## Read First

- ideas/open/604_bir_local_memory_gep_address_semantics.md
- ideas/closed/597_pointer_address_semantic_model_research.md
- ideas/closed/599_pointer_base_plus_offset_selected_authority.md
- ideas/closed/600_pointer_value_memory_use_freshness_authority.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md
- docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md
- docs/rv64_gcc_torture_1000_pass_recovery/index.md

## Current Targets

- Owning layer: BIR semantic producer.
- Evidence breadth: source idea reports `43` local-memory GEP rows, with overlap risk against `27` pointer local-memory consumer rows.
- Proof surface: RV64 gcc_torture backend-object rows whose first reproduced stop is local-memory GEP/address semantic production.
- Adjacent owner risk: pointer/base+offset authority, unsupported pointer arithmetic policy, RV64 local-memory consumption, and global symbol address formation.

## Non-Goals

- Do not implement RV64 local-memory frame-slot consumption.
- Do not add broad pointer arithmetic support or target-side address reconstruction.
- Do not combine this route with alloca/scalar, global initializer, prepared/global authority, ABI, runtime, expectation, unsupported-marker, allowlist, timeout, or accounting work.
- Do not claim progress through expectation rewrites, diagnostic weakening, classification-only changes, or helper renames that retain the same local-memory GEP stop.

## Working Model

- Treat local-memory GEP rows as BIR producer failures only when the BIR layer can prove the address shape before prepared or RV64 consumption.
- Keep address production, pointer-value authority, selected base+offset authority, and target materialization distinct.
- Use representative rows as probes only. The implementation must be semantic GEP/address production, not testcase-name matching.
- If evidence shows a row requires pointer architecture policy or RV64 consumer authority, record it as an adjacent owner instead of pulling it into this plan.

## Execution Rules

- Keep packet progress in `todo.md`; rewrite this runbook only for a real route correction.
- Each implementation step that changes code needs at least `cmake --build --preset default` plus the supervisor-delegated proof subset.
- Start with a narrow RV64 gcc_torture backend-object proof, then broaden to same-family rows when movement needs confirmation.
- Preserve guard rows for local-memory load/store, alloca/scalar, prepared/RV64, global-data, ABI, runtime, and pointer/address ownership.
- If a candidate fix would reconstruct addresses on the RV64 side without BIR facts, stop and hand the lifecycle decision back.

## Ordered Steps

### Step 1: Select Representative GEP-Family Proof Rows

Goal: identify a small proof set that exercises local-memory GEP/address producer stops and nearby pointer/address guard owners.

Actions:
- Inspect current RV64 gcc_torture scan artifacts and per-case logs for rows classified as local-memory GEP/address semantic failures.
- Select more than one GEP-shaped row from the reported `43`-row family.
- Select guard rows that cover pointer/base+offset authority, pointer-value freshness, local-memory load/store, alloca/scalar, prepared/RV64, global-data, ABI, and runtime ownership where available.
- Record the selected rows, expected pre-fix outcomes, and exact supervisor proof command in `todo.md`.

Completion check:
- `todo.md` names the GEP proof rows, guard rows, expected pre-fix failures, and delegated proof command.

### Step 2: Trace the GEP Producer Boundary

Goal: prove the first missing semantic fact belongs to BIR local-memory address production.

Actions:
- Compare HIR and BIR dumping behavior for the selected GEP rows.
- Identify the first producer diagnostic and the owning source files/functions.
- Separate local address-shape production from pointer-value freshness, selected base+offset authority, and RV64 frame-slot consumption.
- Record rows that actually belong to pointer policy, local-memory load/store, alloca/scalar, prepared/RV64, global-data, ABI, or runtime owners as guards rather than implementation targets.

Completion check:
- `todo.md` records the owning BIR producer path, the missing address fact, and the rows excluded from Step 3 implementation ownership.

### Step 3: Repair Local-Memory GEP Address Production

Goal: add the narrow BIR producer behavior needed for ordinary local-memory GEP/address shapes.

Primary target:
- BIR local-memory lowering and address/provenance code, especially producer paths that can publish local address facts before RV64 consumption.

Actions:
- Implement the minimal producer change for the selected local-memory GEP/address subfamily.
- Reuse the established pointer/address model from ideas 597, 599, and 600.
- Preserve diagnostics for unsupported pointer arithmetic, missing pointer authority, load/store, alloca/scalar, prepared/RV64, global-data, ABI, runtime, expectation, and allowlist cases.
- Build and run the supervisor-delegated narrow proof.

Completion check:
- Build passes.
- More than one selected GEP row progresses beyond the old BIR address producer stop or reaches a defensible downstream owner.
- Guard rows keep their established non-GEP ownership.

### Step 4: Prove Same-Family Breadth

Goal: show the repair generalizes beyond the initial proof rows without collapsing adjacent pointer/address ownership.

Actions:
- Run the selected narrow proof again if Step 3 changed after first proof.
- Run the broader same-family RV64 gcc_torture backend-object subset requested by the supervisor.
- Compare current GEP/address stop counts against the starting `43`-row evidence where refreshed artifacts support a defensible comparison.
- Record progressed rows, remaining GEP limitations, adjacent-owner rows, and guard-owner preservation in `todo.md`.

Completion check:
- `todo.md` summarizes same-family movement, remaining GEP/address limitations, guard-owner preservation, and any downstream handoffs.

### Step 5: Final Proof Summary and Closure Readiness

Goal: make the lifecycle handoff clear enough for the supervisor to decide close, rewrite, or continue.

Actions:
- Summarize the implementation surface and proof results.
- List remaining local-memory GEP/address limitations separately from adjacent pointer/address, prepared/RV64, global-data, ABI, runtime, and policy owners.
- State whether the source idea acceptance criteria are satisfied.
- Recommend the next lifecycle action.

Completion check:
- `todo.md` contains final evidence, remaining limitations, and a clear closure-readiness recommendation.

## Acceptance Gate

This plan is complete only when multiple local-memory GEP/address rows progress beyond the current BIR producer stop, rows that require pointer/address architecture discussion remain rejected or classified with that owner, and proof demonstrates the route reuses the established pointer/address model rather than testcase-local exceptions.
