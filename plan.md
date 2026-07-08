# BIR Local-Memory Store Semantics Runbook

Status: Active
Source Idea: ideas/open/603_bir_local_memory_store_semantics.md

## Purpose

Repair BIR production of local-memory store semantics so RV64 gcc_torture rows that currently stop at the store producer boundary can progress to their next true owner.

## Goal

Produce semantic BIR store facts for ordinary local-frame writes while preserving the load, GEP, alloca, prepared, RV64, ABI, runtime, and global-data boundaries.

## Core Rule

Fix the store semantic producer. Do not make a row pass by reclassifying a non-store failure as a store, adding named-case shortcuts, weakening diagnostics, or compensating in RV64 lowering.

## Read First

- ideas/open/603_bir_local_memory_store_semantics.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md
- docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md
- docs/rv64_gcc_torture_1000_pass_recovery/index.md

## Current Targets

- Owning layer: BIR semantic producer.
- Evidence breadth: current scan reported `56` local-memory store rows.
- Proof surface: RV64 gcc_torture backend-object rows that stop in local-memory store semantic production.
- Starting evidence context: after 602, full RV64 scan artifacts were reported at `473/1467`.

## Non-Goals

- Do not touch load, GEP, alloca, global data, RV64 target lowering, ABI, runtime, expectations, unsupported markers, allowlists, timeout, or accounting behavior.
- Do not combine this route with local-memory GEP, alloca/scalar, ABI, runtime, or global-data work.
- Do not claim progress through expectation rewrites, diagnostic weakening, classification-only changes, or helper renames that retain the same unsupported store family.

## Working Model

- Treat local-memory store rows as BIR producer failures until evidence proves a downstream owner.
- Keep store source authority, destination/address validation, and target materialization distinct.
- Preserve the completed 602 load-semantics boundary; this route should not regress load-family behavior or move load ownership into the store repair.
- Use representative rows as probes only. The implementation must be semantic store production, not testcase-name matching.

## Execution Rules

- Keep packet progress in `todo.md`; rewrite this runbook only for a real route correction.
- Each implementation step that changes code needs at least `cmake --build --preset default` plus the supervisor-delegated proof subset.
- Use the narrow RV64 gcc_torture backend-object proof first, then broaden when same-family movement needs confirmation.
- Preserve guard rows for load, GEP, alloca, prepared/RV64, ABI, runtime, and global-data ownership.
- If evidence shows this is actually an address-formation, alloca, or RV64 consumer problem, stop and hand the lifecycle decision back instead of broadening this idea in place.

## Ordered Steps

### Step 1: Select Representative Store-Family Proof Rows

Goal: identify a small proof set that exercises local-memory store producer stops and nearby non-store guard owners.

Actions:
- Inspect current RV64 gcc_torture scan artifacts and per-case logs for rows classified as local-memory store producer failures.
- Select more than one store-shaped row from the `56`-row family.
- Select guard rows that cover load, GEP, alloca, prepared/RV64, ABI, runtime, and global-data ownership where available.
- Record the selected rows and the exact proof command in `todo.md`.

Completion check:
- `todo.md` names the store proof rows, guard rows, expected pre-fix failures, and the delegated proof command.

### Step 2: Trace the Store Producer Boundary

Goal: prove the first missing semantic fact is produced by the BIR local-memory store lowering path.

Actions:
- Compare HIR and BIR dumping behavior for the selected store rows.
- Identify the first producer diagnostic and the owning source files/functions.
- Separate store source-value handling from destination/address authority.
- Record any rows that are actually GEP, alloca, load, prepared/RV64, ABI, runtime, or global-data owners as guards, not implementation targets.

Completion check:
- `todo.md` records the owning BIR producer path, the missing store fact, and the rows excluded from Step 3 implementation ownership.

### Step 3: Repair Local-Memory Store Production

Goal: add the narrow semantic BIR store producer behavior needed for ordinary local-frame writes.

Primary target:
- BIR local-memory lowering code, especially the same producer area that emits local-memory load/store facts.

Actions:
- Implement the minimal producer change for ordinary local-frame store writes.
- Preserve diagnostics for unsupported address, source, aggregate, alloca, GEP, and RV64 consumer cases.
- Avoid changing expectations, unsupported markers, allowlists, runtime, timeout, accounting, or RV64 materialization.
- Build and run the supervisor-delegated narrow proof.

Completion check:
- Build passes.
- More than one selected store row progresses beyond the old BIR store producer stop or reaches a defensible downstream owner.
- Guard rows keep their non-store ownership.

### Step 4: Prove Same-Family Breadth

Goal: show the repair generalizes beyond the initial proof rows.

Actions:
- Run the selected narrow proof again if Step 3 changed after first proof.
- Run the broader same-family RV64 gcc_torture backend-object subset requested by the supervisor.
- Compare current store-family stop counts against the pre-repair `56` evidence where the artifacts support a defensible comparison.
- Record progressed rows and remaining store-family limitations in `todo.md`.

Completion check:
- `todo.md` summarizes same-family movement, remaining store limitations, guard-owner preservation, and any downstream handoffs.

### Step 5: Final Proof Summary and Closure Readiness

Goal: make the lifecycle handoff clear enough for the supervisor to decide close, rewrite, or continue.

Actions:
- Summarize the implementation surface and proof results.
- List remaining local-memory store limitations separately from adjacent owner families.
- State whether the source idea acceptance criteria are satisfied.
- Recommend the next lifecycle action.

Completion check:
- `todo.md` contains final evidence, remaining limitations, and a clear closure-readiness recommendation.

### Step 6: Classify the Remaining Store Subfamily

Goal: identify the next common producer limitation inside the remaining visible local-memory store stops without broadening into adjacent owners.

Actions:
- Start from the Step 5 evidence: `6/46` full-row BIR dump successes and `40/46` remaining visible rows still reporting the store local-memory semantic family.
- Sample the remaining store-family rows with focused HIR and BIR dumps.
- Group failures by first missing store fact, source-value shape, destination/address authority, and aggregate/global-data handoff risk.
- Keep `src/pr39120.c` separate unless the focused failing function proves it is still a local-memory store producer problem rather than the later aggregate-to-global boundary.
- Record the selected next producer subfamily, representative rows, guard rows, and exact supervisor proof command in `todo.md`.

Completion check:
- `todo.md` names the next local-memory store producer subfamily, at least two representative rows, excluded adjacent-owner rows, guard rows, and the delegated proof command.

### Step 7: Reclassify Remaining In-Scope Store Ownership

Goal: repair the Step 6 route classification after local aggregate subobject stores proved already supported for local-only probes.

Actions:
- Treat aggregate value copy/store to global or static storage as adjacent global-data handoff work, not as a Step 7 implementation target for this runbook.
- Record `ideas/open/619_bir_aggregate_global_store_handoff.md` as the durable follow-up for `u = v`, `x = <clit>`, `s[1] = t`, `x = foo(&i)`, and similar aggregate/static global handoff shapes.
- Reclassify the remaining visible store-family rows into:
  - in-scope ordinary local-frame store producer limitations, if any remain;
  - pointer/address-authority rows that belong to a separate address route;
  - function-label/local pointer-array rows that need separate source or destination authority;
  - aggregate/global-data handoff rows covered by idea 619 or existing global-data ideas;
  - non-store guard owners.
- Select the next implementation packet only if at least two rows share a first missing local-memory store producer fact that is still inside idea 603.
- Record the classification, representative rows, excluded rows, and exact proof command in `todo.md`.

Completion check:
- `todo.md` no longer asks executor to repair already-supported local aggregate subobject stores.
- `todo.md` either names a defensible next in-scope local-memory store producer subfamily for Step 8, or recommends parking idea 603 without closure because the remaining visible stops are adjacent owner families.

### Step 8: Repair the Next In-Scope Store Producer Subfamily

Goal: implement one generic BIR local-memory store producer repair only if Step 7 identifies an in-scope subfamily.

Primary target:
- BIR local-memory store producer code, especially `src/backend/bir/lir_to_bir/memory/` and scalar value lowering only when Step 7 proves the missing fact is a store source-value issue.

Actions:
- Implement the minimal semantic producer change for the selected in-scope subfamily.
- Preserve the Step 3 repairs for `f128` local immediates, loaded global pointer-field authority, and dynamic local pointer-array stores.
- Preserve diagnostics for unsupported aggregate/global-data, GEP, alloca, ABI, runtime, RV64 consumer, expectation, and allowlist cases.
- Do not implement aggregate/static global handoff shapes under this idea; use idea 619 for that work.
- Build and run the supervisor-delegated narrow proof.

Completion check:
- Build passes.
- More than one Step 7 representative row progresses beyond the old local-memory store producer stop or reaches a defensible downstream owner.
- Guard rows keep their established non-store ownership.

### Step 9: Reprove Store-Family Breadth After Any Follow-Up Repair

Goal: prove whether any Step 8 repair generalizes across the remaining visible store-family rows.

Actions:
- Run the selected narrow proof again if Step 8 changed after first proof.
- Run the broader same-family RV64 gcc_torture backend-object subset requested by the supervisor.
- Re-run focused BIR dump classification for the visible store-family rows when the supervisor asks for same-family movement evidence.
- Compare against the Step 5 checkpoint of `6/46` full-row BIR dump successes and `40/46` remaining visible store-family stops.
- Record progressed rows, remaining store-family limitations, stale-artifact caveats, guard-owner preservation, and downstream handoffs in `todo.md`.

Completion check:
- `todo.md` summarizes same-family movement after Step 8, remaining local-memory store limitations, guard-owner preservation, and whether another runbook refinement is needed.

### Step 10: Updated Closure Readiness

Goal: decide whether idea 603 is now closure-ready or still needs another local-memory store producer packet.

Actions:
- Summarize all implementation surfaces accepted under this runbook.
- Separate remaining local-memory store limitations from adjacent owner families.
- State whether the source idea acceptance criteria are satisfied.
- Recommend close only if source-idea completion is defensible under refreshed evidence; otherwise recommend the next local-memory store producer substep, parking this idea, or switching to a separate lifecycle idea such as 619.

Completion check:
- `todo.md` contains updated final evidence, remaining limitations, and a clear close-or-continue recommendation.

## Acceptance Gate

This plan is complete only when same-family local-memory store rows progress beyond the current BIR producer stop, non-store guard owners are preserved, and proof includes more than one store-shaped row from the current scan.
